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

#include "ImageWidget.h"
#include "graphics/Image.h"
#include "graphics/Graphics.h"

using namespace Sexy;

ImageWidget::ImageWidget()
{
	mImage = nullptr;
	mOverImage = nullptr;
	mHasAlpha = true;
}

ImageWidget::~ImageWidget() = default;

void ImageWidget::Draw(Graphics* g)
{
	Image* anImage = mImage;
	if (anImage == nullptr)
		anImage = mOverImage;
	if (anImage == nullptr)
		return;
	// Stretch the image to the widget rect. If the rect and image are the same size this is
	// a 1:1 blit; otherwise it scales, which matches the menu script's Resize-after-SetImage
	// layout (positions/sizes come from the .txt).
	g->DrawImage(anImage, 0, 0, mWidth, mHeight);
}
