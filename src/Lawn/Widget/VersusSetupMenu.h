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
enum VSSetupState
{
	VSSETUP_NONE = -1,              // pre-enter sentinel (the ctor sets mState = -1)
	VSSETUP_CONTROLLER_SETUP = 0,   // waiting for player two to confirm (WaitForSecondPlayerDialog)
	VSSETUP_SIDES = 1,              // VSSetupSides.txt: both players pick their lawn/side
	VSSETUP_MODE_SELECT = 2,        // pick how to fill the deck (quick/custom/random)
	VSSETUP_SEED_CHOOSER = 3        // both sides pick seeds, then the match starts
};

// Mirrors VSSetupMenu::ButtonType in the decompiled source: ids 9 (quick), 10 (custom),
// 11 (random) are the three LawnButtonWidgets of VSSetupSides.txt.
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

public:
	explicit VersusSetupMenu(LawnApp* theApp);

	// Faithful state machine (VSSetupMenu::GoToState / OnStateEnter / OnStateExit).
	void GoToState(VSSetupState theState);
	void OnStateEnter(VSSetupState theState, VSSetupState theOldState);
	void OnStateExit(VSSetupState theState, VSSetupState theNewState);

	// Faithful helpers.
	void SetSecondPlayerIndex(int theGamepadIndex);
	void CloseVSSetup(bool theAccepted);
	void OnPlayerPickedSeed(int theGamepadIndex);
	void PickRandomPlants(std::vector<SeedType>& thePlants, const std::vector<SeedType>& theZombies);
	void PickRandomZombies(std::vector<SeedType>& theZombies);

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

	// Input/UI overrides.
	void Update() override;
	void KeyDown(Sexy::KeyCode theKey) override;
	void OnMenuButtonDepress(int theId) override;

private:
	void SetUpSidesScreen();
	void EnsureSeedChooser();
	void PreFillVersusPlantBank(const std::vector<SeedType>& theDeck);
	void PreFillVersusZombieBank(const std::vector<SeedType>& theDeck);
	// Reads a loose .txt menu script (e.g. menu/VSSetupSides.txt) straight from the app's
	// resource folder (where main.pak lives), the 1:1 console data source. Returns the
	// full script text on success, empty string on failure (mLastError explains why).
	std::string ReadMenuScriptFromResources(const std::string& theRelativePath);
};

#endif // __VERSUSSETUPMENU_H__
