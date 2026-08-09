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

#pragma once

#include "ConstEnums.h"
constexpr const double PI = 3.141592653589793;

// Constants
constexpr const int BOARD_WIDTH = 1280;
constexpr const int BOARD_HEIGHT = 720;
constexpr const int BOARD_OFFSET_X = 220;
constexpr const int BOARD_OFFSET_Y = 60;
constexpr const int BOARD_ADDITIONAL_WIDTH = 240;
constexpr const bool HAS_PAGE_SELECTOR = true;
constexpr const bool HAS_ACHIEVEMENTS = true;
constexpr const int WIDE_BOARD_WIDTH = 800 + BOARD_ADDITIONAL_WIDTH;
constexpr const int BOARD_EDGE = -100 + BOARD_ADDITIONAL_WIDTH;
constexpr const int BOARD_IMAGE_WIDTH_OFFSET = 1180 + BOARD_ADDITIONAL_WIDTH;
constexpr const int BOARD_ICE_START = 800 + BOARD_ADDITIONAL_WIDTH;
constexpr const int LAWN_XMIN = 40;
constexpr const int LAWN_YMIN = 80;
constexpr const int HIGH_GROUND_HEIGHT = 30;

constexpr const int STREET_ZOMBIE_START_X = 1020;
constexpr const int STREET_ZOMBIE_ROOF_START_X = 900;
constexpr const int STREET_ZOMBIE_START_Y = 70;
constexpr const int STREET_ZOMBIE_GRID_SIZE_X = 30;
constexpr const int STREET_ZOMBIE_GRID_SIZE_Y = 90;
constexpr const int STREET_ZOMBIE_ROOF_OFFSET = 15;
constexpr const int ROOF_POLE_START = WIDE_BOARD_WIDTH + 70 - BOARD_ADDITIONAL_WIDTH;
constexpr const int ROOF_POLE_END = -BOARD_WIDTH;
constexpr const int ROOF_TREE_START = WIDE_BOARD_WIDTH + 130 - BOARD_ADDITIONAL_WIDTH;
constexpr const int ROOF_TREE_END = -670;

constexpr const int ZOMBIE_CLIPRECT_WIDTH = BOARD_WIDTH - 45;

constexpr const int SEEDBANK_MAX = 10;
constexpr const int SEED_BANK_OFFSET_X = 0;
constexpr const int SEED_BANK_OFFSET_X_END = 10;
constexpr const int SEED_CHOOSER_EXTRA_HEIGHT = 120;
constexpr const int SEED_CHOOSER_OFFSET_Y = 720;
constexpr const int SEED_PACKET_WIDTH = 50;
constexpr const int SEED_PACKET_HEIGHT = 70;

// About levels
constexpr const int ADVENTURE_AREAS = 5;
constexpr const int LEVELS_PER_AREA = 10;
constexpr const int NUM_LEVELS = ADVENTURE_AREAS * LEVELS_PER_AREA;
constexpr const int FINAL_LEVEL = NUM_LEVELS;
constexpr const int LAST_STAND_FLAGS = 5;
constexpr const int SURVIVAL_NORMAL_FLAGS = 5;
constexpr const int SURVIVAL_HARD_FLAGS = 10;
