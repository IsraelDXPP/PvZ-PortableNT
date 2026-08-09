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

#ifndef __CHALLENGEPAGESDIALOG_H__
#define __CHALLENGEPAGESDIALOG_H__

#include "LawnDialog.h"
#include "widget/Slider.h"
#include "widget/SliderListener.h"

class ChallengePagesDialog : public LawnDialog, public SliderListener
{
public:
	LawnApp*					mApp;
	Slider*						mSlider;
	LawnStoneButton*			mPageButtons[MAX_CHALLANGE_PAGES];
	Rect						mPageButtonRects[MAX_CHALLANGE_PAGES];
	float						mScrollPosition;
	float						mScrollAmount;
	const float					mBaseScrollSpeed = 1.5f;
	const float					mScrollAccel = 0.1f;
	float						mMaxScrollPosition;
	Rect						mClipRect;

public:
	ChallengePagesDialog(LawnApp* theApp);
	~ChallengePagesDialog() override;

	void						AddedToManager(WidgetManager* theWidgetManager) override;
	void						RemovedFromManager(WidgetManager* theWidgetManager) override;
	void						Draw(Graphics* g) override;
	void						Update() override;
	void						ButtonDepress(int theId) override;
	void						SliderVal(int theId, double theVal) override;
	void						MouseWheel(int theDelta) override;
	void						KeyDown(KeyCode theKey) override;
};

#endif
