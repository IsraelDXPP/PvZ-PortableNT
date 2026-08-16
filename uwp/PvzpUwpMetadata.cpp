/*
 * PvZ-Portable (UWP/Xbox One)
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is compiled with /ZW (C++/CX) and provides a Windows Metadata
 * anchor so the linker emits the app's .winmd. AppX packaging requires the
 * package to contain a .winmd for C++/CX-built apps; without at least one
 * /ZW-compiled translation unit the metadata file is not generated and
 * builds fail with APPX0702.
 */

#include <string>
#include <functional>
#include <exception>
#include <windows.h>

namespace PvzpUwp
{
	ref class PvzpUwpMetadata sealed
	{
	public:
		PvzpUwpMetadata() {}
	};
}

// Absolute path to this app's LocalState folder (the per-user writable area
// outside the read-only package). On Xbox the game data (main.pak and the
// properties/ folder) is seeded there through the device portal file explorer:
//   LocalAppData\<PackageFamilyName>\LocalState
// SexyAppBase::Init() uses this as the UWP resource-folder fallback.
std::string PvzpUwpGetLocalStatePath()
{
	using namespace Windows::Storage;
	ApplicationData^ aData = ApplicationData::Current;
	if (aData == nullptr)
		return std::string();
	StorageFolder^ aFolder = aData->LocalFolder;
	if (aFolder == nullptr)
		return std::string();
	const std::wstring aWide(aFolder->Path->Data());
	if (aWide.empty())
		return std::string();
	const int aLen = ::WideCharToMultiByte(CP_UTF8, 0, aWide.c_str(), (int)aWide.size(), nullptr, 0, nullptr, nullptr);
	if (aLen <= 0)
		return std::string();
	std::string aOut(static_cast<size_t>(aLen), '\0');
	::WideCharToMultiByte(CP_UTF8, 0, aWide.c_str(), (int)aWide.size(), &aOut[0], aLen, nullptr, nullptr);
	return aOut;
}

// Runs a callable, catching C++/CX Platform::Exception (which SDL2's WinRT
// paths throw on failures) plus std::exception. Returns 0 on success; on
// failure returns -1 and fills outError with a diagnostic string.
int PvzpUwpRunCxGuarded(const std::function<void()>& theCallable, std::string& outError)
{
	try
	{
		theCallable();
		return 0;
	}
	catch (Platform::Exception^ e)
	{
		const std::wstring aMsg(e->Message != nullptr ? e->Message->Data() : L"(no message)");
		char aHex[16];
		std::snprintf(aHex, sizeof(aHex), "0x%08X", (unsigned int)e->HResult);
		outError = "Platform::Exception " + std::string(aHex);
		const int aLen = ::WideCharToMultiByte(CP_UTF8, 0, aMsg.c_str(), (int)aMsg.size(), nullptr, 0, nullptr, nullptr);
		if (aLen > 0)
		{
			std::string aUtf8(static_cast<size_t>(aLen), '\0');
			::WideCharToMultiByte(CP_UTF8, 0, aMsg.c_str(), (int)aMsg.size(), &aUtf8[0], aLen, nullptr, nullptr);
			outError += " " + aUtf8;
		}
		return -1;
	}
	catch (std::exception& e)
	{
		outError = "std::exception: ";
		outError += e.what();
		return -1;
	}
	catch (...)
	{
		outError = "unknown exception";
		return -1;
	}
}
