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

#ifndef __VERSUSRESULTSMENU_H__
#define __VERSUSRESULTSMENU_H__

#include "widget/MenuWidget.h"

class LawnApp;

// Loads the real, user-supplied VSResultsMenu.txt from the console/Android TV build through
// Sexy::MenuWidget/MenuParser -- see that header for exactly what's ported vs. recognized-
// but-inert. Shown by LawnApp::ShowVersusResultsMenu once a Versus match ends (see
// GridItem::TakeDamage's win condition and Board::ZombiesWon's Versus branch), overlaying
// the board the same way VersusSetupMenu overlays the game selector. PLAY_AGAIN and
// QUIT_BUTTON -- resolved by name at runtime, like VersusSetupMenu's buttons -- are its only
// two functional widgets: the script's INFO_BOX_P1/P2, PLANT_SIDE/ZOMBIE_SIDE and WIN_IMAGE
// widgets are all ImageWidget, which this port's menu engine intentionally doesn't create
// (no art ships for any of it -- see MenuWidget.h), so this screen doesn't yet show which
// side won, only lets the match be replayed or exited.
class VersusResultsMenu : public Sexy::MenuWidget
{
public:
	LawnApp* mApp;

public:
	explicit VersusResultsMenu(LawnApp* theApp);

	void OnMenuButtonDepress(int theId) override;
};

#endif // __VERSUSRESULTSMENU_H__
