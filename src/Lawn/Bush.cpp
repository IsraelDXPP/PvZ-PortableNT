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

#include "Bush.h"

#include "Board.h"
#include "../LawnApp.h"
#include "../PvzpLib/Reanimator.h"
#include "../PvzpLib/PvzpCommon.h"

const ReanimationType cBushReanims[] = {
	ReanimationType::REANIM_BUSH3, ReanimationType::REANIM_BUSH5, ReanimationType::REANIM_BUSH4,
	ReanimationType::REANIM_BUSH3_NIGHT, ReanimationType::REANIM_BUSH5_NIGHT, ReanimationType::REANIM_BUSH4_NIGHT
};

const int cBushPos[][2] = {
	{ 950, 40 },
	{ 962, 168 },
	{ 968, 258 },
	{ 972, 378 },
	{ 964, 459 },
	{ 980, 510 }
};
const int cBushPos6Rows[][2] = {
	{ 952, 42 },
	{ 964, 170 },
	{ 968, 258 },
	{ 974, 380 },
	{ 966, 461 },
	{ 979, 509 }
};

void Bush::BushInitialize(int theRow, bool theNight)
{
	int aIndex = (theRow + 3) % 3;
	if (theNight)
		aIndex += 3;
	mPosX = mBoard->StageHas6Rows() ? cBushPos6Rows[theRow][0] : cBushPos[theRow][0];
	mPosY = mBoard->StageHas6Rows() ? cBushPos6Rows[theRow][1] : cBushPos[theRow][1];
	mID = theRow;
	mBushIndex = aIndex;
	mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_ZOMBIE, theRow + 1, 0);
	Reanimation* aBodyReanim = mApp->AddReanimation(mPosX, mPosY, mRenderOrder, cBushReanims[mBushIndex]);
	mReanimID = mApp->ReanimationGetID(aBodyReanim);
	aBodyReanim->PlayReanim("base bush", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 0.001f);
}

void Bush::AnimateBush()
{
	Reanimation* aReanim = mApp->ReanimationTryToGet(mReanimID);
	if (aReanim)
		aReanim->PlayReanim("anim_rustle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, RandRangeFloat(8.0f, 10.0f));
}

void Bush::Update()
{
	Reanimation* aReanim = mApp->ReanimationTryToGet(mReanimID);
	if (aReanim)
		aReanim->Update();
}

void Bush::Draw(Graphics* g)
{
	Reanimation* aReanim = mApp->ReanimationTryToGet(mReanimID);
	if (aReanim)
		aReanim->Draw(g);
}
