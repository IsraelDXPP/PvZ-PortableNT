/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 *
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
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

#ifndef __LABELWIDGET_H__
#define __LABELWIDGET_H__

#include "Widget.h"
#include <memory>

namespace Sexy
{

class _Font;

// Minimal LabelWidget for the decompiled menu-script grammar (the vs setup screen's
// 'VS_PICK_SIDES' title). Draws a single line of text with the widget's font and color,
// justified by the menu script's SetAlign value (LA_Left=0 / LA_Center=1 / LA_Right=2).
class LabelWidget : public Widget
{
public:
	std::string				mLabel;
	std::unique_ptr<_Font>	mFont;
	int						mAlign;

public:
	LabelWidget();
	~LabelWidget() override;

	void			Draw(Graphics* g) override;
};

}

#endif //__LABELWIDGET_H__
