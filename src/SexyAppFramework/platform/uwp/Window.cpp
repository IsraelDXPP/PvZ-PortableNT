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

#include <fstream>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/GLPlatform.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

// Implemented in uwp/PvzpUwpMetadata.cpp (C++/CX). Returns the absolute path
// of this app's LocalState folder, or an empty string on failure.
std::string PvzpUwpGetLocalStatePath();

namespace
{
void UwpDebugLog(const char* theMessage)
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

		UwpDebugLog("MakeWindow: SDL_Init(VIDEO) before");
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			UwpDebugLog("MakeWindow: SDL_Init(VIDEO) FAILED");

		Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;

		// OpenGL ES 2.0 via ANGLE only
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		mWindow = (void*)SDL_CreateWindow(
			mTitle.c_str(),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			mWidth, mHeight, winFlags);

		if (mWindow)
			UwpDebugLog("MakeWindow: SDL_CreateWindow OK");
		else
			UwpDebugLog("MakeWindow: SDL_CreateWindow NULL");

		if (mWindow)
		{
			UwpDebugLog("MakeWindow: SDL_GL_CreateContext before");
			mContext = (void*)SDL_GL_CreateContext((SDL_Window*)mWindow);
			if (mContext)
				UwpDebugLog("MakeWindow: SDL_GL_CreateContext OK (first try)");
		}

		// EGL surfaces may be transiently unavailable on WinRT while the
		// swap chain settles (app launch, display handoff, etc.)
		for (int retry = 0; !mContext && mWindow && retry < 20; retry++)
		{
			SDL_Delay(100);
			SDL_PumpEvents();
			mContext = (void*)SDL_GL_CreateContext((SDL_Window*)mWindow);
			if (mContext)
				UwpDebugLog("MakeWindow: SDL_GL_CreateContext OK (retry)");
		}
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
