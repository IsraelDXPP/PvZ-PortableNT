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

#ifndef __QUICKPLAYWIDGET_H__
#define __QUICKPLAYWIDGET_H__

#include "../../ConstEnums.h"
#include "widget/Widget.h"
#include "widget/ButtonListener.h"
#include "widget/CheckboxListener.h"

class GameSelector;
class LawnApp;
class NewLawnButton;
class LawnStoneButton;
class Zombie;
class Plant;

namespace Sexy
{
	class Checkbox;
};

using namespace Sexy;

class QuickPlayWidget : public Widget, public ButtonListener, public CheckboxListener
{
public:
	enum
	{
		QUICKPLAY_BTN_BACK = 400,
		QUICKPLAY_BTN_LEFT,
		QUICKPLAY_BTN_RIGHT,
		QUICKPLAY_BTN_PLAY,
		QUICKPLAY_BTN_CRAZY_SEEDS
	};

public:
	GameSelector*				mGameSelector;
	LawnApp*					mApp;
	NewLawnButton*				mBackButton;
	NewLawnButton*				mLeftButton;
	NewLawnButton*				mRightButton;
	LawnStoneButton*			mPlayButton;
	BackgroundType				mBackground;
	ZombieType					mZombieType;
	SeedType					mSeedType;
	Zombie*						mDisplayZombie;
	Plant*						mDisplayPlant;
	Plant*						mFlowerPot;
	ReanimationID				mHammerID;
	Checkbox*					mCrazySeedsCheck;

public:
	QuickPlayWidget(GameSelector* theGameSelector);
	~QuickPlayWidget() override;

	void						Open();
	void						BackToSelector();
	void						StartLevel();
	void						PreviousLevel();
	void						NextLevel();
	void						ChooseBackground();
	void						ChooseZombieType();
	void						ResetZombie();
	void						ResetPlant();
	ZombieType					GetZombieType(int ID);
	void						DrawPool(Graphics* g, bool isNight);

	void						Draw(Graphics* g) override;
	void						Update() override;
	void						AddedToManager(WidgetManager* theWidgetManager) override;
	void						RemovedFromManager(WidgetManager* theWidgetManager) override;
	void						KeyDown(KeyCode theKey) override;

	void						ButtonPress(int theId) override;
	void						ButtonDepress(int theId) override;
	void						ButtonDownTick(int) override {}
	void						ButtonMouseEnter(int) override {}
	void						ButtonMouseLeave(int) override {}
	void						ButtonMouseMove(int, int, int) override {}
	void						CheckboxChecked(int theId, bool checked) override {}
};

#endif
