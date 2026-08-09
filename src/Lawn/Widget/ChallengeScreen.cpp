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

#include "GameButton.h"
#include "../../LawnApp.h"
#include "../System/Music.h"
#include "ChallengeScreen.h"
#include "../../Resources.h"
#include "../ToolTipWidget.h"
#include "../System/PlayerInfo.h"
#include "../../PvzpLib/PvzpDebug.h"
#include "../../PvzpLib/PvzpFoley.h"
#include "../../PvzpLib/PvzpCommon.h"
#include "misc/Debug.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "widget/WidgetManager.h"
#include "widget/Slider.h"
#include "../../GameConstants.h"
#include <algorithm>
#include <cmath>

const Rect cChallengeRect = Rect(0, 91 + BOARD_OFFSET_Y, BOARD_WIDTH, 480);
const int cButtonHeight = 118;

constinit const ChallengeDefinition gChallengeDefs[NUM_CHALLENGE_MODES] = {
	{ GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1,              0,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    0,  0,  "[SURVIVAL_DAY_NORMAL]", true },
	{ GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_2,              1,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    1,  0,  "[SURVIVAL_NIGHT_NORMAL]", true },
	{ GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_3,              2,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    2,  0,  "[SURVIVAL_POOL_NORMAL]", true },
	{ GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_4,              3,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    3,  0,  "[SURVIVAL_FOG_NORMAL]", true },
	{ GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_5,              4,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    4,  0,  "[SURVIVAL_ROOF_NORMAL]", true },
	{ GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_1,                5,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    0,  1,  "[SURVIVAL_DAY_HARD]", true },
	{ GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_2,                6,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    1,  1,  "[SURVIVAL_NIGHT_HARD]", true },
	{ GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_3,                7,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    2,  1,  "[SURVIVAL_POOL_HARD]", true },
	{ GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_4,                8,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    3,  1,  "[SURVIVAL_FOG_HARD]", true },
	{ GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_5,                9,   ChallengePage::CHALLENGE_PAGE_SURVIVAL,    4,  1,  "[SURVIVAL_ROOF_HARD]", true },
	{ GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_1,             10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       0,  3,  "[SURVIVAL_DAY_ENDLESS]", false },
	{ GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_2,             10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       1,  3,  "[SURVIVAL_NIGHT_ENDLESS]", false },
	{ GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_3,             10,  ChallengePage::CHALLENGE_PAGE_SURVIVAL,    2,  2,  "[SURVIVAL_POOL_ENDLESS]", false },
	{ GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_4,             10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       2,  3,  "[SURVIVAL_FOG_ENDLESS]", false },
	{ GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_5,             10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       3,  3,  "[SURVIVAL_ROOF_ENDLESS]", false },
	{ GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS,               0,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   0,  0,  "[WAR_AND_PEAS]", true },
	{ GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING,            6,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   1,  0,  "[WALL_NUT_BOWLING]", true },
	{ GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE,               2,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   2,  0,  "[SLOT_MACHINE]", true },
	{ GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS,              3,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   3,  0,  "[ITS_RAINING_SEEDS]", true },
	{ GameMode::GAMEMODE_CHALLENGE_BEGHOULED,                  1,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  0,  "[BEGHOULED]", true },
	{ GameMode::GAMEMODE_CHALLENGE_INVISIGHOUL,                8,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   0,  1,  "[INVISIGHOUL]", true },
	{ GameMode::GAMEMODE_CHALLENGE_SEEING_STARS,               5,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   1,  1,  "[SEEING_STARS]", true },
	{ GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM,               7,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   2,  1,  "[ZOMBIQUARIUM]", true },
	{ GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST,            20,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   3,  1,  "[BEGHOULED_TWIST]", true },
	{ GameMode::GAMEMODE_CHALLENGE_LITTLE_TROUBLE,             12,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  1,  "[LITTLE_TROUBLE]", true },
	{ GameMode::GAMEMODE_CHALLENGE_PORTAL_COMBAT,              15,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   0,  2,  "[PORTAL_COMBAT]", true },
	{ GameMode::GAMEMODE_CHALLENGE_COLUMN,                     4,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   1,  2,  "[COLUMN_AS_YOU_SEE_EM]", true },
	{ GameMode::GAMEMODE_CHALLENGE_BOBSLED_BONANZA,            17,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   2,  2,  "[BOBSLED_BONANZA]", true },
	{ GameMode::GAMEMODE_CHALLENGE_SPEED,                      18,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   3,  2,  "[ZOMBIES_ON_SPEED]", true },
	{ GameMode::GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE,             16,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  2,  "[WHACK_A_ZOMBIE]", true },
	{ GameMode::GAMEMODE_CHALLENGE_LAST_STAND,                 21,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   0,  3,  "[LAST_STAND]", true },
	{ GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS_2,             0,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   1,  3,  "[WAR_AND_PEAS_2]", true },
	{ GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2,          6,   ChallengePage::CHALLENGE_PAGE_CHALLENGE,   2,  3,  "[WALL_NUT_BOWLING_EXTREME]", true },
	{ GameMode::GAMEMODE_CHALLENGE_POGO_PARTY,                 14,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   3,  3,  "[POGO_PARTY]", true },
	{ GameMode::GAMEMODE_CHALLENGE_FINAL_BOSS,                 19,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  3,  "[FINAL_BOSS]", true },
	{ GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT,      0,   ChallengePage::CHALLENGE_PAGE_LIMBO,       0,  0,  "[ART_CHALLENGE_WALL_NUT]", false },
	{ GameMode::GAMEMODE_CHALLENGE_SUNNY_DAY,                  1,   ChallengePage::CHALLENGE_PAGE_LIMBO,       1,  0,  "[SUNNY_DAY]", false },
	{ GameMode::GAMEMODE_CHALLENGE_RESODDED,                   2,   ChallengePage::CHALLENGE_PAGE_LIMBO,       2,  0,  "[UNSODDED]", false },
	{ GameMode::GAMEMODE_CHALLENGE_BIG_TIME,                   3,   ChallengePage::CHALLENGE_PAGE_LIMBO,       3,  0,  "[BIG_TIME]", false },
	{ GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER,    4,   ChallengePage::CHALLENGE_PAGE_LIMBO,       4,  0,  "[ART_CHALLENGE_SUNFLOWER]", false },
	{ GameMode::GAMEMODE_CHALLENGE_AIR_RAID,                   5,   ChallengePage::CHALLENGE_PAGE_LIMBO,       0,  1,  "[AIR_RAID]", false },
	{ GameMode::GAMEMODE_CHALLENGE_ICE,                        6,   ChallengePage::CHALLENGE_PAGE_LIMBO,       1,  1,  "[ICE_LEVEL]", false },
	{ GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN,                 7,   ChallengePage::CHALLENGE_PAGE_LIMBO,       2,  1,  "[ZEN_GARDEN]", false },
	{ GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY,               8,   ChallengePage::CHALLENGE_PAGE_LIMBO,       3,  1,  "[HIGH_GRAVITY]", false },
	{ GameMode::GAMEMODE_CHALLENGE_GRAVE_DANGER,               11,  ChallengePage::CHALLENGE_PAGE_LIMBO,       4,  1,  "[GRAVE_DANGER]", false },
	{ GameMode::GAMEMODE_CHALLENGE_SHOVEL,                     10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       0,  2,  "[CAN_YOU_DIG_IT]", false },
	{ GameMode::GAMEMODE_CHALLENGE_STORMY_NIGHT,               13,  ChallengePage::CHALLENGE_PAGE_LIMBO,       1,  2,  "[DARK_STORMY_NIGHT]", false },
	{ GameMode::GAMEMODE_CHALLENGE_BUNGEE_BLITZ,               9,   ChallengePage::CHALLENGE_PAGE_LIMBO,       2,  2,  "[BUNGEE_BLITZ]", false },
	{ GameMode::GAMEMODE_CHALLENGE_SQUIRREL,                   10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       0,  4,  "Squirrel", false },
	{ GameMode::GAMEMODE_TREE_OF_WISDOM,                       10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       4,  2,  "Tree Of Wisdom", false },
	{ GameMode::GAMEMODE_SCARY_POTTER_1,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      0,  0,  "[SCARY_POTTER_1]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_2,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      1,  0,  "[SCARY_POTTER_2]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_3,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      2,  0,  "[SCARY_POTTER_3]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_4,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      3,  0,  "[SCARY_POTTER_4]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_5,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      4,  0,  "[SCARY_POTTER_5]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_6,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      0,  1,  "[SCARY_POTTER_6]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_7,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      1,  1,  "[SCARY_POTTER_7]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_8,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      2,  1,  "[SCARY_POTTER_8]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_9,                       10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      3,  1,  "[SCARY_POTTER_9]", true },
	{ GameMode::GAMEMODE_SCARY_POTTER_ENDLESS,                 10,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      4,  1,  "[SCARY_POTTER_ENDLESS]", false },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_1,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      0,  2,  "[I_ZOMBIE_1]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_2,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      1,  2,  "[I_ZOMBIE_2]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_3,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      2,  2,  "[I_ZOMBIE_3]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_4,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      3,  2,  "[I_ZOMBIE_4]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_5,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      4,  2,  "[I_ZOMBIE_5]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_6,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      0,  3,  "[I_ZOMBIE_6]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_7,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      1,  3,  "[I_ZOMBIE_7]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_8,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      2,  3,  "[I_ZOMBIE_8]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_9,                    11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      3,  3,  "[I_ZOMBIE_9]", true },
	{ GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS,              11,  ChallengePage::CHALLENGE_PAGE_PUZZLE,      4,  3,  "[I_ZOMBIE_ENDLESS]", false },
	{ GameMode::GAMEMODE_UPSELL,                               10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       4,  3,  "[UPSELL]", false },
	{ GameMode::GAMEMODE_INTRO,                                10,  ChallengePage::CHALLENGE_PAGE_LIMBO,       3,  2,  "[INTRO]", false }
};

ChallengeScreen::ChallengeScreen(LawnApp* theApp, ChallengePage thePage)
{
	mLockShakeX = 0;
	mLockShakeY = 0;
	mScrollAmount = 0;
	mScrollPosition = 0;
	mMaxScrollPosition = 0;

	mPageIndex = thePage;
	mApp = theApp;
	mClip = false;
	mCheatEnableChallenges = false;
	mUnlockState = UNLOCK_OFF;
	mUnlockChallengeIndex = -1;
	mUnlockStateCounter = 0;
	mLoadedResourceNames.push_back("DelayLoad_ChallengeScreen");

	for (std::string& resource : mLoadedResourceNames)
		PvzpLoadResources(resource.c_str());

	mBackButton = MakeNewButton(ChallengeScreen::ChallengeScreen_Back, this, "[BACK_TO_MENU]", nullptr, Sexy::IMAGE_SEEDCHOOSER_BUTTON2,
		Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
	mBackButton->mTextDownOffsetX = 1;
	mBackButton->mTextDownOffsetY = 1;
	mBackButton->mColors[ButtonWidget::COLOR_LABEL] = Color(42, 42, 90);
	mBackButton->mColors[ButtonWidget::COLOR_LABEL_HILITE] = Color(42, 42, 90);
	mBackButton->Resize(18 + BOARD_OFFSET_X + 35, 568 + BOARD_OFFSET_Y, 111, 26);

	mChallengesButton = MakeNewButton(ChallengeScreen::ChallengeScreen_Selector, this, "[PAGE_SELECTION_BUTTON]", nullptr, Sexy::IMAGE_SEEDCHOOSER_BUTTON2,
		Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
	mChallengesButton->mTextDownOffsetX = 1;
	mChallengesButton->mTextDownOffsetY = 1;
	mChallengesButton->mColors[ButtonWidget::COLOR_LABEL] = Color(42, 42, 90);
	mChallengesButton->mColors[ButtonWidget::COLOR_LABEL_HILITE] = Color(42, 42, 90);
	int aWidth = 111;
	mChallengesButton->Resize(618 + BOARD_OFFSET_X + (aWidth / 2), 568 + BOARD_OFFSET_Y, aWidth, 26);

	for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++)
	{
		const ChallengeDefinition& aChlDef = GetChallengeDefinition(aChallengeMode);
		ButtonWidget* aChallengeButton = new ButtonWidget(ChallengeScreen::ChallengeScreen_Mode + aChallengeMode, this);
		mChallengeButtons[aChallengeMode] = aChallengeButton;
		aChallengeButton->mDoFinger = !MoreTrophiesNeeded(aChallengeMode);
		aChallengeButton->mDisabled = MoreTrophiesNeeded(aChallengeMode);
		aChallengeButton->mFrameNoDraw = true;
		aChallengeButton->Resize(0, 0, 104, cButtonHeight);
	}

	mToolTip = new ToolTipWidget();
	mToolTip->mCenter = true;
	mToolTip->mVisible = false;
	UpdateButtons();

	if (mApp->mGameMode != GAMEMODE_UPSELL || mApp->mGameScene != SCENE_LEVEL_INTRO)
		mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);

	if (mPageIndex == CHALLENGE_PAGE_SURVIVAL && mApp->mPlayerInfo->mHasNewSurvival)
	{
		SetUnlockChallengeIndex(mPageIndex, false);
		mApp->mPlayerInfo->mHasNewSurvival = false;
	}
	else if (mPageIndex == CHALLENGE_PAGE_CHALLENGE && mApp->mPlayerInfo->mHasNewMiniGame)
	{
		SetUnlockChallengeIndex(mPageIndex, false);
		mApp->mPlayerInfo->mHasNewMiniGame = false;
	}
	else if (mPageIndex == CHALLENGE_PAGE_PUZZLE)
	{
		if (mApp->mPlayerInfo->mHasNewScaryPotter)
		{
			SetUnlockChallengeIndex(mPageIndex, false);
			mApp->mPlayerInfo->mHasNewScaryPotter = false;
		}
		else if (mApp->mPlayerInfo->mHasNewIZombie)
		{
			SetUnlockChallengeIndex(mPageIndex, true);
			mApp->mPlayerInfo->mHasNewIZombie = false;
		}
	}

	mSlider = new Slider(IMAGE_OPTIONS_SLIDERSLOT_PLANT, IMAGE_OPTIONS_SLIDERKNOB_PLANT, 0, this);
	mSlider->SetValue(std::max(0.0f, std::min(mMaxScrollPosition, mScrollPosition)));
	mSlider->mHorizontal = false;
	mSlider->Resize(775 + BOARD_OFFSET_X + 19, cChallengeRect.mY, 20, cChallengeRect.mHeight);
	mSlider->mThumbOffsetX = -4;
}

void ChallengeScreen::SliderVal(int theId, double theVal)
{
	switch (theId)
	{
	case 0:
		mScrollPosition = theVal * mMaxScrollPosition;
		break;
	}
}

ChallengeScreen::~ChallengeScreen()
{
	delete mBackButton;
	delete mChallengesButton;
	for (ButtonWidget* aChallengeButton : mChallengeButtons) delete aChallengeButton;
	delete mToolTip;
	delete mSlider;
}

const ChallengeDefinition& GetChallengeDefinition(int theChallengeMode)
{
	PVZP_ASSERT(theChallengeMode >= 0 && theChallengeMode < NUM_CHALLENGE_MODES);

	const ChallengeDefinition& aDef = gChallengeDefs[theChallengeMode];
	(void)aDef; // Unused in Release mode
	PVZP_ASSERT(aDef.mChallengeMode == theChallengeMode + GAMEMODE_SURVIVAL_NORMAL_STAGE_1);

	return gChallengeDefs[theChallengeMode];
}

bool ChallengeScreen::IsScaryPotterLevel(GameMode theGameMode)
{
	return theGameMode >= GAMEMODE_SCARY_POTTER_1 && theGameMode <= GAMEMODE_SCARY_POTTER_ENDLESS;
}

bool ChallengeScreen::IsIZombieLevel(GameMode theGameMode)
{
	return theGameMode >= GAMEMODE_PUZZLE_I_ZOMBIE_1 && theGameMode <= GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS;
}

void ChallengeScreen::SetUnlockChallengeIndex(ChallengePage thePage, bool theIsIZombie)
{
	mUnlockState = UNLOCK_SHAKING;
	mUnlockStateCounter = 100;
	mUnlockChallengeIndex = 0;
	for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++)
	{
		const ChallengeDefinition& aDef = GetChallengeDefinition(aChallengeMode);
		if (aDef.mPage == thePage)
		{
			if (thePage != CHALLENGE_PAGE_PUZZLE || (!theIsIZombie && IsScaryPotterLevel(aDef.mChallengeMode)) || (theIsIZombie && IsIZombieLevel(aDef.mChallengeMode)))
			{
				if (AccomplishmentsNeeded(aChallengeMode) <= 0)
				{
					mUnlockChallengeIndex = aChallengeMode;
				}
			}
		}
	}
}

int ChallengeScreen::MoreTrophiesNeeded(int theChallengeIndex)
{
	const ChallengeDefinition& aDef = GetChallengeDefinition(theChallengeIndex);
	if (mApp->mGameMode == GAMEMODE_UPSELL && mApp->mGameScene == SCENE_LEVEL_INTRO)
	{
		return aDef.mChallengeMode == GAMEMODE_CHALLENGE_FINAL_BOSS ? 1 : 0;
	}

	if (mApp->IsTrialStageLocked())
	{
		if (mPageIndex == CHALLENGE_PAGE_PUZZLE && aDef.mChallengeMode >= GAMEMODE_SCARY_POTTER_4)
		{
			return aDef.mChallengeMode == GAMEMODE_SCARY_POTTER_4 ? 1 : 2;
		}
		else if (mPageIndex == CHALLENGE_PAGE_CHALLENGE && aDef.mChallengeMode >= GAMEMODE_CHALLENGE_RAINING_SEEDS)
		{
			return aDef.mChallengeMode == GAMEMODE_CHALLENGE_RAINING_SEEDS ? 1 : 2;
		}
		else if (mPageIndex == CHALLENGE_PAGE_SURVIVAL && aDef.mChallengeMode >= GAMEMODE_SURVIVAL_NORMAL_STAGE_4)
		{
			return aDef.mChallengeMode == GAMEMODE_SURVIVAL_NORMAL_STAGE_4 ? 1 : 2;
		}
	}

	if (aDef.mPage == CHALLENGE_PAGE_PUZZLE)
	{
		if (IsScaryPotterLevel(aDef.mChallengeMode))
		{
			int aLevelsCompleted = 0;
			for (const ChallengeDefinition& aSPDef : gChallengeDefs)
			{
				if (IsScaryPotterLevel(aSPDef.mChallengeMode) && mApp->HasBeatenChallenge(aSPDef.mChallengeMode))
				{
					aLevelsCompleted++;
				}
			}

			if (aDef.mChallengeMode < GAMEMODE_SCARY_POTTER_4 || mApp->HasFinishedAdventure() || aLevelsCompleted < 3)
			{
				return std::clamp(aDef.mChallengeMode - GAMEMODE_SCARY_POTTER_1 - aLevelsCompleted, 0, 9);
			}
			else
			{
				return aDef.mChallengeMode == GAMEMODE_SCARY_POTTER_4 ? 1 : 2;
			}
		}
		else if (IsIZombieLevel(aDef.mChallengeMode))
		{
			int aLevelsCompleted = 0;
			for (const ChallengeDefinition& aIZDef : gChallengeDefs)
			{
				if (IsIZombieLevel(aIZDef.mChallengeMode) && mApp->HasBeatenChallenge(aIZDef.mChallengeMode))
				{
					aLevelsCompleted++;
				}
			}

			if (aDef.mChallengeMode < GAMEMODE_PUZZLE_I_ZOMBIE_4 || mApp->HasFinishedAdventure() || aLevelsCompleted < 3)
			{
				return std::clamp(aDef.mChallengeMode - GAMEMODE_PUZZLE_I_ZOMBIE_1 - aLevelsCompleted, 0, 9);
			}
			else
			{
				return aDef.mChallengeMode == GAMEMODE_PUZZLE_I_ZOMBIE_4 ? 1 : 2;
			}
		}
	}
	else
	{
		int aIdxInPage = aDef.mCol * 5 + aDef.mRow;
		if ((aDef.mPage == CHALLENGE_PAGE_CHALLENGE || aDef.mPage == CHALLENGE_PAGE_SURVIVAL) && !mApp->HasFinishedAdventure())
		{
			return aIdxInPage < 3 ? 0 : aIdxInPage == 3 ? 1 : 2;
		}
		else
		{
			int aNumTrophies = mApp->GetNumTrophies(aDef.mPage);
			if (aDef.mPage == CHALLENGE_PAGE_LIMBO)
			{
				return 0;
			}
			if (mApp->IsSurvivalEndless(aDef.mChallengeMode))
			{
				return 10 - aNumTrophies;
			}
			if (aDef.mPage == CHALLENGE_PAGE_SURVIVAL || aDef.mPage == CHALLENGE_PAGE_CHALLENGE)
			{
				aNumTrophies += 3;
			}
			else
			{
				PVZP_ASSERT(false);
			}

			return aIdxInPage >= aNumTrophies ? aIdxInPage - aNumTrophies + 1 : 0;
		}
	}

	unreachable();
}

bool ChallengeScreen::ShowPageButtons()
{
	return mApp->mCheatKeys && mPageIndex != CHALLENGE_PAGE_SURVIVAL && mPageIndex != CHALLENGE_PAGE_PUZZLE;
}

void ChallengeScreen::UpdateButtons()
{
	for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++)
		mChallengeButtons[aChallengeMode]->mVisible = GetChallengeDefinition(aChallengeMode).mPage == mPageIndex;
}

int ChallengeScreen::AccomplishmentsNeeded(int theChallengeIndex)
{
	int aTrophiesNeeded = MoreTrophiesNeeded(theChallengeIndex);
	GameMode aGameMode = GetChallengeDefinition(theChallengeIndex).mChallengeMode;
	if (mApp->IsSurvivalEndless(aGameMode) && aTrophiesNeeded <= 3 && mApp->GetNumTrophies(CHALLENGE_PAGE_SURVIVAL) < 10 &&
		mApp->HasFinishedAdventure() && !mApp->IsTrialStageLocked()) aTrophiesNeeded = 1;
	return mCheatEnableChallenges ? 0 : aTrophiesNeeded;
}

void ChallengeScreen::DrawButton(Graphics* g, int theChallengeIndex)
{
	ButtonWidget* aChallengeButton = mChallengeButtons[theChallengeIndex];
	if (aChallengeButton->mVisible)
	{
		aChallengeButton->mMouseVisible = cChallengeRect.Contains(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY);
		const ChallengeDefinition& aDef = GetChallengeDefinition(theChallengeIndex);
		aChallengeButton->mX = 38 + aDef.mRow * 155 + BOARD_OFFSET_X + 19;
		mButtonYStartOffset = cChallengeRect.mY + (aDef.mPage == CHALLENGE_PAGE_SURVIVAL ? 34 : 2);
		mButtonYOffset = cButtonHeight + (aDef.mPage == CHALLENGE_PAGE_SURVIVAL ? 30 : 2);
		aChallengeButton->mY = mButtonYStartOffset + aDef.mCol * mButtonYOffset - mScrollPosition;
		int aPosX = aChallengeButton->mX;
		int aPosY = aChallengeButton->mY;
		if (aChallengeButton->mIsDown)
		{
			aPosX++;
			aPosY++;
		}

		if (AccomplishmentsNeeded(theChallengeIndex) <= 1)
		{
			if (aChallengeButton->mDisabled)
			{
				g->SetColor(Color(92, 92, 92));
				g->SetColorizeImages(true);
			}
			if (theChallengeIndex == mUnlockChallengeIndex)
			{
				if (mUnlockState == UNLOCK_SHAKING)
				{
					g->SetColor(Color(92, 92, 92));
				}
				else if (mUnlockState == UNLOCK_FADING)
				{
					int aColor = PvzpAnimateCurve(50, 25, mUnlockStateCounter, 92, 255, CURVE_LINEAR);
					g->SetColor(Color(aColor, aColor, aColor));
				}
				g->SetColorizeImages(true);
			}
			g->SetClipRect(cChallengeRect);
			g->SetScale(0.5f, 0.5f, aPosX + 13, aPosY + 4);
			if (mPageIndex == CHALLENGE_PAGE_SURVIVAL)
			{
				g->DrawImageCel(Sexy::IMAGE_SURVIVAL_THUMBNAILS, aPosX + 13, aPosY + 4, aDef.mChallengeIconIndex);
			}
			else
			{
				g->DrawImageCel(Sexy::IMAGE_CHALLENGE_THUMBNAILS, aPosX + 13, aPosY + 4, aDef.mChallengeIconIndex);
			}
			g->SetScale(1.0f, 1.0f, aPosX + 13, aPosY + 4);

			bool aHighLight = aChallengeButton->mIsOver && theChallengeIndex != mUnlockChallengeIndex;
			g->SetColorizeImages(false);
			g->DrawImage(aHighLight ? Sexy::IMAGE_CHALLENGE_WINDOW_HIGHLIGHT : Sexy::IMAGE_CHALLENGE_WINDOW, aPosX - 6, aPosY - 2);

			Color aTextColor = aHighLight ? Color(250, 40, 40) : Color(42, 42, 90);
			std::string aName = PvzpStringTranslate(aDef.mChallengeName);
			if (aChallengeButton->mDisabled || (theChallengeIndex == mUnlockChallengeIndex && mUnlockState == UNLOCK_SHAKING))
			{
				aName = "?";
			}
			PvzpDrawStringWrapped(g, aName, Rect(aPosX + 6, aPosY + 74, 94, 33), Sexy::FONT_BRIANNETOD12, aTextColor, DS_ALIGN_CENTER_VERTICAL_MIDDLE);

			uint32_t aRecord = mApp->mPlayerInfo->mChallengeRecords[theChallengeIndex];
			if (theChallengeIndex == mUnlockChallengeIndex)
			{
				Image* aLockImage = Sexy::IMAGE_LOCK;
				if (mUnlockState == UNLOCK_FADING)
				{
					aLockImage = Sexy::IMAGE_LOCK_OPEN;
					g->SetColor(Color(255, 255, 255, PvzpAnimateCurve(25, 0, mUnlockStateCounter, 255, 0, CURVE_LINEAR)));
					g->SetColorizeImages(true);
				}
				PvzpDrawImageScaledF(g, aLockImage, aPosX + 24 + mLockShakeX, aPosY + 9 + mLockShakeY, 0.7f, 0.7f);
				g->SetColorizeImages(false);
			}
			else if (aRecord > 0)
			{
				if (mApp->HasBeatenChallenge(aDef.mChallengeMode))
				{
					g->DrawImage(Sexy::IMAGE_MINIGAME_TROPHY, aPosX - 6, aPosY - 2);
				}
				else if (mApp->IsEndlessScaryPotter(aDef.mChallengeMode) || mApp->IsEndlessIZombie(aDef.mChallengeMode))
				{
					std::string aAchievement = PvzpReplaceNumberString("[LONGEST_STREAK]", "{STREAK}", aRecord);
					Rect aRect(aPosX, aPosY + 15, 96, 200);
					PvzpDrawStringWrapped(g, aAchievement, aRect, Sexy::FONT_CONTINUUMBOLD14OUTLINE, Color::White, DS_ALIGN_CENTER);
					PvzpDrawStringWrapped(g, aAchievement, aRect, Sexy::FONT_CONTINUUMBOLD14, Color(255, 0, 0), DS_ALIGN_CENTER);
				}
				else if (mApp->IsSurvivalEndless(aDef.mChallengeMode))
				{
					std::string aAchievement = mApp->Pluralize(aRecord, "[ONE_FLAG]", "[COUNT_FLAGS]");
					PvzpDrawString(g, aAchievement, aPosX + 48, aPosY + 48, Sexy::FONT_CONTINUUMBOLD14OUTLINE, Color::White, DS_ALIGN_CENTER);
					PvzpDrawString(g, aAchievement, aPosX + 48, aPosY + 48, Sexy::FONT_CONTINUUMBOLD14, Color(255, 0, 0), DS_ALIGN_CENTER);
				}
			}
			else if (aChallengeButton->mDisabled)
			{
				PvzpDrawImageScaledF(g, Sexy::IMAGE_LOCK, aPosX + 24, aPosY + 9, 0.7f, 0.7f);
			}
		}
		else
		{
			g->DrawImage(Sexy::IMAGE_CHALLENGE_BLANK, aPosX, aPosY);
		}
	}
}

void ChallengeScreen::Draw(Graphics* g)
{
	g->SetLinearBlend(true);
	g->DrawImage(Sexy::IMAGE_CHALLENGE_BACKGROUND, 0, 0);

	PvzpDrawString(g, GetPageTitle(mPageIndex), 400 + BOARD_OFFSET_X + 19, 58 + BOARD_OFFSET_Y, Sexy::FONT_HOUSEOFTERROR28, Color(220, 220, 220), DS_ALIGN_CENTER);

	int aTrophiesGot = mApp->GetNumTrophies(mPageIndex);
	int aTrophiesTotal = mApp->GetTotalTrophies(mPageIndex);
	PvzpDrawString(g, aTrophiesTotal > 0 ? StrFormat("%d/%d", aTrophiesGot, aTrophiesTotal) : PvzpStringTranslate("[TROPHY_NONE]"), 739 + BOARD_ADDITIONAL_WIDTH, 73 + BOARD_OFFSET_Y,
		Sexy::FONT_DWARVENTODCRAFT12, Color(255, 240, 0), DS_ALIGN_CENTER);
	PvzpDrawImageScaledF(g, Sexy::IMAGE_TROPHY, 718 + BOARD_ADDITIONAL_WIDTH, 26 + BOARD_OFFSET_Y, 0.5f, 0.5f);

	int aHighestColumn = 0;
	for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++)
	{
		DrawButton(g, aChallengeMode);
		const ChallengeDefinition& aDef = GetChallengeDefinition(aChallengeMode);
		if (aDef.mCol >= aHighestColumn && aDef.mPage == mPageIndex)
			aHighestColumn = aDef.mCol;
	}
	mMaxScrollPosition = std::max(0, (aHighestColumn * mButtonYOffset) + cButtonHeight + (mButtonYStartOffset - cChallengeRect.mY) - cChallengeRect.mHeight);

	mToolTip->Draw(g);
}

void ChallengeScreen::Update()
{
	Widget::Update();
	UpdateToolTip();
	mScrollPosition = std::clamp(mScrollPosition += mScrollAmount * (mBaseScrollSpeed + std::abs(mScrollAmount) * mScrollAccel), 0.0f, mMaxScrollPosition);
	mScrollAmount *= (1.0f - mScrollAccel);
	mSlider->SetValue(mMaxScrollPosition > 0.0f ? std::max(0.0f, std::min(mMaxScrollPosition, mScrollPosition)) / mMaxScrollPosition : 0.0f);
	mSlider->mVisible = mMaxScrollPosition != 0;

	if (mUnlockStateCounter > 0) mUnlockStateCounter--;
	if (mUnlockState == UNLOCK_SHAKING)
	{
		if (mUnlockStateCounter == 0)
		{
			mApp->PlayFoley(FOLEY_PAPER);
			mUnlockState = UNLOCK_FADING;
			mUnlockStateCounter = 50;
			mLockShakeX = 0;
			mLockShakeY = 0;
		}
		else
		{
			mLockShakeX = RandRangeFloat(-2, 2);
			mLockShakeY = RandRangeFloat(-2, 2);
		}
	}
	else if (mUnlockState == UNLOCK_FADING && mUnlockStateCounter == 0)
	{
		mUnlockState = UNLOCK_OFF;
		mUnlockStateCounter = 0;
		mUnlockChallengeIndex = -1;
	}

	MarkDirty();
}

void ChallengeScreen::MouseWheel(int theDelta)
{
	mScrollAmount -= mBaseScrollSpeed * theDelta;
	mScrollAmount -= mScrollAmount * mScrollAccel;
}

std::string ChallengeScreen::GetPageTitle(ChallengePage thePage)
{
	std::string aTitle = PvzpStringTranslate("[UNKNOWN_PAGE]");
	switch (thePage)
	{
		case ChallengePage::CHALLENGE_PAGE_CHALLENGE:
			aTitle = PvzpStringTranslate("[PICK_CHALLENGE]");
		break;
		case ChallengePage::CHALLENGE_PAGE_PUZZLE:
			aTitle = PvzpStringTranslate("[SCARY_POTTER]");
			break;
		case ChallengePage::CHALLENGE_PAGE_SURVIVAL:
			aTitle = PvzpStringTranslate("[PICK_AREA]");
		break;
		case ChallengePage::CHALLENGE_PAGE_LIMBO:
			aTitle = PvzpStringTranslate("[LIMBO_PAGE]");
		break;
	}
	return aTitle;
}

bool ChallengeScreen::IsPageUnlocked(ChallengePage thePage)
{
	switch (thePage)
	{
		case ChallengePage::CHALLENGE_PAGE_CHALLENGE:
			return mApp->HasFinishedAdventure() || mApp->mPlayerInfo->mHasUnlockedMinigames;
		case ChallengePage::CHALLENGE_PAGE_PUZZLE:
			return mApp->HasFinishedAdventure() || mApp->mPlayerInfo->mHasUnlockedPuzzleMode;
		case ChallengePage::CHALLENGE_PAGE_SURVIVAL:
			return mApp->HasFinishedAdventure() || mApp->mPlayerInfo->mHasUnlockedSurvivalMode;
		case ChallengePage::CHALLENGE_PAGE_LIMBO:
			return mApp->HasFinishedAdventure();
	}
	return false;
}

void ChallengeScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	AddWidget(mBackButton);
	if (HAS_PAGE_SELECTOR)
		AddWidget(mChallengesButton);
	for (ButtonWidget* aButton : mChallengeButtons) AddWidget(aButton);
	AddWidget(mSlider);
}

void ChallengeScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveWidget(mBackButton);
	if (HAS_PAGE_SELECTOR)
		RemoveWidget(mChallengesButton);
	for (ButtonWidget* aButton : mChallengeButtons) RemoveWidget(aButton);
	RemoveWidget(mSlider);
}

void ChallengeScreen::ButtonPress(int theId)
{
	(void)theId;
	mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
}

void ChallengeScreen::ButtonDepress(int theId)
{
	if (theId == ChallengeScreen::ChallengeScreen_Back)
	{
		mApp->KillChallengeScreen();
		mApp->DoBackToMain();
	}
	else if (theId == ChallengeScreen::ChallengeScreen_Selector)
	{
		mApp->DoChallengePagesDialog();
	}

	int aChallengeMode = theId - ChallengeScreen::ChallengeScreen_Mode;
	if (aChallengeMode >= 0 && aChallengeMode < NUM_CHALLENGE_MODES)
	{
		mApp->KillChallengeScreen();
		mApp->PreNewGame((GameMode)(aChallengeMode + 1), true);
	}
}

void ChallengeScreen::UpdateToolTip()
{
	if (!mApp->mWidgetManager->mMouseIn || !mApp->mActive)
	{
		mToolTip->mVisible = false;
		return;
	}

	for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++)
	{
		const ChallengeDefinition& aDef = GetChallengeDefinition(aChallengeMode);
		ButtonWidget* aChallengeButton = mChallengeButtons[aChallengeMode];
		if (aChallengeButton->mVisible && aChallengeButton->mDisabled &&
			aChallengeButton->Contains(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY) &&
			AccomplishmentsNeeded(aChallengeMode) <= 1)
		{
			mToolTip->mX = aChallengeButton->mWidth / 2 + aChallengeButton->mX;
			mToolTip->mY = aChallengeButton->mY;
			if (MoreTrophiesNeeded(aChallengeMode) > 0)
			{
				std::string aLabel;
				if (mPageIndex == CHALLENGE_PAGE_PUZZLE)
				{
					if (IsScaryPotterLevel(aDef.mChallengeMode))
					{
						if (!mApp->HasFinishedAdventure() && aDef.mChallengeMode == GAMEMODE_SCARY_POTTER_4)
						{
							aLabel = "[FINISH_ADVENTURE_TOOLTIP]";
						}
						else
						{
							aLabel = "[ONE_MORE_SCARY_POTTER_TOOLTIP]";
						}
					}
					else if (IsIZombieLevel(aDef.mChallengeMode))
					{
						if (!mApp->HasFinishedAdventure() && aDef.mChallengeMode == GAMEMODE_PUZZLE_I_ZOMBIE_4)
						{
							aLabel = "[FINISH_ADVENTURE_TOOLTIP]";
						}
						else
						{
							aLabel = "[ONE_MORE_IZOMBIE_TOOLTIP]";
						}
					}
				}
				else if (!mApp->HasFinishedAdventure() || mApp->IsTrialStageLocked())
				{
					aLabel = "[FINISH_ADVENTURE_TOOLTIP]";
				}
				else if (mApp->IsSurvivalEndless(aDef.mChallengeMode))
				{
					aLabel = "[10_SURVIVAL_TOOLTIP]";
				}
				else if (mPageIndex == CHALLENGE_PAGE_SURVIVAL)
				{
					aLabel = "[ONE_MORE_SURVIVAL_TOOLTIP]";
				}
				else if (mPageIndex == CHALLENGE_PAGE_CHALLENGE)
				{
					aLabel = "[ONE_MORE_CHALLENGE_TOOLTIP]";
				}
				else continue;

				mToolTip->SetLabel(aLabel);
				mToolTip->mVisible = true;
				return;
			} // end if (MoreTrophiesNeeded(aChallengeMode) > 0)
		} // end if (label needs to be shown)
	}

	mToolTip->mVisible = false;
}
