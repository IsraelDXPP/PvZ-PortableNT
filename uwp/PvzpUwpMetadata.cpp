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
