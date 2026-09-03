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

#include "LabelWidget.h"
#include "graphics/Font.h"
#include "graphics/Graphics.h"
#include "WidgetManager.h"
#include "SexyAppBase.h"

using namespace Sexy;

// SetAlign values from the menu script grammar.
enum
{
	ALIGN_LEFT = 0,
	ALIGN_CENTER = 1,
	ALIGN_RIGHT = 2
};

LabelWidget::LabelWidget()
{
	mAlign = ALIGN_LEFT;
	mHasAlpha = true;
}

LabelWidget::~LabelWidget() = default;

void LabelWidget::Draw(Graphics* g)
{
	if (mLabel.empty())
		return;

	_Font* aDefaultFont = mWidgetManager->mApp->mDefaultFont.load();
	if ((mFont == nullptr) && (aDefaultFont != nullptr))
		mFont.reset(aDefaultFont->Duplicate());

	if (mFont == nullptr)
		return;

	int aFontX = 0;
	if (mAlign == ALIGN_CENTER)
		aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
	else if (mAlign == ALIGN_RIGHT)
		aFontX = mWidth - mFont->StringWidth(mLabel);

	int aFontY = (mHeight - mFont->GetHeight()) / 2 + mFont->GetAscent();

	g->SetFont(mFont.get());
	g->SetColor(mColors[0]);
	g->DrawString(mLabel, aFontX, aFontY);
}
