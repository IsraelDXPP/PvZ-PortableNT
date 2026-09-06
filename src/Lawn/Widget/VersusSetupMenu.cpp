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
#include "../Cutscene.h"
#include "../SeedPacket.h"
#include "../Challenge.h"
#include "widget/Dialog.h"
#include "widget/ButtonWidget.h"
#include "widget/LabelWidget.h"
#include "widget/ImageWidget.h"
#include "widget/WidgetManager.h"
#include "Common.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "graphics/SharedImage.h"
#include "PvzpLib/PvzpDebug.h"
#include "PvzpLib/PvzpStringFile.h"
#include "misc/KeyCodes.h"
#include "GameConstants.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace Sexy;

// The console/Android TV build's VSSetupMenu is a gamepad state machine driven by the
// extracted VSSetupMenu/VSSetupSides/VSSetupControllers.txt scripts. The original renders
// VSSetupSides.txt (the side-pick + deck-fill screen) through the menu system, resolves its
// images through the resource index, and fans the widgets out across the screen.
//
// This port reconstructs the state machine 1:1 but, exactly like the PVZ-QEWide-Tweaks
// reference port, overrides the loader/rendering of VSSetupSides.txt so the screen is laid
// out correctly on the PC: every image is loaded straight from disk via GetSharedImage
// (the .txt coordinates are lawn-relative, so each Resize is shifted by BOARD_ADDITIONAL_WIDTH
// / BOARD_OFFSET_Y), the alpha-capable versus widgets pick the plant/zombie side and their
// two gamepad indicators up, and bracketed labels are re-resolved with PvzpStringTranslate.
//
// - controller setup : the console pops a WaitForSecondPlayerDialog and waits for a second
//                      gamepad; on PC player two is the keyboard, so we promote it instantly.
// - sides            : both players pick a side with the arrow keys (p1) / WASD (p2), then a
//                      deck-fill button: Quick (9), Custom (10), Random (11).
// - mode select      : the three deck-fill buttons (ids 9..11) are live.
// - seed chooser     : Custom opens the port's single SeedChooserScreen; Quick/Random prefill
//                      the banks from the faithful deck data and jump to the match.
//
// Input is remapped from gamepad buttons to PC keys (see GameButtonDown / KeyDown):
//   gamepad A (confirm)   -> Enter / mouse click (p1), keyboard (p2)
//   gamepad B (back)      -> Escape
//   gamepad left / right  -> arrow keys (p1), A / D (p2)
//   gamepad up / down     -> arrow up/down (p1), W / S (p2)

namespace
{
	// VSSetupMenu::msQuickPlayDecks[2][6] -- the fixed plant (index 0) and zombie (index 1)
	// decks used by Quick Play, copied verbatim from the decompiled console build's data
	// section: {1,0,3,4,20,17} plants / {61,62,63,64,70,67} zombies.
	const std::array<std::array<SeedType, 6>, 2>& kQuickPlayDecks()
	{
		static const std::array<std::array<SeedType, 6>, 2> decks = { {
			{ (SeedType)1, (SeedType)0, (SeedType)3, (SeedType)4, (SeedType)20, (SeedType)17 },
			{ (SeedType)61, (SeedType)62, (SeedType)63, (SeedType)64, (SeedType)70, (SeedType)67 }
		} };
		return decks;
	}

	// The three horizontal positions the two gamepad indicators glide between once a side is
	// chosen (plant, unassigned-mid, zombie). These are the reference port's exact values
	// (QEWide GAMEPAD_X_POSITIONS, board-offset translated); the widgets' .txt anchor
	// (x=325) is the mid slot, so no entry drift occurs.
	constexpr int GAMEPAD_X_POSITIONS[3] = {
		240 + BOARD_ADDITIONAL_WIDTH,
		325 + BOARD_ADDITIONAL_WIDTH,
		410 + BOARD_ADDITIONAL_WIDTH,
	};

	// --- VSSetupSides.txt tokenizer (1:1 with the reference port, tolerant of the quoted
	// comma-list 'Resize' statement left over at the end of the original .txt) ---

	std::string TrimStr(const std::string& s)
	{
		size_t aStart = s.find_first_not_of(" \t\r\n");
		if (aStart == std::string::npos)
			return std::string();
		size_t anEnd = s.find_last_not_of(" \t\r\n");
		return s.substr(aStart, anEnd - aStart + 1);
	}

	bool IsDelimChar(char c)
	{
		return c == ' ' || c == '\t' || c == '(' || c == ')' || c == ',' || c == ';';
	}

	std::vector<std::string> TokenizeLine(const std::string& theLine)
	{
		std::vector<std::string> aTokens;
		std::string aCur;
		bool aInQuote = false;
		for (size_t i = 0; i < theLine.size(); i++)
		{
			char c = theLine[i];
			if (c == '\'')
			{
				aInQuote = !aInQuote;
				if (!aInQuote)
				{
					aTokens.push_back(aCur);
					aCur.clear();
				}
				continue;
			}
			if (!aInQuote && IsDelimChar(c))
			{
				if (!aCur.empty())
				{
					aTokens.push_back(aCur);
					aCur.clear();
				}
				continue;
			}
			aCur += c;
		}
		if (!aCur.empty())
			aTokens.push_back(aCur);
		return aTokens;
	}
}

const std::array<std::array<SeedType, 6>, 2>& VersusSetupMenu::QuickPlayDecks()
{
	return kQuickPlayDecks();
}

int VersusSetupMenu::msNextFirstPick = 1;

VersusSetupMenu::VersusSetupMenu(LawnApp* theApp) :
	mApp(theApp)
{
	mState = VSSETUP_NONE;
	mControllerIndex = { -1, -1 };
	mSides = { MP_SIDE_NONE, MP_SIDE_NONE };
	mPlayer0Confirmed = false;
	mPlayer1Confirmed = false;
	mSeedPickTurn = 0;
	mSeedPickAge = 0;
	mMode = VERSUS_ROLE_PLANTS;
	mQuickPlay = false;
	mAnimCounter = 0;
	mModeFocusId = VSSETUP_BUTTON_QUICK;
}

VersusSetupMenu::~VersusSetupMenu()
{
	for (size_t i = 0; i < mCreatedWidgets.size(); i++)
		delete mCreatedWidgets[i];
	mCreatedWidgets.clear();
}

// Reads the raw text of a loose .txt file from the app's resource folder (the same folder
// the port resolves main.pak and properties/resources.xml against).
std::string VersusSetupMenu::ReadMenuScriptFromResources(const std::string& theRelativePath)
{
	std::string aPath = Sexy::GetResourcePath(theRelativePath);
	std::ifstream aFileStream(Sexy::PathFromU8(aPath), std::ios::binary);
	if (!aFileStream)
		return std::string();
	std::ostringstream aStream;
	aStream << aFileStream.rdbuf();
	return aStream.str();
}

// ============================================================================
// Alpha-capable versus widgets (1:1 rendering approach with the reference port)
// ============================================================================

class VersusImageWidget : public Widget
{
public:
	SharedImageRef mImage;
	SharedImageRef mOverImage;
	int mAlpha;
	std::string mImagePath;

	explicit VersusImageWidget() : mAlpha(255)
	{
		mClip = false;
	}

	void Draw(Graphics* g) override
	{
		Image* aImage = (mIsOver && mOverImage.mSharedImage && mOverImage.mSharedImage->mImage)
			? (Image*)mOverImage
			: (Image*)mImage;
		if (aImage)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mAlpha));
			g->DrawImage(aImage, 0, 0);
		}
	}
};

class VersusLabelWidget : public Widget
{
public:
	_Font* mFont;
	std::string mText;
	int mAlign;
	Color mColor;

	VersusLabelWidget() : mFont(nullptr), mAlign(0), mColor(Color::White)
	{
		mClip = false;
	}

	~VersusLabelWidget() override
	{
		delete mFont;
	}

	void SetMenuFont(_Font* theFont)
	{
		delete mFont;
		mFont = (theFont != nullptr) ? theFont->Duplicate() : nullptr;
	}

	void Draw(Graphics* g) override
	{
		if (!mFont || mText.empty())
			return;
		g->SetFont(mFont);
		g->SetColor(mColor);
		int aFontX = 0;
		if (mAlign == 1)
			aFontX = -mFont->StringWidth(mText) / 2;
		g->DrawString(mText, aFontX, 0);
	}
};

// ============================================================================
// Custom VSSetupSides.txt loader (PVZ-QEWide-Tweaks VSSetupMenu::LoadMenuFile)
// ============================================================================

void VersusSetupMenu::LoadMenuFile(const std::string& theSource)
{
	// Parse the script into semicolon-delimited statement token lists (comments stripped).
	struct RawStatement
	{
		std::string cmd;
		std::vector<std::string> args;
	};

	std::vector<RawStatement> aStatements;
	{
		std::string aScript = theSource;
		std::string aFull;
		{
			std::istringstream aLineStream(aScript);
			std::string aLine;
			while (std::getline(aLineStream, aLine))
			{
				size_t aHash = aLine.find('#');
				if (aHash != std::string::npos)
					aLine.resize(aHash);
				aFull += aLine;
				aFull += ' ';
			}
		}

		std::string aStmt;
		for (size_t i = 0; i <= aFull.size(); i++)
		{
			char c = (i < aFull.size()) ? aFull[i] : ';';
			if (c == ';')
			{
				std::string aT = TrimStr(aStmt);
				if (!aT.empty())
				{
					auto aToks = TokenizeLine(aT);
					if (!aToks.empty())
					{
						aStatements.push_back(RawStatement{ aToks[0],
							std::vector<std::string>(aToks.begin() + 1, aToks.end()) });
					}
				}
				aStmt.clear();
			}
			else
				aStmt += c;
		}
	}

	// Reset scope.
	mSymbolTable.clear();
	mCreatedWidgets.clear();
	memset(mWidgetsByPspId, 0, sizeof(mWidgetsByPspId));
	int aNextEnumVal = 0;
	Widget* aCurrentWidget = nullptr;

	auto ResolveSymbol = [&](const std::string& s) -> int {
		auto anItr = mSymbolTable.find(s);
		if (anItr != mSymbolTable.end())
			return anItr->second;
		return std::stoi(s);
	};

	auto LoadImageByName = [](const std::string& name) -> SharedImageRef {
		if (name.empty())
			return SharedImageRef();
		// Prefer the resource manager (the pathway the base MenuParser used and that the
		// side/controller/background art relies on): it re-runs DoLoadImage when a resource
		// is stale, so a previously attempted-and-failed entry still resolves here. Falls
		// back to a straight disk load (GetSharedImage) for names that are not registered
		// in the resource index.
		if (gSexyAppBase->mResourceManager)
		{
			SharedImageRef aRes = gSexyAppBase->mResourceManager->GetImage(name);
			if (aRes.mSharedImage && aRes.mSharedImage->mImage)
				return aRes;
		}
		return gSexyAppBase->GetSharedImage(name);
	};

	auto LoadFontByName = [](const std::string& name) -> _Font* {
		if (name.empty())
			return nullptr;
		if (gSexyAppBase->mResourceManager)
			return gSexyAppBase->mResourceManager->LoadFont(name);
		return nullptr;
	};

	for (const auto& aStmt : aStatements)
	{
		const std::string& aCmd = aStmt.cmd;
		const std::vector<std::string>& aArgs = aStmt.args;

		if (aCmd == "DefineWidgetIds")
		{
			for (size_t i = 0; i < aArgs.size(); i++)
				mSymbolTable[aArgs[i]] = (int)i;
		}
		else if (aCmd == "Enum")
		{
			for (const auto& aName : aArgs)
				mSymbolTable[aName] = aNextEnumVal++;
		}
		else if (aCmd == "Define")
		{
			if (aArgs.size() >= 2)
				mSymbolTable[aArgs[0]] = ResolveSymbol(aArgs[1]);
		}
		else if (aCmd == "AddWidget")
		{
			if (aArgs.size() < 2)
				continue;
			const std::string& aType = aArgs[0];
			int aPspId = ResolveSymbol(aArgs[1]);

			Widget* aW = nullptr;
			if (aType == "ImageWidget")
				aW = new VersusImageWidget();
			else if (aType == "LabelWidget")
				aW = new VersusLabelWidget();
			else if (aType == "LawnButtonWidget" || aType == "ButtonWidget")
				aW = new ButtonWidget(aPspId, this);
			else
				continue;

			if (aPspId >= 0 && aPspId < kMaxMenuWidgets)
			{
				if (mWidgetsByPspId[aPspId])
				{
					auto it = std::find(mCreatedWidgets.begin(), mCreatedWidgets.end(), mWidgetsByPspId[aPspId]);
					if (it != mCreatedWidgets.end())
						mCreatedWidgets.erase(it);
					delete mWidgetsByPspId[aPspId];
				}
				mWidgetsByPspId[aPspId] = aW;
			}
			aCurrentWidget = aW;
			mCreatedWidgets.push_back(aW);
		}
		else if (!aCurrentWidget)
		{
			continue;
		}
		else if (aCmd == "SetImage")
		{
			if (aArgs.size() >= 1)
			{
				auto* aImg = dynamic_cast<VersusImageWidget*>(aCurrentWidget);
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				if (aImg)
				{
					aImg->mImage = LoadImageByName(aArgs[0]);
					aImg->mImagePath = aArgs[0];
				}
				else if (aBtn)
					aBtn->mButtonImage = LoadImageByName(aArgs[0]);
			}
		}
		else if (aCmd == "SetOverImage")
		{
			if (aArgs.size() >= 1)
			{
				auto* aImg = dynamic_cast<VersusImageWidget*>(aCurrentWidget);
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				if (aImg)
					aImg->mOverImage = LoadImageByName(aArgs[0]);
				else if (aBtn)
					aBtn->mOverImage = LoadImageByName(aArgs[0]);
			}
		}
		else if (aCmd == "SetDownImage")
		{
			if (aArgs.size() >= 1)
			{
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				if (aBtn)
					aBtn->mDownImage = LoadImageByName(aArgs[0]);
			}
		}
		else if (aCmd == "Resize")
		{
			// Lawn-relative coordinates: shift by the lawn margins so the menu lands centered
			// in the widescreen playfield exactly as QEWide lays it out.
			if (aArgs.size() >= 4)
			{
				aCurrentWidget->Resize(
					ResolveSymbol(aArgs[0]) + BOARD_ADDITIONAL_WIDTH,
					ResolveSymbol(aArgs[1]) + BOARD_OFFSET_Y,
					ResolveSymbol(aArgs[2]),
					ResolveSymbol(aArgs[3]));
			}
		}
		else if (aCmd == "SetColor")
		{
			if (aArgs.size() < 4)
				continue;
			// The tokenizer drops the script's '(' / ')' / ',', so the color role is arg 0
			// and the RGB values are args 1..3.
			std::vector<int> aVals;
			for (size_t i = 1; i < aArgs.size() && aVals.size() < 3; i++)
				aVals.push_back(std::stoi(aArgs[i]));
			if (aVals.size() < 3)
				continue;
			Color aColor(aVals[0], aVals[1], aVals[2]);
			auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
			auto* aLbl = dynamic_cast<VersusLabelWidget*>(aCurrentWidget);
			if (aBtn)
			{
				int aBtnColorIdx = ButtonWidget::COLOR_LABEL;
				const std::string& aColorName = aArgs[0];
				if (aColorName == "COLOR_LABEL_HILITE")
					aBtnColorIdx = ButtonWidget::COLOR_LABEL_HILITE;
				else if (aColorName == "COLOR_DARK_OUTLINE")
					aBtnColorIdx = ButtonWidget::COLOR_DARK_OUTLINE;
				else if (aColorName == "COLOR_LIGHT_OUTLINE")
					aBtnColorIdx = ButtonWidget::COLOR_LIGHT_OUTLINE;
				else if (aColorName == "COLOR_MEDIUM_OUTLINE")
					aBtnColorIdx = ButtonWidget::COLOR_MEDIUM_OUTLINE;
				else if (aColorName == "COLOR_BKG")
					aBtnColorIdx = ButtonWidget::COLOR_BKG;
				if ((size_t)aBtnColorIdx >= aBtn->mColors.size())
					aBtn->mColors.resize(aBtnColorIdx + 1);
				aBtn->mColors[aBtnColorIdx] = aColor;
			}
			else if (aLbl)
				aLbl->mColor = aColor;
		}
		else if (aCmd == "SetLabel")
		{
			if (aArgs.size() >= 1)
			{
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				auto* aLbl = dynamic_cast<VersusLabelWidget*>(aCurrentWidget);
				if (aBtn)
					aBtn->mLabel = std::string(PvzpStringTranslate(aArgs[0]));
				else if (aLbl)
					aLbl->mText = std::string(PvzpStringTranslate(aArgs[0]));
			}
		}
		else if (aCmd == "SetLabelJustify")
		{
			if (aArgs.size() >= 1)
			{
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				if (aBtn)
				{
					int aJustify = ResolveSymbol(aArgs[0]);
					if (aJustify == 2)
						aJustify = ButtonWidget::BUTTON_LABEL_CENTER;
					aBtn->mLabelJustify = aJustify;
				}
			}
		}
		else if (aCmd == "SetFont")
		{
			if (aArgs.size() >= 1)
			{
				_Font* aFont = LoadFontByName(aArgs[0]);
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				auto* aLbl = dynamic_cast<VersusLabelWidget*>(aCurrentWidget);
				if (aBtn && aFont)
					aBtn->SetFont(aFont);
				else if (aLbl)
					aLbl->SetMenuFont(aFont);
			}
		}
		else if (aCmd == "SetAlign")
		{
			if (aArgs.size() >= 1)
			{
				auto* aLbl = dynamic_cast<VersusLabelWidget*>(aCurrentWidget);
				if (aLbl)
					aLbl->mAlign = ResolveSymbol(aArgs[0]);
			}
		}
		else if (aCmd == "SetVisible")
		{
			if (aArgs.size() >= 1)
				aCurrentWidget->mVisible = std::stoi(aArgs[0]) != 0;
		}
		else if (aCmd == "SetDisabled")
		{
			if (aArgs.size() >= 1)
			{
				auto* aBtn = dynamic_cast<ButtonWidget*>(aCurrentWidget);
				if (aBtn)
					aBtn->mDisabled = std::stoi(aArgs[0]) != 0;
			}
		}
		// SetBackground / SetInitialFocus / SetGameLinks / AddHelpButton / AddAnimator /
		// SetHelpVisible are consumed silently (the custom loader does not need them).
	}

	// Attach all created widgets to the menu.
	for (size_t i = 0; i < mCreatedWidgets.size(); i++)
		AddWidget(mCreatedWidgets[i]);
}

// ============================================================================
// State machine (faithful VSSetupMenu::GoToState / OnStateEnter / OnStateExit)
// ============================================================================

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
		// Binary parity: state 1 only resets the per-player confirmed flags.
		mPlayer0Confirmed = false;
		mPlayer1Confirmed = false;
		break;

	case VSSETUP_MODE_SELECT:
		// Enable the three deck-fill buttons (ids 9..11) and set focus to the button matching
		// the last mode used.
		for (int i = VSSETUP_BUTTON_QUICK; i <= VSSETUP_BUTTON_RANDOM; ++i)
		{
			Widget* aWidget = GetWidgetById(i);
			if (aWidget)
				aWidget->SetVisible(true);
		}
		{
			int aFocusId = VSSETUP_BUTTON_QUICK;
			int aModeIdx = static_cast<int>(mMode) - 1;
			if (aModeIdx >= 0 && aModeIdx <= 1)
			{
				static const int kModeToButton[2] = { VSSETUP_BUTTON_CUSTOM, VSSETUP_BUTTON_RANDOM };
				aFocusId = kModeToButton[aModeIdx];
			}
			mModeFocusId = aFocusId;
			Widget* aFocus = GetWidgetById(aFocusId);
			if (aFocus && mWidgetManager)
				mWidgetManager->SetFocus(aFocus);
		}
		break;

	case VSSETUP_SEED_CHOOSER:
		// Hide all image widgets (ids 0..8) and the three deck-fill buttons (ids 9..11).
		for (int i = 0; i < 9; ++i)
		{
			Widget* aWidget = GetWidgetById(i);
			if (aWidget)
				aWidget->SetVisible(false);
		}
		for (int i = VSSETUP_BUTTON_QUICK; i <= VSSETUP_BUTTON_RANDOM; ++i)
		{
			Widget* aWidget = GetWidgetById(i);
			if (aWidget)
				aWidget->SetVisible(false);
		}
		mSeedPickTurn = msNextFirstPick;
		break;

	default:
		break;
	}
}

void VersusSetupMenu::OnStateExit(VSSetupState theState, VSSetupState /*theNewState*/)
{
	if (theState == VSSETUP_SEED_CHOOSER)
	{
		// Restore all image widgets (ids 0..8) and show the three deck-fill buttons again.
		for (int i = 0; i < 9; ++i)
		{
			Widget* aWidget = GetWidgetById(i);
			if (aWidget)
				aWidget->SetVisible(true);
		}
		for (int i = VSSETUP_BUTTON_QUICK; i <= VSSETUP_BUTTON_RANDOM; ++i)
		{
			Widget* aWidget = GetWidgetById(i);
			if (aWidget)
				aWidget->SetVisible(true);
		}
	}
}

void VersusSetupMenu::SetSecondPlayerIndex(int theControllerIndex)
{
	PVZP_ASSERT(theControllerIndex != -1);
	mControllerIndex[1] = theControllerIndex;
	mApp->SetSecondPlayer(theControllerIndex);
}

void VersusSetupMenu::CloseVSSetup(bool theAccepted)
{
	if (theAccepted)
	{
		mApp->KillVersusSetupMenu();
		if (mApp->mBoard && mApp->mBoard->mCutScene)
			mApp->mBoard->mCutScene->EndSeedChooser();
	}
	else
	{
		mApp->KillVersusSetupMenu();
		if (mApp->mBoard && mApp->mBoard->mCutScene)
			mApp->mBoard->mCutScene->EndSeedChooser();
		if (mApp->mBoard)
			mApp->KillBoard();
		mApp->ShowGameSelector();
	}
}

void VersusSetupMenu::PreFillVersusPlantBank(const std::vector<SeedType>& theDeck)
{
	if (!mApp->mBoard || !mApp->mBoard->mSeedBank)
		return;
	int aCount = std::min<int>((int)theDeck.size(), mApp->mBoard->mSeedBank->mNumPackets);
	for (int i = 0; i < aCount; i++)
		mApp->mBoard->mSeedBank->mSeedPackets[i].SetPacketType(theDeck[i], SeedType::SEED_NONE);
}

void VersusSetupMenu::PreFillVersusZombieBank(const std::vector<SeedType>& theDeck)
{
	if (!mApp->mBoard || !mApp->mBoard->mSeedBank2)
		return;
	int aCount = std::min<int>((int)theDeck.size(), mApp->mBoard->mSeedBank2->mNumPackets);
	for (int i = 0; i < aCount; i++)
		mApp->mBoard->mSeedBank2->mSeedPackets[i].SetPacketType(theDeck[i], SeedType::SEED_NONE);
}

void VersusSetupMenu::SetModeFocus(int theButtonId)
{
	if (theButtonId < VSSETUP_BUTTON_QUICK || theButtonId > VSSETUP_BUTTON_RANDOM)
		return;
	mModeFocusId = theButtonId;
	Widget* aFocus = GetWidgetById(theButtonId);
	if (aFocus && mWidgetManager)
		mWidgetManager->SetFocus(aFocus);
}

// Remapped gamepad-button handler for the sides/deck-fill state machine. Player 1 is the
// number pad / arrow keys (see KeyDown), player 2 is WASD(/;/.) etc. Buttons: 2 = left side,
// 3 = right side, 6 = A (confirm), 7 = B (back). Mirrors VSSetupMenu::GameButtonDown's
// skeleton (ids 9/10/11 handled separately by ButtonListener depress above).
void VersusSetupMenu::GameButtonDown(int theButton, int thePlayer)
{
	if (thePlayer < 0 || thePlayer > 1)
		thePlayer = 0;

	switch (theButton)
	{
	case 2: // choose plant side
		if (mState == VSSETUP_MODE_SELECT)
		{
			// The deck-fill buttons are one row; left moves toward the first button.
			int aNext = mModeFocusId - 1;
			if (aNext < VSSETUP_BUTTON_QUICK)
				aNext = VSSETUP_BUTTON_RANDOM;
			SetModeFocus(aNext);
			break;
		}
		if (mState == VSSETUP_SIDES)
		{
			// QEWide slot cycling: step one slot toward the plant side. From the middle
			// you land on plants, from zombies you drop back to the middle, and on plants
			// you stay (mirrors the reference's -1 -> 0 / 1 -> -1 swaps). A confirmed
			// player is locked to their side until they back out (B).
			bool aLocked = (thePlayer == 0) ? mPlayer0Confirmed : mPlayer1Confirmed;
			if (aLocked)
				break;
			if (mSides[thePlayer] == MP_SIDE_NONE)
				mSides[thePlayer] = MP_SIDE_ONE;
			else if (mSides[thePlayer] == MP_SIDE_TWO)
				mSides[thePlayer] = MP_SIDE_NONE;
		}
		break;

	case 3: // choose zombie side
		if (mState == VSSETUP_MODE_SELECT)
		{
			int aNext = mModeFocusId + 1;
			if (aNext > VSSETUP_BUTTON_RANDOM)
				aNext = VSSETUP_BUTTON_QUICK;
			SetModeFocus(aNext);
			break;
		}
		if (mState == VSSETUP_SIDES)
		{
			// QEWide slot cycling: step one slot toward the zombie side. From the middle
			// you land on zombies, from plants you drop back to the middle, and on zombies
			// you stay (mirrors the reference's -1 -> 1 / 0 -> -1 swaps). A confirmed
			// player is locked to their side until they back out (B).
			bool aLocked = (thePlayer == 0) ? mPlayer0Confirmed : mPlayer1Confirmed;
			if (aLocked)
				break;
			if (mSides[thePlayer] == MP_SIDE_NONE)
				mSides[thePlayer] = MP_SIDE_TWO;
			else if (mSides[thePlayer] == MP_SIDE_ONE)
				mSides[thePlayer] = MP_SIDE_NONE;
		}
		break;

	case 6: // confirm (A)
		if (mState == VSSETUP_MODE_SELECT)
		{
			OnMenuButtonDepress(mModeFocusId);
			break;
		}
		if (mState == VSSETUP_SIDES)
		{
			if (mSides[thePlayer] == MP_SIDE_NONE)
				break; // must pick a side first
			// Opponent already locked onto the same side -> buzzer (mirrors the
			// reference's "mSides[other] == side && mPlayerReady[other]" test).
			int aOther = 1 - thePlayer;
			bool aOtherConfirmed = (aOther == 0) ? mPlayer0Confirmed : mPlayer1Confirmed;
			if (mSides[aOther] == mSides[thePlayer] && aOtherConfirmed)
			{
				gSexyAppBase->PlaySample(Sexy::SOUND_BUZZER);
				break;
			}
			if (thePlayer == 0)
				mPlayer0Confirmed = true;
			else
				mPlayer1Confirmed = true;
			if (mPlayer0Confirmed && mPlayer1Confirmed)
				GoToState(VSSETUP_MODE_SELECT);
		}
		break;

	case 7: // back (B)
		switch (mState)
		{
		case VSSETUP_SIDES:
			if (thePlayer == 0 && mPlayer0Confirmed)
				mPlayer0Confirmed = false;
			else if (thePlayer == 1 && mPlayer1Confirmed)
				mPlayer1Confirmed = false;
			else
			{
				CloseVSSetup(false);
			}
			break;

		case VSSETUP_MODE_SELECT:
			GoToState(VSSETUP_SIDES);
			break;

		default:
			CloseVSSetup(false);
			break;
		}
		break;

	default:
		break;
	}
}

// Pre-fill the deck helpers go through the SeedBank/SeedPacket calls above. These eval the
// mode buttons via the ButtonListener path (OnMenuButtonDepress).
void VersusSetupMenu::OnMenuButtonDepress(int theId)
{
	if (mState != VSSETUP_MODE_SELECT)
		return;

	if (theId == VSSETUP_BUTTON_QUICK)
	{
		gSexyAppBase->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = true;
		const auto& aDecks = kQuickPlayDecks();
		PreFillVersusPlantBank(std::vector<SeedType>(aDecks[0].begin(), aDecks[0].end()));
		PreFillVersusZombieBank(std::vector<SeedType>(aDecks[1].begin(), aDecks[1].end()));
		mApp->mVsSkipSeedChooser = true;
		mMode = VERSUS_ROLE_PLANTS;
		CloseVSSetup(true);
		return;
	}

	if (theId == VSSETUP_BUTTON_CUSTOM)
	{
		gSexyAppBase->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = false;
		mApp->ShowSeedChooserScreen();
		mMode = VERSUS_ROLE_PLANTS;
		GoToState(VSSETUP_SEED_CHOOSER);
		return;
	}

	if (theId == VSSETUP_BUTTON_RANDOM)
	{
		gSexyAppBase->PlaySample(Sexy::SOUND_TAP);
		mQuickPlay = true;
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
		mMode = VERSUS_ROLE_ZOMBIES;
		CloseVSSetup(true);
		return;
	}
}

void VersusSetupMenu::KeyDown(Sexy::KeyCode theKey)
{
	// Player 1: arrow keys navigate sides/mode; Enter confirms; Escape backs out. Player 2
	// uses WASD (see GameButtonDown for side mapping; SEED chooser handled separately).
	if (theKey == KEYCODE_ESCAPE)
	{
		GameButtonDown(7, 1);
		return;
	}
	if (mState == VSSETUP_CONTROLLER_SETUP)
	{
		SetSecondPlayerIndex(1);
		GoToState(VSSETUP_SIDES);
		return;
	}
	if (mState == VSSETUP_SIDES)
	{
		// Reference (QEWide) mapping: arrows = player 1 (UP confirm, DOWN back), WASD =
		// player 2 (W confirm, S back). SDL yields the ASCII/VK uppercase codes; accept
		// the lowercase codes too as a safety net for other key-routing paths.
		if (theKey == KEYCODE_LEFT)
			GameButtonDown(2, 0);
		else if (theKey == KEYCODE_RIGHT)
			GameButtonDown(3, 0);
		else if (theKey == KEYCODE_UP)
			GameButtonDown(6, 0);
		else if (theKey == KEYCODE_DOWN)
			GameButtonDown(7, 0);
		else if (theKey == (Sexy::KeyCode)'A' || theKey == (Sexy::KeyCode)'a')
			GameButtonDown(2, 1);
		else if (theKey == (Sexy::KeyCode)'D' || theKey == (Sexy::KeyCode)'d')
			GameButtonDown(3, 1);
		else if (theKey == (Sexy::KeyCode)'W' || theKey == (Sexy::KeyCode)'w')
			GameButtonDown(6, 1);
		else if (theKey == (Sexy::KeyCode)'S' || theKey == (Sexy::KeyCode)'s')
			GameButtonDown(7, 1);
		else
			Widget::KeyDown(theKey);
		return;
	}
	if (mState == VSSETUP_MODE_SELECT)
	{
		// Arrow keys (and WASD for player 2) sweep the QUICK/CUSTOM/RANDOM row; Enter/Space
		// confirm the highlighted button; the usual Escape path backs out.
		if (theKey == KEYCODE_LEFT || theKey == (Sexy::KeyCode)'A' || theKey == (Sexy::KeyCode)'a')
			GameButtonDown(2, theKey == KEYCODE_LEFT ? 0 : 1);
		else if (theKey == KEYCODE_RIGHT || theKey == (Sexy::KeyCode)'D' || theKey == (Sexy::KeyCode)'d')
			GameButtonDown(3, theKey == KEYCODE_RIGHT ? 0 : 1);
		else if (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE)
			GameButtonDown(6, 0);
		else if (theKey == (Sexy::KeyCode)'W' || theKey == (Sexy::KeyCode)'w'
			|| theKey == (Sexy::KeyCode)'S' || theKey == (Sexy::KeyCode)'s')
			GameButtonDown(6, 1);
		else
			Widget::KeyDown(theKey);
		return;
	}
	Widget::KeyDown(theKey);
}

void VersusSetupMenu::Update()
{
	if (mState == VSSETUP_NONE)
	{
		GoToState(VSSETUP_CONTROLLER_SETUP);
		return;
	}
	mAnimCounter++;

	// Animate the two gamepad indicators so they glide onto the side their player picked
	// (widgets 7 / 8), and fade the plant/zombie side glows (widgets 2 / 5) once a player
	// locks that side. Only during the sides screen -- the visuals belong to it.
	if (mState == VSSETUP_SIDES)
	{
		for (int i = 0; i < 2; i++)
		{
			Widget* aW = GetWidgetById(7 + i);
			auto* aImg = dynamic_cast<VersusImageWidget*>(aW);
			if (!aImg)
				continue;

			int aSideIdx = (mSides[i] == MP_SIDE_ONE) ? 0 : (mSides[i] == MP_SIDE_TWO) ? 2 : 1;
			int aTargetX = GAMEPAD_X_POSITIONS[aSideIdx];
			if (aImg->mX != aTargetX)
			{
				int aDiff = aTargetX - aImg->mX;
				int aStep = aDiff / 4;
				if (aDiff < 0 && aStep == 0)
					aStep = -1;
				aImg->mX += aStep;
			}

			bool aConfirm = (i == 0) ? mPlayer0Confirmed : mPlayer1Confirmed;
			aImg->mAlpha = aConfirm ? 0 : 255;
		}

		for (int i = 0; i < 2; i++)
		{
			MultiplayerSide aSide = (i == 0) ? MP_SIDE_ONE : MP_SIDE_TWO;
			bool aShow = (mSides[0] == aSide && mPlayer0Confirmed) || (mSides[1] == aSide && mPlayer1Confirmed);
			Widget* aGlow = GetWidgetById((aSide == MP_SIDE_ONE) ? 2 : 5);
			auto* aGlowImg = dynamic_cast<VersusImageWidget*>(aGlow);
			if (aGlowImg)
				aGlowImg->mAlpha = aShow ? 255 : 0;
		}
	}
}

void VersusSetupMenu::AddedToManager(Sexy::WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	std::string aScript = ReadMenuScriptFromResources("menu/VSSetupSides.txt");
	if (!aScript.empty())
		LoadMenuFile(aScript);
}

void VersusSetupMenu::RemovedFromManager(Sexy::WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	for (size_t i = 0; i < mCreatedWidgets.size(); i++)
	{
		if (mCreatedWidgets[i])
			RemoveWidget(mCreatedWidgets[i]);
	}
}

void VersusSetupMenu::Draw(Sexy::Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	if (aBoard && aBoard->mCutScene && aBoard->mCutScene->IsBeforePreloading())
		return;
	Widget::Draw(g);
}

// Faithful static-data helpers used by Random battle. msRandomPools is a flat array of
// 8-entry pools, each a -1-terminated list. VSSetupMenu::PickRandomZombies maps deck slot ->
// one pool via {0,0,0,2,1} over the 5 picks (pool base = 8*v4+47, entries at 8*v4+48);
// plants use {0,0,1,1,2} (pool base = 8*v6). Each pick is drawn from the pool's valid
// entries, confirmed via LawnApp::HasSeedType, never repeated.
static int ZombiePoolBase(int theSlot)
{
	static const int kBase[5] = { 47, 47, 47, 63, 55 };
	return kBase[theSlot % 5];
}

static int PlantPoolStart(int theSlot)
{
	static const int kStart[5] = { 0, 0, 8, 8, 16 };
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
	mSeedPickTurn = mSeedPickTurn == 0;
	mSeedPickAge = 0;
}