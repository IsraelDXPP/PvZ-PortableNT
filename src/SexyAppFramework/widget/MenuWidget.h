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

#ifndef __MENUWIDGET_H__
#define __MENUWIDGET_H__

#include "Widget.h"
#include "ButtonListener.h"
#include <string>
#include <string_view>
#include <vector>
#include <map>

namespace Sexy
{

// A tiny interpreter for the console/Android TV build's declarative menu-script format,
// ported from the decompiled Sexy::MenuParser::HandleCommand (libGameMain.so): each
// statement is `Command arg1 arg2 ...;`, `#` starts a line comment, 'single quoted'
// strings are text, parenthesized groups like `(a, b, c)` are comma lists, and a bare word
// that was named by an earlier Define/Enum/DefineWidgetIds resolves to the value it was
// given. This only implements the commands this port's menu scripts actually use to build
// real, functional widgets (AddWidget ButtonWidget, SetFont, SetLabel, SetLabelJustify,
// Resize, SetInitialFocus); commands the decompiled parser also recognizes but that need
// engine features this port doesn't have (SetImage/SetOverImage/etc. -- no ImageWidget or
// LawnButtonWidget exists here; AddAnimator/AddHelpButton -- no widget-curve-animation or
// controller-hint-bar system exists here; SetGameLinks -- no directional
// gamepad/keyboard focus-link system exists here) are still recognized syntactically (so a
// real menu script doesn't fail to load) but intentionally have no effect. Recognized-but-
// inert commands are listed in HandleStatement's comment, not silently swallowed elsewhere.
class MenuWidget;
class MenuParser
{
public:
	explicit MenuParser(MenuWidget* theOwner);

	// Parses theSource (the full contents of a .txt menu script) and adds child widgets to
	// the owning MenuWidget. Returns false (with mLastError set) on a malformed statement
	// or a reference to a widget id that was never declared by DefineWidgetIds/AddWidget.
	bool ParseFile(const std::string& theSource);

	int GetWidgetId(std::string_view theName) const;
	Widget* GetWidgetById(int theId) const;

	std::string mLastError;

private:
	bool HandleStatement(const std::vector<std::string>& theTokens);
	bool ResolveInt(const std::string& theToken, int& theValue) const;
	bool DefineSymbol(const std::string& theName, int theValue);
	int InternSymbol(const std::string& theName); // auto-registers on first use
	static std::vector<std::string> SplitGroup(const std::string& theGroupText);
	Widget* CreateWidget(const std::string& theType, int theId);

	MenuWidget*						mOwner;
	std::map<std::string, int, std::less<>>	mSymbols; // Define/Enum/DefineWidgetIds names -> value
	std::map<int, Widget*>				mWidgetsById;
	Widget*							mCurrentWidget = nullptr; // the most recent AddWidget target ("parse widget")
	int								mNextAutoSymbol = 1;
	int								mNextEnumValue = 0;
};

// Sexy::MenuWidget in the decompiled build: a Widget whose children are built from a menu
// script instead of hand-written C++. Every child ButtonWidget's press/depress forwards
// here with its script-assigned id, same shape as any other ButtonListener widget in this
// codebase.
class MenuWidget : public Widget, public ButtonListener
{
public:
	MenuWidget();
	~MenuWidget() override;

	bool LoadMenuFile(const std::string& theSource);

	void ButtonPress(int theId) override { OnMenuButtonPress(theId); }
	void ButtonDepress(int theId) override { OnMenuButtonDepress(theId); }
	void ButtonDownTick(int) override {}
	void ButtonMouseEnter(int) override {}
	void ButtonMouseLeave(int) override {}
	void ButtonMouseMove(int, int, int) override {}

	virtual void OnMenuButtonPress(int) {}
	virtual void OnMenuButtonDepress(int) {}

	int GetWidgetId(std::string_view theName) const { return mParser.GetWidgetId(theName); }
	Widget* GetWidgetById(int theId) const { return mParser.GetWidgetById(theId); }

	std::string mLastError;

private:
	MenuParser mParser;
};

}

#endif // __MENUWIDGET_H__
