/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 *
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL.h>

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <thread>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/GLPlatform.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

// Implemented in uwp/PvzpUwpMetadata.cpp (C++/CX). Returns the absolute path
// of this app's LocalState folder, or an empty string on failure.
std::string PvzpUwpGetLocalStatePath();

// Implemented in uwp/PvzpUwpMetadata.cpp (C++/CX). Runs a callable, catching
// C++/CX Platform::Exception. Returns 0 on success, -1 on failure (outError
// filled with a diagnostic).
int PvzpUwpRunCxGuarded(const std::function<void()>& theCallable, std::string& outError);

namespace
{
void UwpDebugLog(const std::string& theMessage)
{
	std::string aPath = PvzpUwpGetLocalStatePath();
	if (aPath.empty())
		return;
	std::ofstream aFile(aPath + "/uwp_debug.log", std::ios::app);
	if (aFile)
	{
		aFile << theMessage << "\n";
		if (const char* anError = SDL_GetError(); anError && *anError)
			aFile << "  SDL_GetError: " << anError << "\n";
	}
}

// Heartbeat thread: writes a timestamp every 200 ms so we can tell whether the
// process hangs inside a call or is killed outright, and how long it lives.
void RunUwpWatchdog(const std::atomic<bool>& aRunning)
{
	std::string aPath = PvzpUwpGetLocalStatePath();
	if (aPath.empty())
		return;
	const auto aT0 = std::chrono::steady_clock::now();
	while (aRunning.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		const auto aMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - aT0).count();
		std::ofstream aFile(aPath + "/uwp_debug.log", std::ios::app);
		if (aFile)
			aFile << "watchdog t=" << aMs << "ms\n";
	}
}

// SEH crash trap, used as an __except filter. Logs the exception code,
// faulting address and a module-annotated stack so native crashes inside
// SDL/ANGLE (which the C++/CX guard cannot catch) leave a trace.
void UwpLogFault(unsigned int aCode, EXCEPTION_POINTERS* aPointers)
{
	std::string aPath = PvzpUwpGetLocalStatePath();
	if (aPath.empty())
		return;
	std::ofstream aFile(aPath + "/uwp_debug.log", std::ios::app);
	if (!aFile)
		return;
	aFile << "SEH FAULT code=0x" << std::hex << aCode << std::dec;
	if (aPointers && aPointers->ExceptionRecord)
		aFile << " addr=0x" << std::hex << (uintptr_t)aPointers->ExceptionRecord->ExceptionAddress << std::dec;
	aFile << "\n";
	void* aFrames[32];
	const int aCount = CaptureStackBackTrace(0, 32, aFrames, nullptr);
	for (int i = 0; i < aCount; i++)
	{
		aFile << "  #" << i << " 0x" << std::hex << (uintptr_t)aFrames[i] << std::dec;
		HMODULE aModule = nullptr;
		if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				(LPCWSTR)aFrames[i], &aModule))
		{
			wchar_t aWide[MAX_PATH] = {};
			if (GetModuleFileNameW(aModule, aWide, MAX_PATH) > 0)
			{
				std::wstring aMod(aWide);
				const auto aSlash = aMod.find_last_of(L'\\');
				if (aSlash != std::wstring::npos)
					aMod = aMod.substr(aSlash + 1);
				char aName[MAX_PATH] = {};
				::WideCharToMultiByte(CP_UTF8, 0, aMod.c_str(), (int)aMod.size(), aName, MAX_PATH, nullptr, nullptr);
				aFile << " " << aName;
			}
		}
		aFile << "\n";
	}
}

int UwpCallLoadLibrary()
{
	__try
	{
		return SDL_GL_LoadLibrary(nullptr);
	}
	__except (UwpLogFault(GetExceptionCode(), GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
		return -1001;
	}
}

void* UwpCallCreateWindow(const char* aTitle, int aWidth, int aHeight, Uint32 aFlags)
{
	__try
	{
		return (void*)SDL_CreateWindow(aTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, aWidth, aHeight, aFlags);
	}
	__except (UwpLogFault(GetExceptionCode(), GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
		return nullptr;
	}
}

void* UwpCallCreateContext(SDL_Window* aWindow)
{
	__try
	{
		return (void*)SDL_GL_CreateContext(aWindow);
	}
	__except (UwpLogFault(GetExceptionCode(), GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
		return (void*)(intptr_t)-1;
	}
}
} // namespace

// UWP (Xbox One / PC) has no desktop OpenGL: rendering is OpenGL ES 2.0
// through SDL2's winrt video driver, which provides EGL via ANGLE (D3D11).
// The window is always borderless fullscreen, and input is a virtual cursor
// driven by the gamepad (see platform/default/Input.cpp).

void SexyAppBase::MakeWindow()
{
	if (mWindow)
	{
		SDL_SetWindowFullscreen((SDL_Window*)mWindow, SDL_WINDOW_FULLSCREEN);
	}
	else
	{
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

		std::atomic<bool> aWatchdogRunning{ true };
		std::thread aWatchdog(RunUwpWatchdog, std::ref(aWatchdogRunning));

		std::string aErr;
		int aSdlInit = -1;
		UwpDebugLog("MakeWindow: SDL_Init(VIDEO) before");
		if (PvzpUwpRunCxGuarded([&]() { aSdlInit = SDL_Init(SDL_INIT_VIDEO); }, aErr) != 0)
			UwpDebugLog("MakeWindow: SDL_Init threw: " + aErr);
		else
			UwpDebugLog(aSdlInit < 0 ? "MakeWindow: SDL_Init returned <0" : "MakeWindow: SDL_Init returned 0");

		Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;

		// OpenGL ES 2.0 via ANGLE only
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		// SDL_CreateWindow(OPENGL) lazily loads the GL/EGL library (ANGLE +
		// D3D11 device init) as its first step. Load it explicitly first so a
		// crash there can be isolated from window creation.
		int aLoadLib = -2;
		UwpDebugLog("MakeWindow: SDL_GL_LoadLibrary before");
		if (PvzpUwpRunCxGuarded([&]() { aLoadLib = UwpCallLoadLibrary(); }, aErr) != 0)
			UwpDebugLog("MakeWindow: SDL_GL_LoadLibrary threw: " + aErr);
		else if (aLoadLib == -1001)
			UwpDebugLog("MakeWindow: SDL_GL_LoadLibrary SEH CRASH (see SEH FAULT)");
		else
			UwpDebugLog(aLoadLib == 0 ? "MakeWindow: SDL_GL_LoadLibrary OK" : "MakeWindow: SDL_GL_LoadLibrary FAILED");
		if (aLoadLib == -1001)
		{
			aWatchdogRunning = false;
			aWatchdog.join();
			return;
		}

		if (PvzpUwpRunCxGuarded([&]() {
				mWindow = UwpCallCreateWindow(mTitle.c_str(), mWidth, mHeight, winFlags);
			}, aErr) != 0)
			UwpDebugLog("MakeWindow: SDL_CreateWindow threw: " + aErr);
		else if (mWindow)
			UwpDebugLog("MakeWindow: SDL_CreateWindow OK");
		else
			UwpDebugLog("MakeWindow: SDL_CreateWindow NULL");

		auto aCreateContext = [&]() {
			UwpDebugLog("MakeWindow: SDL_GL_CreateContext attempt");
			if (PvzpUwpRunCxGuarded([&]() {
					mContext = UwpCallCreateContext((SDL_Window*)mWindow);
				}, aErr) != 0)
				UwpDebugLog("MakeWindow: SDL_GL_CreateContext threw: " + aErr);
			else if (mContext == (void*)(intptr_t)-1)
			{
				UwpDebugLog("MakeWindow: SDL_GL_CreateContext SEH CRASH (see SEH FAULT)");
				mContext = nullptr;
			}
		};
		if (mWindow)
			aCreateContext();

		// EGL surfaces may be transiently unavailable on WinRT while the
		// swap chain settles (app launch, display handoff, etc.)
		for (int retry = 0; !mContext && mWindow && retry < 20; retry++)
		{
			SDL_Delay(100);
			SDL_PumpEvents();
			aCreateContext();
		}
		aWatchdogRunning = false;
		aWatchdog.join();
		if (!mContext)
		{
			UwpDebugLog("MakeWindow: SDL_GL_CreateContext FAILED after retries");
			if (mWindow) { SDL_DestroyWindow((SDL_Window*)mWindow); mWindow = nullptr; }
			Sexy::LogError("Failed to create OpenGL ES context (ANGLE).");
			return;
		}

		UwpDebugLog("MakeWindow: context created, SetSwapInterval");
		SDL_GL_SetSwapInterval(1);
	}

	if (mGLInterface == nullptr)
	{
		mGLInterface = new GLInterface(this);
		if (!InitGLInterface())
		{
			UwpDebugLog("MakeWindow: InitGLInterface FAILED");
			delete mGLInterface;
			mGLInterface = nullptr;
			return;
		}
	}

	UwpDebugLog("MakeWindow: complete");

	bool isActive = mActive;
	mActive = mMinimized ? false : true;

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();

	mGLInterface->UpdateViewport();
	mWidgetManager->Resize(mScreenBounds, mGLInterface->mInputSourceRect);
}
