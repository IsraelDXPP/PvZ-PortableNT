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

#include "VersusResultsMenu.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "widget/ButtonWidget.h"
#include "PvzpLib/PvzpStringFile.h"
#include "PvzpLib/PvzpDebug.h"

using namespace Sexy;

namespace
{
	// Verbatim VSResultsMenu.txt as supplied from the console/Android TV build (same source
	// as VSSetupMenu.txt -- see that class's comment). Kept inline for the same reason: no
	// resource-pack entry or generic disk-based menu-file loader exists in this port.
	constexpr const char* kVSResultsMenuScript = R"MENU(
DefineWidgetIds ( PLAY_AGAIN, QUIT_BUTTON, INFO_BOX_P1, INFO_BOX_P2, PLANT_SIDE, PLANT_SIDE_FRONT, ZOMBIE_SIDE, ZOMBIE_SIDE_FRONT, WIN_IMAGE );

Enum
(
	COLOR_LABEL,
	COLOR_LABEL_HILITE,
	COLOR_DARK_OUTLINE,
	COLOR_LIGHT_OUTLINE,
	COLOR_MEDIUM_OUTLINE,
	COLOR_BKG,
	NUM_COLORS
);

Enum (
	WIDGET_ANIMATION_IDLE,
	WIDGET_ANIMATION_ENTER,
	WIDGET_ANIMATION_EXIT
	);


Enum (
	SLIDE_DOWN,
	SLIDE_UP,
	SLIDE_LEFT,
	SLIDE_RIGHT
);

Define BUTTON_LABEL_WRAP_CENTER 2;

Define EASE_IN_OUT_ANIMATOR 4;
Define SIN_WAVE_ANIMATOR 12;

SetInitialFocus PLAY_AGAIN;

Define BUTTON_Y 472;
Define BUTTON_WIDTH 205;
Define BUTTON_HEIGHT 96;

SetBackground 'IMAGE_CHALLENGE_BACKGROUND';

AddWidget LabelWidget -1;
SetLabel '[Battle Results]';

AddWidget ImageWidget PLANT_SIDE;
SetImage 'images/plant_side';
Resize 38 84 256 256;

AddWidget ImageWidget PLANT_SIDE_FRONT;
SetImage 'images/plant_side_plants';
Resize 36 84 256 256;

AddWidget ImageWidget ZOMBIE_SIDE;
SetImage 'images/zombie_side';
Resize 32 72 256 256;

AddWidget ImageWidget ZOMBIE_SIDE_FRONT;
SetImage 'images/zombie_side_zombies';
Resize 18 72 256 256;

AddWidget ImageWidget WIN_IMAGE;
SetImage 'images/win';
SetPos 96 385;

#AddWidget ImageWidget -1;
#SetImage 'images/plant_win_trophy';

#AddWidget ImageWidget -1;
#SetImage 'images/zombie_win_trophy';

#AddWidget ImageWidget -1;
#SetImage 'images/trophy_BaseSmall';


AddWidget ImageWidget INFO_BOX_P1;
SetImage 'images/vs_info_box_plants';
Resize 340 96 446 197;
AddAnimator WIDGET_ANIMATION_ENTER SlideInOutWidgetAnimator ( SLIDE_RIGHT );
AddAnimator WIDGET_ANIMATION_EXIT SlideInOutWidgetAnimator ( SLIDE_RIGHT );


AddWidget ImageWidget INFO_BOX_P2;
SetImage 'images/vs_info_box_zombies';
Resize 340 270 416 162;
AddAnimator WIDGET_ANIMATION_ENTER SlideInOutWidgetAnimator ( SLIDE_RIGHT, 20 );
AddAnimator WIDGET_ANIMATION_EXIT SlideInOutWidgetAnimator ( SLIDE_RIGHT, 20);

#
# VSResultsMenu
#

AddWidget LawnButtonWidget PLAY_AGAIN;
SetImage 'images/VS Button';
SetOverImage 'images/VS Button_selected';
SetFont 'FONT_DWARVENTODCRAFT24';
SetColor COLOR_LABEL (25,197,45);
SetColor COLOR_LABEL_HILITE (277,225,108);
SetLabelJustify BUTTON_LABEL_WRAP_CENTER;
SetLabel '[PLAY_AGAIN]';
Resize 180 BUTTON_Y BUTTON_WIDTH BUTTON_HEIGHT;
SetGameLinks -1 -1 -1 QUIT_BUTTON;

AddWidget LawnButtonWidget QUIT_BUTTON;
SetImage 'images/VS Button';
SetOverImage 'images/VS Button_selected';
SetFont 'FONT_DWARVENTODCRAFT24';
SetColor COLOR_LABEL (25,197,45);
SetColor COLOR_LABEL_HILITE (277,225,108);
SetLabelJustify BUTTON_LABEL_WRAP_CENTER;
SetLabel '[QUIT_VS]';
Resize 420 BUTTON_Y BUTTON_WIDTH BUTTON_HEIGHT;
SetGameLinks -1 -1 PLAY_AGAIN -1;

#AddWidget HelpBarWidget HELP_BAR;
#AddHelpButton A '[SELECT]';
#AddHelpButton B '[BACK]';
#AddAnimator WIDGET_ANIMATION_ENTER SlideInOutWidgetAnimator ( SLIDE_DOWN );
#AddAnimator WIDGET_ANIMATION_EXIT SlideInOutWidgetAnimator ( SLIDE_DOWN );
)MENU";

	// MenuWidget's SetLabel handler intentionally uses a command's label token as-is (it's
	// generic SexyAppFramework code with no dependency on PvzpLib's PvzpStringTranslate --
	// see that handler's comment): fine for VSSetupMenu.txt's plain literal labels, but this
	// script's SetLabel calls use bracketed localization keys ('[PLAY_AGAIN]', '[QUIT_VS]').
	// Re-resolving them here, in this Lawn-layer subclass, keeps that Sexy::/Lawn:: boundary
	// intact while still showing real text instead of the literal brackets.
	void RetranslateButtonLabel(VersusResultsMenu& theMenu, const char* theWidgetName)
	{
		auto* aButton = dynamic_cast<ButtonWidget*>(theMenu.GetWidgetById(theMenu.GetWidgetId(theWidgetName)));
		if (aButton)
			aButton->mLabel = PvzpStringTranslate(aButton->mLabel);
	}
}

VersusResultsMenu::VersusResultsMenu(LawnApp* theApp) :
	mApp(theApp)
{
	if (!LoadMenuFile(kVSResultsMenuScript))
	{
		PvzpLogLn("VersusResultsMenu: failed to load the embedded VSResultsMenu.txt script: {}", mLastError);
		return;
	}

	RetranslateButtonLabel(*this, "PLAY_AGAIN");
	RetranslateButtonLabel(*this, "QUIT_BUTTON");
}

void VersusResultsMenu::OnMenuButtonDepress(int theId)
{
	if (theId == GetWidgetId("PLAY_AGAIN"))
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		mApp->KillVersusResultsMenu();
		mApp->StartMultiplayerGame(GameMode::GAMEMODE_VERSUS);
		return;
	}

	if (theId == GetWidgetId("QUIT_BUTTON"))
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		mApp->KillVersusResultsMenu();
		mApp->KillBoard();
		mApp->ShowGameSelector();
	}
}
