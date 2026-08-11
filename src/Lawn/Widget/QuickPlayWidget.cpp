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

#include "QuickPlayWidget.h"
#include "GameSelector.h"
#include "GameButton.h"
#include "../Zombie.h"
#include "../Plant.h"
#include "../System/PoolEffect.h"
#include "../System/Music.h"
#include "../System/PlayerInfo.h"
#include "../LawnCommon.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../GameConstants.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "../../PvzpLib/PvzpCommon.h"
#include "../../PvzpLib/Reanimator.h"
#include "widget/Checkbox.h"
#include "widget/WidgetManager.h"

#include <algorithm>

QuickPlayWidget::QuickPlayWidget(GameSelector* theGameSelector)
{
	mGameSelector = theGameSelector;
	mApp = theGameSelector->mApp;

	mBackground = BackgroundType::BACKGROUND_1_DAY;
	mZombieType = ZombieType::ZOMBIE_NORMAL;
	mSeedType = SeedType::SEED_PEASHOOTER;

	mBackButton = MakeNewButton(QUICKPLAY_BTN_BACK, this, "", nullptr,
		Sexy::IMAGE_BLANK, Sexy::IMAGE_ZOMBATAR_MAINMENUBACK_HIGHLIGHT, Sexy::IMAGE_ZOMBATAR_MAINMENUBACK_HIGHLIGHT);

	mLeftButton = MakeNewButton(QUICKPLAY_BTN_LEFT, this, "", nullptr, Sexy::IMAGE_ZOMBATAR_PREV_BUTTON,
		Sexy::IMAGE_ZOMBATAR_PREV_BUTTON_HIGHLIGHT, Sexy::IMAGE_ZOMBATAR_PREV_BUTTON_HIGHLIGHT);

	mRightButton = MakeNewButton(QUICKPLAY_BTN_RIGHT, this, "", nullptr, Sexy::IMAGE_ZOMBATAR_NEXT_BUTTON,
		Sexy::IMAGE_ZOMBATAR_NEXT_BUTTON_HIGHLIGHT, Sexy::IMAGE_ZOMBATAR_NEXT_BUTTON_HIGHLIGHT);

	mPlayButton = MakeButton(QUICKPLAY_BTN_PLAY, this, "[PLAY_BUTTON]");

	mCrazySeedsCheck = MakeNewCheckbox(QUICKPLAY_BTN_CRAZY_SEEDS, this, mApp->mCrazySeeds);
	mCrazySeedsCheck->mVisible = true;

	Resize(BOARD_WIDTH, 0, BOARD_WIDTH, BOARD_HEIGHT);

	mDisplayZombie = new Zombie();
	mDisplayZombie->mApp = mApp;
	mDisplayZombie->mBoard = nullptr;
	mDisplayZombie->ZombieInitialize(0, mZombieType, false, nullptr, Zombie::ZOMBIE_WAVE_UI);

	mDisplayPlant = new Plant();
	mDisplayPlant->mIsOnBoard = false;
	mDisplayPlant->PlantInitialize(0, 0, mSeedType, SeedType::SEED_NONE);

	mFlowerPot = new Plant();
	mFlowerPot->mIsOnBoard = false;
	mFlowerPot->PlantInitialize(0, 0, SeedType::SEED_FLOWERPOT, SeedType::SEED_NONE);

	ReanimatorEnsureDefinitionLoaded(ReanimationType::REANIM_HAMMER, true);
	Reanimation* aHammerReanim = mApp->AddReanimation(250.0f + BOARD_ADDITIONAL_WIDTH, 280.0f, 0, ReanimationType::REANIM_HAMMER);
	aHammerReanim->mIsAttachment = true;
	aHammerReanim->PlayReanim("anim_whack_zombie", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
	aHammerReanim->mAnimTime = 1.0f;
	mHammerID = mApp->ReanimationGetID(aHammerReanim);

	mBackButton->Resize(372, 533, IMAGE_ZOMBATAR_MAINMENUBACK_HIGHLIGHT->mWidth, IMAGE_ZOMBATAR_MAINMENUBACK_HIGHLIGHT->mHeight);
	mLeftButton->Resize(373 + BOARD_ADDITIONAL_WIDTH, 380, IMAGE_ZOMBATAR_PREV_BUTTON->mWidth, IMAGE_ZOMBATAR_PREV_BUTTON->mHeight);
	mPlayButton->Resize(mLeftButton->mX + mLeftButton->mWidth + 20, mLeftButton->mY, 163, 46);
	mRightButton->Resize(mPlayButton->mX + mPlayButton->mWidth + 20, mPlayButton->mY, IMAGE_ZOMBATAR_NEXT_BUTTON->mWidth, IMAGE_ZOMBATAR_NEXT_BUTTON->mHeight);
	mCrazySeedsCheck->Resize(130 + BOARD_ADDITIONAL_WIDTH, mRightButton->mY + 2, 50, 50);

	ChooseBackground();
	ResetZombie();
	ResetPlant();
}

QuickPlayWidget::~QuickPlayWidget()
{
	delete mLeftButton;
	delete mRightButton;
	delete mBackButton;
	delete mPlayButton;
	delete mCrazySeedsCheck;
	delete mDisplayZombie;
	delete mDisplayPlant;
	delete mFlowerPot;
}

void QuickPlayWidget::Open()
{
	ChooseBackground();
	ResetZombie();
	ResetPlant();

	mGameSelector->SlideTo(-BOARD_WIDTH, 0);
	if (mWidgetManager)
	{
		mWidgetManager->BringToFront(this);
		mWidgetManager->SetFocus(this);
	}
}

void QuickPlayWidget::BackToSelector()
{
	mGameSelector->BackFromQuickPlay();
}

void QuickPlayWidget::Draw(Graphics* g)
{
	if (mX >= BOARD_WIDTH)
		return;

	g->DrawImage(Sexy::IMAGE_ZOMBATAR_MAIN_BG, 0, -42);
	g->SetClipRect(130 + BOARD_ADDITIONAL_WIDTH, 30, 530, 370);
	switch (mBackground)
	{
	case BackgroundType::BACKGROUND_1_DAY:			g->DrawImage(Sexy::IMAGE_BACKGROUND1, -130, -BOARD_OFFSET_Y); break;
	case BackgroundType::BACKGROUND_2_NIGHT:		g->DrawImage(Sexy::IMAGE_BACKGROUND2, -130, -BOARD_OFFSET_Y); break;
	case BackgroundType::BACKGROUND_3_POOL:
		g->DrawImage(Sexy::IMAGE_BACKGROUND3, -130, -BOARD_OFFSET_Y);
		DrawPool(g, false);
		break;
	case BackgroundType::BACKGROUND_4_FOG:
		g->DrawImage(Sexy::IMAGE_BACKGROUND4, -130, -BOARD_OFFSET_Y);
		DrawPool(g, true);
		break;
	case BackgroundType::BACKGROUND_5_ROOF:			g->DrawImage(Sexy::IMAGE_BACKGROUND5, -130, -BOARD_OFFSET_Y); break;
	case BackgroundType::BACKGROUND_6_BOSS:			g->DrawImage(Sexy::IMAGE_BACKGROUND6BOSS, -130, -BOARD_OFFSET_Y); break;
	default:										break;
	}
	if (mDisplayZombie)
	{
		if (mApp->mQuickLevel != 35)
		{
			Graphics aZombieGraphics = Graphics(*g);
			if (mApp->mQuickLevel == 25)
			{
				mDisplayZombie->mScaleZombie = 0.5f;
			}
			mDisplayZombie->mPosX = 340 + BOARD_ADDITIONAL_WIDTH;
			mDisplayZombie->mPosY = 240;
			if (mBackground == BackgroundType::BACKGROUND_3_POOL || mBackground == BackgroundType::BACKGROUND_4_FOG)
			{
				mDisplayZombie->mPosY -= 120;
			}
			if (mZombieType == ZombieType::ZOMBIE_BOSS)
			{
				mDisplayZombie->mPosX = -100 + BOARD_ADDITIONAL_WIDTH;
				mDisplayZombie->mPosY = -20;
			}
			if (mDisplayZombie->BeginDraw(&aZombieGraphics))
			{
				if (mZombieType != ZombieType::ZOMBIE_BUNGEE && mZombieType != ZombieType::ZOMBIE_BOSS &&
					mZombieType != ZombieType::ZOMBIE_ZAMBONI && mZombieType != ZombieType::ZOMBIE_CATAPULT)
					mDisplayZombie->DrawShadow(&aZombieGraphics);
				mDisplayZombie->Draw(&aZombieGraphics);
				mDisplayZombie->EndDraw(&aZombieGraphics);
			}
		}
	}
	if (mFlowerPot)
	{
		if (mBackground == BackgroundType::BACKGROUND_5_ROOF || mBackground == BackgroundType::BACKGROUND_6_BOSS)
		{
			Graphics aPotGraphics = Graphics(*g);
			mFlowerPot->mX = 280 + BOARD_ADDITIONAL_WIDTH;
			mFlowerPot->mY = 280;
			if (mFlowerPot->BeginDraw(&aPotGraphics))
			{
				mFlowerPot->Draw(&aPotGraphics);
				mFlowerPot->EndDraw(&aPotGraphics);
			}
		}
	}
	if (mDisplayPlant)
	{
		if (mApp->mQuickLevel != 35 && mApp->mQuickLevel != 15)
		{
			Graphics aPlantGraphics = Graphics(*g);

			mDisplayPlant->mX = 280 + BOARD_ADDITIONAL_WIDTH;
			mDisplayPlant->mY = 280;
			if ((mBackground == BackgroundType::BACKGROUND_3_POOL || mBackground == BackgroundType::BACKGROUND_4_FOG) && !mDisplayPlant->IsAquatic(mDisplayPlant->mSeedType))
			{
				mDisplayPlant->mY -= 120;
			}
			if (mBackground == BackgroundType::BACKGROUND_5_ROOF || mBackground == BackgroundType::BACKGROUND_6_BOSS)
			{
				mDisplayPlant->mY -= 10;
			}
			if (mDisplayPlant->BeginDraw(&aPlantGraphics))
			{
				mDisplayPlant->Draw(&aPlantGraphics);
				mDisplayPlant->EndDraw(&aPlantGraphics);
			}
		}
	}
	if (mApp->mQuickLevel == 5)
	{
		g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 268 + BOARD_ADDITIONAL_WIDTH, 77);
	}
	if (mApp->mQuickLevel == 15)
	{
		mApp->ReanimationGet(mHammerID)->Draw(g);
	}
	if (mApp->mQuickLevel == 35)
	{
		g->DrawImageCel(Sexy::IMAGE_SCARY_POT, 370 + BOARD_ADDITIONAL_WIDTH, 270, 0, 1);
		g->DrawImageCel(Sexy::IMAGE_SCARY_POT, 290 + BOARD_ADDITIONAL_WIDTH, 270, 1, 1);
	}
	g->ClearClipRect();
	int posX = 100 + BOARD_ADDITIONAL_WIDTH;
	g->DrawImage(Sexy::IMAGE_QUICKPLAY_WIDGET, posX, 0);
	PvzpDrawStringWrapped(g, mApp->GetStageString(mApp->mQuickLevel),
		Rect(posX, 8, Sexy::IMAGE_QUICKPLAY_WIDGET->mWidth, 30),
		Sexy::FONT_DWARVENTODCRAFT18GREENINSET, Color::White, DS_ALIGN_CENTER);
	PvzpDrawStringWrapped(g, "[CRAZY_DAVE_SEEDS]",
		Rect(mCrazySeedsCheck->mX + 45, mCrazySeedsCheck->mY + 10, 350, 30),
		Sexy::FONT_DWARVENTODCRAFT18GREENINSET, Color::White, DS_ALIGN_LEFT);
}

void QuickPlayWidget::KeyDown(KeyCode theKey)
{
	if (mWidgetManager->mFocusWidget != this)
		return;
	switch (theKey)
	{
	case KEYCODE_ESCAPE:
		BackToSelector();
		break;
	case KEYCODE_LEFT:
		PreviousLevel();
		break;
	case KEYCODE_RIGHT:
		NextLevel();
		break;
	case KEYCODE_RETURN:
		StartLevel();
		break;
	}
}

void QuickPlayWidget::DrawPool(Graphics* g, bool isNight)
{
	g->SetClipRect(135 + BOARD_ADDITIONAL_WIDTH - mX, 30, 450, 370);
	int aOffsetX = BOARD_ADDITIONAL_WIDTH / 2 + 12;
	g->mTransX += aOffsetX;
	mApp->mPoolEffect->PoolEffectDraw(g, isNight);
	g->mTransX -= aOffsetX;
	g->ClearClipRect();
}

void QuickPlayWidget::ChooseBackground()
{
	std::string aGroupName;
	if (mApp->mQuickLevel == 35)
	{
		aGroupName = "DelayLoad_Background2";
		mBackground = BackgroundType::BACKGROUND_2_NIGHT;
	}
	else if (mApp->mQuickLevel <= 1 * LEVELS_PER_AREA)
	{
		aGroupName = "DelayLoad_Background1";
		mBackground = BackgroundType::BACKGROUND_1_DAY;
	}
	else if (mApp->mQuickLevel <= 2 * LEVELS_PER_AREA)
	{
		aGroupName = "DelayLoad_Background2";
		mBackground = BackgroundType::BACKGROUND_2_NIGHT;
	}
	else if (mApp->mQuickLevel <= 3 * LEVELS_PER_AREA)
	{
		aGroupName = "DelayLoad_Background3";
		mBackground = BackgroundType::BACKGROUND_3_POOL;
	}
	else if (mApp->mQuickLevel <= 4 * LEVELS_PER_AREA)
	{
		aGroupName = "DelayLoad_Background4";
		mBackground = BackgroundType::BACKGROUND_4_FOG;
	}
	else if (mApp->mQuickLevel < FINAL_LEVEL)
	{
		aGroupName = "DelayLoad_Background5";
		mBackground = BackgroundType::BACKGROUND_5_ROOF;
	}
	else if (mApp->mQuickLevel == FINAL_LEVEL)
	{
		aGroupName = "DelayLoad_Background6";
		mBackground = BackgroundType::BACKGROUND_6_BOSS;
	}
	else
	{
		aGroupName = "DelayLoad_Background1";
		mBackground = BackgroundType::BACKGROUND_1_DAY;
	}
	PvzpLoadResources(aGroupName);
}

void QuickPlayWidget::ChooseZombieType()
{
	if (mApp->mQuickLevel == 45)
	{
		mZombieType = ZombieType::ZOMBIE_BUNGEE;
		return;
	}
	mZombieType = ZombieType::ZOMBIE_NORMAL;
	for (int i = 0; i < NUM_ZOMBIE_TYPES; i++)
	{
		ZombieType aZombieType = GetZombieType(i);
		ZombieDefinition aZombieDefinition = GetZombieDefinition(aZombieType);
		if (aZombieType != ZombieType::ZOMBIE_INVALID)
		{
			if (mApp->mQuickLevel == aZombieDefinition.mStartingLevel)
			{
				mZombieType = aZombieDefinition.mZombieType;
				break;
			}
		}
	}
}

ZombieType QuickPlayWidget::GetZombieType(int ID)
{
	return ID < NUM_ZOMBIE_TYPES ? (ZombieType)ID : ZombieType::ZOMBIE_INVALID;
}

void QuickPlayWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	AddWidget(mBackButton);
	AddWidget(mLeftButton);
	AddWidget(mRightButton);
	AddWidget(mPlayButton);
	AddWidget(mCrazySeedsCheck);
}

void QuickPlayWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveWidget(mBackButton);
	RemoveWidget(mLeftButton);
	RemoveWidget(mRightButton);
	RemoveWidget(mPlayButton);
	RemoveWidget(mCrazySeedsCheck);
}

void QuickPlayWidget::ButtonPress(int theId)
{
	mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
}

void QuickPlayWidget::Update()
{
	Reanimation* aHammerReanim = mApp->ReanimationTryToGet(mHammerID);
	if (aHammerReanim)
	{
		aHammerReanim->Update();
	}
	mApp->mPoolEffect->PoolEffectUpdate();
	if (mDisplayZombie) mDisplayZombie->Update();
	if (mDisplayPlant) mDisplayPlant->Update();
	if (mFlowerPot) mFlowerPot->Update();
	MarkDirty();
}

void QuickPlayWidget::ButtonDepress(int theId)
{
	switch (theId)
	{
	case QUICKPLAY_BTN_BACK:
		BackToSelector();
		break;
	case QUICKPLAY_BTN_LEFT:
		PreviousLevel();
		break;
	case QUICKPLAY_BTN_RIGHT:
		NextLevel();
		break;
	case QUICKPLAY_BTN_PLAY:
		StartLevel();
		break;
	}
}

void QuickPlayWidget::ResetZombie()
{
	ChooseZombieType();
	delete mDisplayZombie;
	mDisplayZombie = new Zombie();
	mDisplayZombie->mApp = mApp;
	mDisplayZombie->mBoard = nullptr;
	mDisplayZombie->ZombieInitialize(0, mZombieType, false, nullptr, Zombie::ZOMBIE_WAVE_UI);
}

void QuickPlayWidget::ResetPlant()
{
	int mSeedLevel = mApp->mQuickLevel - 1;
	if (mApp->mQuickLevel % 10 == 0)
		mSeedLevel -= 1;
	if (mApp->GetAwardSeedForLevel(mApp->mQuickLevel - 1) == SeedType::SEED_FLOWERPOT)
		return;
	SeedType aSpecialSeed;
	bool aSpecialLevel = mApp->mQuickLevel == 5 || mApp->mQuickLevel == 25 || mApp->mQuickLevel == 45;
	switch (mApp->mQuickLevel)
	{
	case 5:
		aSpecialSeed = SeedType::SEED_EXPLODE_O_NUT;
		break;
	case 25:
		aSpecialSeed = SeedType::SEED_PEASHOOTER;
		break;
	case 45:
		aSpecialSeed = SeedType::SEED_CHOMPER;
		break;
	default:
		aSpecialSeed = SeedType::SEED_PEASHOOTER;
		break;
	}
	delete mDisplayPlant;
	mDisplayPlant = new Plant();
	mDisplayPlant->mIsOnBoard = false;
	mDisplayPlant->PlantInitialize(0, 0, !aSpecialLevel ? mApp->GetAwardSeedForLevel(mSeedLevel) : aSpecialSeed, SeedType::SEED_NONE);
}

void QuickPlayWidget::StartLevel()
{
	mApp->mCrazySeeds = mCrazySeedsCheck->mChecked;
	mApp->KillGameSelector();
	mApp->StartQuickPlay();
}

void QuickPlayWidget::PreviousLevel()
{
	mApp->mQuickLevel = std::clamp(mApp->mQuickLevel - 1, 1, NUM_LEVELS);
	ChooseBackground();
	ResetZombie();
	ResetPlant();
}

void QuickPlayWidget::NextLevel()
{
	mApp->mQuickLevel = std::clamp(mApp->mQuickLevel + 1, 1, NUM_LEVELS);
	ChooseBackground();
	ResetZombie();
	ResetPlant();
}
