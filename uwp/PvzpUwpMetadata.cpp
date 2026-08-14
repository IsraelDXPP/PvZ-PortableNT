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

namespace PvzpUwp
{
	ref class PvzpUwpMetadata sealed
	{
	public:
		PvzpUwpMetadata() {}
	};
}
