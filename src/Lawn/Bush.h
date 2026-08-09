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

#ifndef __BUSH_H__
#define __BUSH_H__

#include <cstdint>

#include "../ConstEnums.h"
#include "GameObject.h"

namespace Sexy
{
	class Graphics;
}

class Bush : public GameObject
{
public:
	int32_t                     mPosX;
	int32_t                     mPosY;
	int32_t                     mID;
	int32_t                     mBushIndex;
	ReanimationID               mReanimID;

public:
	void                    BushInitialize(int theRow, bool theNight);
	void                    Update();
	void                    Draw(Graphics* g);
	void                    AnimateBush();
};

#endif
