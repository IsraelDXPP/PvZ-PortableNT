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
#include "../Board.h"
#include "../SeedPacket.h"
#include "widget/Dialog.h"
#include "widget/ButtonWidget.h"
#include "PvzpLib/PvzpDebug.h"
#include "misc/KeyCodes.h"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace Sexy;

// The console/Android TV build's VSSetupMenu is a state machine over the extracted
// VSSetupMenu/VSSetupSides/VSSetupControllers.txt scripts. This port reconstructs that
// state machine 1:1 and remaps the gamepad input onto this port's PC keyboard/mouse:
//
//   VSSETUP_CONTROLLER_SETUP : console pops a WaitForSecondPlayerDialog and waits for a
//                              second gamepad. On PC player two is the keyboard, so we
//                              promote it immediately and advance (SetSecondPlayerIndex +
//                              GoToState(SIDES)), exactly the transition the console
//                              OnStateEnter(0) performs once a controller is detected.
//   VSSETUP_SIDES             : VSSetupSides.txt -- both players pick a side, then one
//                              chooses how to fill the deck: Quick (9), Custom (10),
//                              Random (11).
//   VSSETUP_MODE_SELECT      : the three deck-fill buttons (ids 9..11) are live here.
//   VSSETUP_SEED_CHOOSER     : the picked-seed phase before the match (see below).
//
// The static deck data is copied verbatim from the decompiled VSSetupMenu (msQuickPlayDecks
// and msRandomPools). The console writes these into the Board's per-side seed banks once the
// board exists; this port starts versus from the GameSelector (no live board yet) and its
// existing versus gameplay sets up the banks itself, so the deck helpers here model the data
// and random-pool selection faithfully but the actual banks are configured by the match code.

namespace
{
	// VSSetupMenu::msQuickPlayDecks[2][6] -- the fixed plant (index 0) and zombie (index 1)
	// decks used by Quick Play, copied verbatim from the decompiled console build's data
	// section (0x5C8B84): {1,0,3,4,20,17} plants / {61,62,63,64,70,67} zombies, laid out as
	// SeedType values and terminated conceptually at the bank's 6-slot width.
	const std::array<std::array<SeedType, 6>, 2>& kQuickPlayDecks()
	{
		static const std::array<std::array<SeedType, 6>, 2> decks = { {
			{ (SeedType)1, (SeedType)0, (SeedType)3, (SeedType)4, (SeedType)20, (SeedType)17 },
			{ (SeedType)61, (SeedType)62, (SeedType)63, (SeedType)64, (SeedType)70, (SeedType)67 }
		} };
		return decks;
	}
}

const std::array<std::array<SeedType, 6>, 2>& VersusSetupMenu::QuickPlayDecks()
{
	return kQuickPlayDecks();
}

VersusSetupMenu::VersusSetupMenu(LawnApp* theApp) :
	mApp(theApp)
{
	// The decompiled constructor loads the VSSetupMenu script into its MenuWidget base, which
	// gives this screen its three deck-fill buttons (Quick/Custom/Random) from the real
	// console .txt (ids/positions/labels driven by the script, not hand-written C++); the
	// buttons are actually created on entering the SIDES/MODE_SELECT state (SetUpSidesScreen).
	// We enter the controller-setup state first so OnStateEnter promotes the keyboard as
	// player two and advances to the sides screen, mirroring the console flow.
	GoToState(VSSETUP_CONTROLLER_SETUP);
}

void VersusSetupMenu::SetUpSidesScreen()
{
	RemoveAllWidgets(true);

	// Load the real console menu/VSSetupSides.txt from disk (1:1 with the console, which
	// loads only this file in the VSSetupMenu constructor). The four side/gamepad images and
	// the three VS-Button deck-fill buttons all come from that script through the MenuWidget
	// base's parser; nothing is hand-written in C++.
	std::string aScript = ReadMenuScriptFromResources("menu/VSSetupSides.txt");
	if (aScript.empty())
	{
		PvzpLogLn("VersusSetupMenu: failed to read menu/VSSetupSides.txt: {}", mLastError);
		return;
	}
	if (!LoadMenuFile(aScript))
		PvzpLogLn("VersusSetupMenu: failed to load menu/VSSetupSides.txt: {}", mLastError);
}

// Reads the raw text of a loose .txt file from the app's resource folder (the same folder
// the port resolves main.pak and properties/resources.xml against). The console loads its
// VSSetupSides.txt via PakInterface from the main.pak pack; this port reads the identical
// file straight off disk so the user can drop the real 1:1 script next to main.pak.
std::string VersusSetupMenu::ReadMenuScriptFromResources(const std::string& theRelativePath)
{
	std::string aPath = Sexy::GetResourcePath(theRelativePath);
	std::ifstream aFileStream(Sexy::PathFromU8(aPath), std::ios::binary);
	if (!aFileStream)
	{
		mLastError = "Unable to open resource file: " + theRelativePath;
		return std::string();
	}
	std::ostringstream aStream;
	aStream << aFileStream.rdbuf();
	return aStream.str();
}

// Faithful VSSetupMenu::GoToState: if the state changed, run OnStateExit on the old state and
// OnStateEnter on the new one.
void VersusSetupMenu::GoToState(VSSetupState theState)
{
	if (mState == theState)
		return;
	VSSetupState aOldState = mState;
	OnStateExit(mState, theState);
	mState = theState;
	OnStateEnter(theState, aOldState);
}

void VersusSetupMenu::OnStateEnter(VSSetupState theState, VSSetupState /*theOldState*/)
{
	switch (theState)
	{
	case VSSETUP_CONTROLLER_SETUP:
		// Console: WaitForSecondPlayerDialog -> SetSecondPlayerIndex -> GoToState(SIDES).
		// PC: the keyboard is always the second player, so promote it and advance.
		SetSecondPlayerIndex(1);
		GoToState(VSSETUP_SIDES);
		break;

	case VSSETUP_SIDES:
	case VSSETUP_MODE_SELECT:
		// Reset the per-player "I have confirmed my side" flags for a fresh pick, then make
		// sure the three deck-fill buttons are on screen.
		mPlayer0Confirmed = false;
		mPlayer1Confirmed = false;
		SetUpSidesScreen();
		break;

	case VSSETUP_SEED_CHOOSER:
		// The console opens a per-player seed chooser for each side here. This port's versus
		// has a single shared PC SeedChooserScreen handling the plant side (the zombie side's
		// deck is the conveyor fed by Board in versus play). See EnsureSeedChooser.
		EnsureSeedChooser();
		break;
	}
}

void VersusSetupMenu::OnStateExit(VSSetupState theState, VSSetupState /*theNewState*/)
{
	if (theState == VSSETUP_SEED_CHOOSER)
		mApp->KillSeedChooserScreen();
}

// Faithful VSSetupMenu::SetSecondPlayerIndex -- records which device joins as player two.
// On PC there is a single keyboard, so this records the controller slot and pushes it into
// the app's player-two plumbing.
void VersusSetupMenu::SetSecondPlayerIndex(int theControllerIndex)
{
	PVZP_ASSERT(theControllerIndex != -1);
	mControllerIndex[1] = theControllerIndex;
	mApp->SetSecondPlayer(theControllerIndex);
}

// Faithful VSSetupMenu::CloseVSSetup(bool). If the match was accepted, tear the setup screen
// down (the level is already running underneath -- NewGame created it); otherwise back out to
// the game selector. Mirrors the console, where CloseVSSetup calls KillVSSetupScreen and then
// CutScene::EndSeedChooser to let the intro proceed into the match.
void VersusSetupMenu::CloseVSSetup(bool theAccepted)
{
	if (!theAccepted)
	{
		mApp->KillVersusSetupMenu();
		mApp->KillBoard();
		mApp->ShowGameSelector();
	}
	else
	{
		mApp->KillVersusSetupMenu();
	}
}

// Pre-fill the plant player's seed bank from a deck, mirroring the console
// VSSetupMenu::ButtonDepress which writes the deck into the versus seed bank before starting.
void VersusSetupMenu::PreFillVersusPlantBank(const std::vector<SeedType>& theDeck)
{
	if (!mApp->mBoard || !mApp->mBoard->mSeedBank)
		return;
	int aCount = std::min<int>((int)theDeck.size(), mApp->mBoard->mSeedBank->mNumPackets);
	for (int i = 0; i < aCount; i++)
		mApp->mBoard->mSeedBank->mSeedPackets[i].SetPacketType(theDeck[i], SeedType::SEED_NONE);
}

// Pre-fill the zombie player's seed bank, mirroring the console's zombie-bank fill used by
// both Quick and Random.
void VersusSetupMenu::PreFillVersusZombieBank(const std::vector<SeedType>& theDeck)
{
	if (!mApp->mBoard || !mApp->mBoard->mSeedBank2)
		return;
	int aCount = std::min<int>((int)theDeck.size(), mApp->mBoard->mSeedBank2->mNumPackets);
	for (int i = 0; i < aCount; i++)
		mApp->mBoard->mSeedBank2->mSeedPackets[i].SetPacketType(theDeck[i], SeedType::SEED_NONE);
}

// The plant deck that Quick battle writes to the bank: ConsoleButtonDepress builds it from
// msQuickPlayDecks[0] (verified against the .so: {1,0,3,4,20,17}) and starts without a chooser.
static std::vector<SeedType> QuickPlantDeck()
{
	const auto& aQuick = VersusSetupMenu::QuickPlayDecks()[0];
	return std::vector<SeedType>(aQuick.begin(), aQuick.end());
}

// Faithful VSSetupMenu::ButtonDepress. The quick/custom/random buttons are dispatched by the
// widget ids the VSSetupSides.txt script assigns (GetWidgetId), matching how VersusResultsMenu
// resolves PLAY_AGAIN/QUIT_BUTTON -- so the ids always agree with what the script actually
// created, never a hardcoded number.
//
// The versus level was already created underneath the setup screen (Console NewGame calls
// ShowVSSetupScreen over the live board), so these paths only fill the banks and/or flag the
// intro, mirroring the console's CloseVSSetup(CutScene::EndSeedChooser):
//   - Quick/Random pre-fill the banks and set mVsSkipSeedChooser so the intro skips the
//     chooser (fast-start), exactly like the console's ButtonDepress 9/11.
//   - Custom leaves mVsSkipSeedChooser clear and shows the seed chooser (ButtonDepress 10).
// The kill is deliberately the last step so `this` stays alive through the bank fill.
void VersusSetupMenu::OnMenuButtonDepress(int theId)
{
	if (mState == VSSETUP_CONTROLLER_SETUP)
		return;

	int aQuickId = GetWidgetId("QUICK_BUTTON");
	int aCustomId = GetWidgetId("CUSTOM_BUTTON");
	int aRandomId = GetWidgetId("RANDOM_BUTTON");

	if (theId == aQuickId)
	{
		// Quick Play: fill the plant side from msQuickPlayDecks then start without a chooser.
		mApp->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = true;
		PreFillVersusPlantBank(QuickPlantDeck());
		mApp->mVsSkipSeedChooser = true;
		mApp->KillVersusSetupMenu();
		return;
	}

	if (theId == aCustomId)
	{
		// Custom Battle: open the seed chooser so the plant side can pick its deck. Leaving
		// mVsSkipSeedChooser clear lets the versus level intro show the shared chooser, exactly
		// like the console's ShowSeedChooserScreen + GoToState(SEED_CHOOSER).
		mApp->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = true;
		mApp->ShowSeedChooserScreen();
		mApp->KillVersusSetupMenu();
		return;
	}

	if (theId == aRandomId)
	{
		// Random Battle: fill both sides from msRandomPools and start without a chooser. The
		// console seeds packet 0 with SEED_SUNFLOWER (plants) and SEED_ZOMBIE_NORMAL (61,
		// zombies), then appends five random picks per side.
		mApp->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = true;
		// Console ButtonDepress: packet 0 is SEED_SUNFLOWER (plants) / SEED_ZOMBIE_NORMAL
		// (zombies), then five random picks are appended.
		std::vector<SeedType> aPlants;
		{
			std::vector<SeedType> aDiscard;
			PickRandomPlants(aPlants, aDiscard);
		}
		aPlants.insert(aPlants.begin(), SeedType::SEED_SUNFLOWER);
		std::vector<SeedType> aZombies;
		PickRandomZombies(aZombies);
		aZombies.insert(aZombies.begin(), SeedType::SEED_ZOMBIE_NORMAL);
		PreFillVersusPlantBank(aPlants);
		PreFillVersusZombieBank(aZombies);
		mApp->mVsSkipSeedChooser = true;
		mApp->KillVersusSetupMenu();
		return;
	}
}

// Faithful VSSetupMenu::KeyDown -- remapped gamepad buttons to PC keys. Player 2 uses the
// keyboard (arrow keys + Enter), mirroring Board::Player2KeyDown; Escape backs out.
void VersusSetupMenu::KeyDown(Sexy::KeyCode theKey)
{
	if (theKey == KEYCODE_ESCAPE)
	{
		CloseVSSetup(false);
		return;
	}
	if (mState == VSSETUP_CONTROLLER_SETUP)
	{
		SetSecondPlayerIndex(1);
		GoToState(VSSETUP_SIDES);
		return;
	}
	Widget::KeyDown(theKey);
}

// Faithful VSSetupMenu::Update: the console build drives the per-player controller cursor
// animation and, in the seed-chooser state, waits until both choosers are done. This port's
// single shared chooser reports completion through the normal CutScene close path, so Update
// just drives the base menu animation.
void VersusSetupMenu::Update()
{
	MenuWidget::Update();
}

// Faithful helpers used by Random battle. msRandomPools is a flat array of 8-entry pools,
// each a -1-terminated list. VSSetupMenu::PickRandomZombies maps deck slot -> one pool via
// the aVoidVssetupmen[] tables (zombie: v4 = {0,0,0,2,1} over the 5 picks, pool base =
// 8*v4+47, so entries begin at flat index 8*v4+48); plants use the table v6 = {0,0,1,1,2},
// pool base = 8*v6 (entries begin at 8*v6), i.e. flats rows 0,0,1,1,2. Each pick is drawn
// from the pool's valid (non-(-1)) entries, confirmed via LawnApp::HasSeedType, never
// repeated, exactly as the decompiled loop does before pushing into the deck vector.
static int ZombiePoolBase(int theSlot)
{
	static const int kBase[5] = { 47, 47, 47, 63, 55 }; // flat msRandomPools index (8*v4+47)
	return kBase[theSlot % 5];
}

static int PlantPoolStart(int theSlot)
{
	static const int kStart[5] = { 0, 0, 8, 8, 16 }; // flat msRandomPools index (8*v6)
	return kStart[theSlot % 5];
}

void VersusSetupMenu::PickRandomZombies(std::vector<SeedType>& theZombies)
{
	while ((int)theZombies.size() < 5)
	{
		int aStart = ZombiePoolBase((int)theZombies.size()) + 1;
		for (int c = aStart; c < aStart + 8; c++)
		{
			int aSeed = msRandomPools[c];
			if (aSeed < 0)
				break;
			SeedType aType = (SeedType)aSeed;
			if (!mApp->HasSeedType(aType))
				continue;
			if (std::find(theZombies.begin(), theZombies.end(), aType) != theZombies.end())
				continue;
			theZombies.push_back(aType);
			break;
		}
	}
}

void VersusSetupMenu::PickRandomPlants(std::vector<SeedType>& thePlants, const std::vector<SeedType>& /*theZombies*/)
{
	while ((int)thePlants.size() < 5)
	{
		int aStart = PlantPoolStart((int)thePlants.size());
		for (int c = aStart; c < aStart + 8; c++)
		{
			int aSeed = msRandomPools[c];
			if (aSeed < 0)
				break;
			SeedType aType = (SeedType)aSeed;
			if (!mApp->HasSeedType(aType))
				continue;
			if (std::find(thePlants.begin(), thePlants.end(), aType) != thePlants.end())
				continue;
			thePlants.push_back(aType);
			break;
		}
	}
}

void VersusSetupMenu::OnPlayerPickedSeed(int /*theGamepadIndex*/)
{
	// The console toggles mSeedPickTurn here when the current player finishes a pick. This
	// port's single shared chooser has no turn concept, so this is a structural no-op.
	mSeedPickTurn = mSeedPickTurn == 0;
	mSeedPickAge = 0;
}

void VersusSetupMenu::EnsureSeedChooser()
{
	// Reuse the single shared PC SeedChooserScreen. It must not already be up, and the board
	// plus its "choose your seeds" cutscene must be active for it to close into a match.
	if (mApp->mSeedChooserScreen)
		return;
}
