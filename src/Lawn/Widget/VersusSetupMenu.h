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

#ifndef __VERSUSSETUPMENU_H__
#define __VERSUSSETUPMENU_H__

#include "widget/MenuWidget.h"

class LawnApp;

// Loads the real, user-supplied VSSetupMenu.txt from the console/Android TV build through
// Sexy::MenuWidget/MenuParser (see that header for exactly what's ported vs. recognized-
// but-inert). Its three buttons -- QUICK_BUTTON, CUSTOM_BUTTON, RANDOM_BUTTON -- are
// resolved by name at runtime rather than compile-time ids, since they come entirely from
// the script, the same way the real engine's LawnMenuParser-driven screens work.
class VersusSetupMenu : public Sexy::MenuWidget
{
public:
	LawnApp* mApp;

public:
	explicit VersusSetupMenu(LawnApp* theApp);

	void OnMenuButtonDepress(int theId) override;
};

#endif // __VERSUSSETUPMENU_H__
