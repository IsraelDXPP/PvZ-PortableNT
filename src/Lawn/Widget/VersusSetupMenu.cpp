/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
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

#include "VersusSetupMenu.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "widget/Dialog.h"
#include "PvzpLib/PvzpDebug.h"

using namespace Sexy;

namespace
{
	// Verbatim VSSetupMenu.txt as supplied from the console/Android TV build (the user
	// extracted the real menu/*.txt files from that build and provided them for this
	// port). Kept inline here rather than as a shipped data file: this port has no
	// resource-pack entry or file path for menu/*.txt, and no generic menu-file loader
	// that reads from disk at runtime -- see MenuWidget.h for why a from-scratch one
	// wasn't built out further than the commands actually needed to run this script.
	constexpr const char* kVSSetupMenuScript = R"MENU(
DefineWidgetIds ( QUICK_BUTTON, CUSTOM_BUTTON, RANDOM_BUTTON );

Enum (
	WIDGET_ANIMATION_IDLE,
	WIDGET_ANIMATION_ENTER,
	WIDGET_ANIMATION_EXIT
	);

Define EASE_IN_OUT_ANIMATOR 4;
Define SIN_WAVE_ANIMATOR 12;

SetInitialFocus QUICK_BUTTON;

Define BUTTON_X 272;
Define BUTTON_WIDTH 256;
Define BUTTON_HEIGHT 64;

AddWidget ButtonWidget QUICK_BUTTON;
SetFont 'FONT_BRIANNETOD16';
SetLabel 'Quick Play';
Resize BUTTON_X 200 BUTTON_WIDTH BUTTON_HEIGHT;
SetGameLinks -1 CUSTOM_BUTTON -1 -1;

AddWidget ButtonWidget CUSTOM_BUTTON;
SetFont 'FONT_BRIANNETOD16';
SetLabel 'Custom Battle';
Resize BUTTON_X 300 BUTTON_WIDTH BUTTON_HEIGHT;
SetGameLinks QUICK_BUTTON RANDOM_BUTTON -1 -1;

AddWidget ButtonWidget RANDOM_BUTTON;
SetFont 'FONT_BRIANNETOD16';
SetLabel 'Random Battle';
Resize BUTTON_X 400 BUTTON_WIDTH BUTTON_HEIGHT;
SetGameLinks CUSTOM_BUTTON -1 -1 -1;

AddWidget HelpBarWidget HELP_BAR;
AddHelpButton A 'Select';
AddAnimator WIDGET_ANIMATION_ENTER TodCurveWidgetAnimator ( 5, (0,1000), (0,600), 100 ) 0;
AddAnimator WIDGET_ANIMATION_IDLE TodCurveWidgetAnimator ( 11, (0,600), (0,590), 35 ) 0;
AddAnimator WIDGET_ANIMATION_EXIT TodCurveWidgetAnimator ( EASE_IN_OUT_ANIMATOR, (0,600), (0,1000), 100 ) 5;
)MENU";
}

VersusSetupMenu::VersusSetupMenu(LawnApp* theApp) :
	mApp(theApp)
{
	if (!LoadMenuFile(kVSSetupMenuScript))
		PvzpLogLn("VersusSetupMenu: failed to load the embedded VSSetupMenu.txt script: {}", mLastError);
}

void VersusSetupMenu::OnMenuButtonDepress(int theId)
{
	if (theId == GetWidgetId("QUICK_BUTTON"))
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		mApp->KillVersusSetupMenu();
		mApp->StartMultiplayerGame(GameMode::GAMEMODE_VERSUS);
		return;
	}

	if (theId == GetWidgetId("CUSTOM_BUTTON") || theId == GetWidgetId("RANDOM_BUTTON"))
	{
		// The real next screens for these two (VSSetupSides.txt -> VSSetupControllers.txt,
		// also user-supplied) pick a lawn/side and detect a second controller -- they need
		// ImageWidget/LawnButtonWidget (no art for them exists in this port's original PC
		// resource pack either) and per-controller input this port doesn't have. Saying so
		// is more honest than silently running the same Quick Play flow, which would
		// misrepresent the choice as if it did something different.
		mApp->PlaySample(Sexy::SOUND_TAP);
		mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[MODE_LOCKED]", "[VERSUS_CUSTOM_NOT_AVAILABLE]", "[DIALOG_BUTTON_OK]", "", Dialog::BUTTONS_FOOTER);
	}
}
