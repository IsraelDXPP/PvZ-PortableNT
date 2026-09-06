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

#ifndef __VERSUSSETUPMENU_H__
#define __VERSUSSETUPMENU_H__

#include "widget/MenuWidget.h"
#include "ConstEnums.h"
#include <array>
#include <map>
#include <string>
#include <vector>

class LawnApp;

// VersusSetupMenu mirrors the console/Android TV build's VSSetupMenu (games/pvz/main/Lawn/
// VSSetupMenu.cpp in that decompiled source): the local-versus pre-match state machine that
// walks both players through (0) confirming a second player, (1) picking sides, (2) choosing
// how to fill the zombie deck (quick/custom/random), and (3) the actual seed chooser for both
// sides before the match starts.
//
// The original is gamepad-driven: WaitForSecondPlayerDialog waits for a second controller to
// press a button and then assigns it via SetSecondPlayerIndex, the sides/choreograph screens
// are navigated with gamepad buttons, and each player's seed chooser is a separate per-player
// SeedChooserScreen. This PC port has none of that dual-device input (all controllers merge
// into one virtual cursor -- see platform/default/Input.cpp) and one shared PC SeedChooserScreen,
// so the state machine is preserved 1:1 while every gamepad button is remapped to the port's
// existing PC keyboard/mouse controls:
//
//   gamepad A            -> Enter / mouse click (player 1) or keyboard (player 2)
//   gamepad B / Back     -> Escape
//   WaitForSecondPlayer  -> a modal dialog that accepts any key for player two
//   per-player chooser   -> the port's single SeedChooserScreen, run once per side
//
// Unlike the base Sexy::MenuWidget/MenuParser (which resolves images through the resource-pack
// index and ignores the lawn-area fan-out), this class overrides LoadMenuFile with a custom
// parser modeled on the PVZ-QEWide-Tweaks reference port: it loads the menu/VSSetupSides.txt
// script's widgets into custom VersusImageWidget/VersusLabelWidget classes (alpha support for
// the controller/side-glow animation), loads every image straight from disk via
// gSexyAppBase->GetSharedImage, shifts every Resize by BOARD_ADDITIONAL_WIDTH / BOARD_OFFSET_Y
// (the .txt coordinates are relative to the lawn, not the screen), and localizes bracketed
// labels through PvzpStringTranslate.
enum VSSetupState
{
	VSSETUP_NONE = -1,              // pre-enter sentinel (the ctor sets mState = -1)
	VSSETUP_CONTROLLER_SETUP = 0,   // waiting for player two to confirm (WaitForSecondPlayerDialog)
	VSSETUP_SIDES = 1,              // VSSetupSides.txt: both players pick their lawn/side
	VSSETUP_MODE_SELECT = 2,        // pick how to fill the deck (quick/custom/random)
	VSSETUP_SEED_CHOOSER = 3        // both sides pick seeds, then the match starts
};

// Mirrors VSSetupMenu::ButtonType in the decompiled source: ids 9 (quick), 10 (custom),
// 11 (random) are the three LawnButtonWidgets of VSSetupSides.txt (DefineWidgetIds order
// puts QUICK_BUTTON at 9, CUSTOM_BUTTON at 10, RANDOM_BUTTON at 11).
enum VSSetupButton
{
	VSSETUP_BUTTON_QUICK = 9,
	VSSETUP_BUTTON_CUSTOM = 10,
	VSSETUP_BUTTON_RANDOM = 11
};

// The ported VSSetupMenu. Despite the "Versus" name it is also reached for co-op (the two
// modes share the setup screen; the side/lane choice is what differs), exactly as the console
// build routes both local multiplayer modes through VSSetupMenu.
class VersusSetupMenu : public Sexy::MenuWidget
{
public:
	LawnApp* mApp;

	// --- Faithful VSSetupMenu state (offsets 292..332 in the decompiled class) ---
	VSSetupState mState = VSSETUP_NONE;
	std::array<int, 2> mControllerIndex = { -1, -1 }; // per-side gamepad index (PC: keyboard id)
	std::array<MultiplayerSide, NUM_MP_SIDES> mSides = { MP_SIDE_NONE, MP_SIDE_NONE };
	bool mPlayer0Confirmed = false;                    // byte 316
	bool mPlayer1Confirmed = false;                    // byte 317
	int mSeedPickTurn = 0;                             // whose chooser turn it is
	int mSeedPickAge = 0;                              // per-turn pick counter (byte 324)
	VersusRole mMode = VERSUS_ROLE_PLANTS;             // quick/custom/random selection
	bool mQuickPlay = false;                           // byte 332: quick-play shortcut
	int mAnimCounter = 0;                              // byte 324 alt / draw anim counter
	int mModeFocusId = VSSETUP_BUTTON_QUICK;           // which deck-fill button the arrow keys point at

public:
	explicit VersusSetupMenu(LawnApp* theApp);
	~VersusSetupMenu() override;

	// Faithful state machine (VSSetupMenu::GoToState / OnStateEnter / OnStateExit).
	void GoToState(VSSetupState theState);
	void OnStateEnter(VSSetupState theState, VSSetupState theOldState);
	void OnStateExit(VSSetupState theState, VSSetupState theNewState);

	// Faithful helpers.
	void SetSecondPlayerIndex(int theGamepadIndex);
	void CloseVSSetup(bool theAccepted);
	void OnPlayerPickedSeed(int theGamepadIndex);
	void PreFillVersusPlantBank(const std::vector<SeedType>& theDeck);
	void PreFillVersusZombieBank(const std::vector<SeedType>& theDeck);
	void PickRandomPlants(std::vector<SeedType>& thePlants, const std::vector<SeedType>& theZombies);
	void PickRandomZombies(std::vector<SeedType>& theZombies);
	// Remapped gamepad-button handler: button 2=left, 3=right, 6=A (confirm), 7=B (back),
	// per player number, exactly VSSetupMenu::GameButtonDown's skeleton.
	void GameButtonDown(int theButton, int thePlayer);

	// Faithful static data (VSSetupMenu::msRandomPools[72], msQuickPlayDecks[2][6]).
	static constexpr std::array<int, 72> msRandomPools = {
		0,  7, 32, 34,  7,  5, -1, 0,
		3,  2,  4, 20, 17, -1,  0, 0,
		6, 11, 18, 21, 29, 39, 36, -1,
		8, 10, 13,  0, 11, -1,  0, 0,
		3, 12, 14, 15, 20, 17, -1, 0,
		11, 21, 36, -1,  0,  0,  0, 0,
		62, 63, 64, 77, 68, -1,  0, 0,
		63, 66, 69, 65, 70, 76, 74, -1,
		72, 78, 79, 67, 75, 71, -1, 0
	};
	static const std::array<std::array<SeedType, 6>, 2>& QuickPlayDecks();
	static int msNextFirstPick; // which player picks first in the seed chooser (toggled by binary)

	// Input/UI overrides.
	void Update() override;
	void KeyDown(Sexy::KeyCode theKey) override;
	void OnMenuButtonDepress(int theId) override;
	void Draw(Sexy::Graphics* g) override;
	void AddedToManager(Sexy::WidgetManager* theManager) override;
	void RemovedFromManager(Sexy::WidgetManager* theManager) override;

private:
	// Custom VSSetupSides.txt loader (see the class comment). Shadows MenuWidget::LoadMenuFile,
	// which is not virtual -- this class never routes through the generic MenuParser.
	void LoadMenuFile(const std::string& theSource);
	std::string ReadMenuScriptFromResources(const std::string& theRelativePath);

	// The custom loader registers its widgets in mWidgetsByPspId (script id -> widget),
	// unlike the generic MenuParser, so state transitions look them up from there.
	Sexy::Widget* GetWidgetById(int theId) const
	{
		if (theId < 0 || theId >= kMaxMenuWidgets)
			return nullptr;
		return mWidgetsByPspId[theId];
	}

	// Moves the deck-fill button selection (ids 9..11) and refocuses it.
	void SetModeFocus(int theButtonId);

	// --- Custom-parser state (PVZ-QEWide-Tweaks VSSetupMenu::mWidgetsByPspId etc.) ---
	enum { kMaxMenuWidgets = 12 };
	Sexy::Widget* mWidgetsByPspId[kMaxMenuWidgets] = {};      // script widget id -> widget
	std::map<std::string, int, std::less<>> mSymbolTable;     // DefineWidgetIds/Enum/Define names
	std::vector<Sexy::Widget*> mCreatedWidgets;               // owned children (deleted by us)
};

#endif // __VERSUSSETUPMENU_H__