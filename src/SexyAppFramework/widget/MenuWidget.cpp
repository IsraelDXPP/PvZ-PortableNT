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

#include "MenuWidget.h"
#include "ButtonWidget.h"
#include "WidgetManager.h"
#include "graphics/Font.h"
#include "SexyAppBase.h"
#include "misc/ResourceManager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace Sexy;

namespace
{
	// Splits raw source text into ';'-terminated statements, then each statement into
	// whitespace-separated tokens, honoring '...' quoted strings and (...) groups (kept as
	// one token, parens included, for the handful of commands that need their contents as
	// a comma list). '#' starts a line comment. Matches the decompiled menu script grammar
	// used by every .txt this port has seen (DefineWidgetIds/Enum/Define/AddWidget/etc).
	std::vector<std::vector<std::string>> Tokenize(const std::string& theSource)
	{
		std::vector<std::vector<std::string>> aStatements;
		std::vector<std::string> aTokens;
		std::string aCurrent;
		bool inQuote = false;
		int aGroupDepth = 0; // AddAnimator's curve tuples nest, e.g. ( 5, (0,1000), (0,600), 100 )
		size_t i = 0;
		while (i < theSource.size())
		{
			char c = theSource[i];
			if (!inQuote && aGroupDepth == 0 && c == '#')
			{
				while (i < theSource.size() && theSource[i] != '\n')
					i++;
				continue;
			}
			if (inQuote)
			{
				if (c == '\'')
					inQuote = false;
				else
					aCurrent += c;
				i++;
				continue;
			}
			if (aGroupDepth > 0)
			{
				aCurrent += c;
				if (c == '(')
					aGroupDepth++;
				else if (c == ')')
					aGroupDepth--;
				i++;
				continue;
			}
			if (c == '\'')
			{
				inQuote = true;
				i++;
				continue;
			}
			if (c == '(')
			{
				aGroupDepth = 1;
				aCurrent += c;
				i++;
				continue;
			}
			if (c == ';')
			{
				if (!aCurrent.empty())
				{
					aTokens.push_back(aCurrent);
					aCurrent.clear();
				}
				if (!aTokens.empty())
					aStatements.push_back(aTokens);
				aTokens.clear();
				i++;
				continue;
			}
			if (std::isspace(static_cast<unsigned char>(c)))
			{
				if (!aCurrent.empty())
				{
					aTokens.push_back(aCurrent);
					aCurrent.clear();
				}
				i++;
				continue;
			}
			if (c == ',')
			{
				// Bare commas outside a group (e.g. between statements) are not part of
				// this grammar anywhere this port has seen; treat as whitespace.
				if (!aCurrent.empty())
				{
					aTokens.push_back(aCurrent);
					aCurrent.clear();
				}
				i++;
				continue;
			}
			aCurrent += c;
			i++;
		}
		if (!aCurrent.empty())
			aTokens.push_back(aCurrent);
		if (!aTokens.empty())
			aStatements.push_back(aTokens);
		return aStatements;
	}
}

MenuParser::MenuParser(MenuWidget* theOwner) :
	mOwner(theOwner)
{
}

std::vector<std::string> MenuParser::SplitGroup(const std::string& theGroupText)
{
	// theGroupText includes the surrounding parens, e.g. "(QUICK_BUTTON, CUSTOM_BUTTON)".
	std::vector<std::string> aResult;
	std::string aCurrent;
	for (size_t i = 1; i + 1 < theGroupText.size(); i++)
	{
		char c = theGroupText[i];
		if (c == ',')
		{
			if (!aCurrent.empty())
			{
				aResult.push_back(aCurrent);
				aCurrent.clear();
			}
		}
		else if (!std::isspace(static_cast<unsigned char>(c)))
		{
			aCurrent += c;
		}
	}
	if (!aCurrent.empty())
		aResult.push_back(aCurrent);
	return aResult;
}

bool MenuParser::DefineSymbol(const std::string& theName, int theValue)
{
	mSymbols[theName] = theValue;
	return true;
}

int MenuParser::InternSymbol(const std::string& theName)
{
	auto anItr = mSymbols.find(theName);
	if (anItr != mSymbols.end())
		return anItr->second;
	int aValue = mNextAutoSymbol++;
	mSymbols[theName] = aValue;
	return aValue;
}

bool MenuParser::ResolveInt(const std::string& theToken, int& theValue) const
{
	if (theToken.empty())
		return false;
	char* anEnd = nullptr;
	long aParsed = std::strtol(theToken.c_str(), &anEnd, 10);
	if (anEnd != theToken.c_str() && *anEnd == '\0')
	{
		theValue = static_cast<int>(aParsed);
		return true;
	}
	auto anItr = mSymbols.find(theToken);
	if (anItr != mSymbols.end())
	{
		theValue = anItr->second;
		return true;
	}
	return false;
}

int MenuParser::GetWidgetId(std::string_view theName) const
{
	auto anItr = mSymbols.find(theName);
	return anItr != mSymbols.end() ? anItr->second : -1;
}

Widget* MenuParser::GetWidgetById(int theId) const
{
	auto anItr = mWidgetsById.find(theId);
	return anItr != mWidgetsById.end() ? anItr->second : nullptr;
}

Widget* MenuParser::CreateWidget(const std::string& theType, int theId)
{
	// Only the widget types this port's menu scripts actually use to build a working
	// screen. Types the decompiled build supports that need engine features this port
	// doesn't have (ImageWidget, LabelWidget, LawnButtonWidget, HelpBarWidget) intentionally
	// return nullptr: the AddWidget statement still parses (see HandleStatement), so a
	// script that references one doesn't fail to load, it just adds nothing for it.
	if (theType == "ButtonWidget")
		return new ButtonWidget(theId, mOwner);
	return nullptr;
}

bool MenuParser::HandleStatement(const std::vector<std::string>& theTokens)
{
	const std::string& aCommand = theTokens[0];

	// Commands recognized by the decompiled Sexy::MenuParser::HandleCommand that this port
	// accepts syntactically but doesn't act on, for the reasons in this file's header
	// comment: SetBackground, SetVisible, SetDisabled, SetPos, SetColor, SetLabelJustify's
	// sibling AddAnimator, Layout, SetImage/SetOverImage/SetDownImage/SetDisabledImage,
	// SetGameLinks, AddHelpButton. Falling through to `return true` for these means "parsed,
	// no effect" -- distinct from an actually unknown command, which is a load error below.
	static const std::vector<std::string> kRecognizedInertCommands = {
		"SetBackground", "SetVisible", "SetDisabled", "SetPos", "SetColor",
		"AddAnimator", "Layout", "SetImage", "SetOverImage", "SetDownImage",
		"SetDisabledImage", "SetGameLinks", "AddHelpButton",
	};

	if (aCommand == "Define")
	{
		if (theTokens.size() != 3)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		int aValue;
		if (!ResolveInt(theTokens[2], aValue))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		return DefineSymbol(theTokens[1], aValue);
	}

	if (aCommand == "Enum")
	{
		if (theTokens.size() != 2 || theTokens[1].empty() || theTokens[1].front() != '(')
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		for (const std::string& aName : SplitGroup(theTokens[1]))
			DefineSymbol(aName, mNextEnumValue++);
		return true;
	}

	if (aCommand == "DefineWidgetIds")
	{
		if (theTokens.size() != 2 || theTokens[1].empty() || theTokens[1].front() != '(')
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		for (const std::string& aName : SplitGroup(theTokens[1]))
			InternSymbol(aName);
		return true;
	}

	if (aCommand == "SetInitialFocus")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		int anId;
		if (!ResolveInt(theTokens[1], anId))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		Widget* aWidget = GetWidgetById(anId);
		if (aWidget && mOwner->mWidgetManager)
			mOwner->mWidgetManager->SetFocus(aWidget);
		return true;
	}

	if (aCommand == "AddWidget")
	{
		if (theTokens.size() != 3)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		// theTokens[2] is a widget id (e.g. QUICK_BUTTON, or -1 for an unreferenced one,
		// or a name -- like this port's VSSetupMenu.txt's HELP_BAR -- never declared by a
		// DefineWidgetIds this script happens to have). ResolveInt handles the first two;
		// InternSymbol covers the third by registering it here, same as DefineWidgetIds
		// would have.
		int anId;
		if (!ResolveInt(theTokens[2], anId))
			anId = InternSymbol(theTokens[2]);
		mCurrentWidget = CreateWidget(theTokens[1], anId);
		if (mCurrentWidget)
		{
			mOwner->AddWidget(mCurrentWidget);
			mWidgetsById[anId] = mCurrentWidget;
		}
		return true;
	}

	if (aCommand == "Resize")
	{
		if (theTokens.size() != 5)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		if (!mCurrentWidget)
		{
			mLastError = "Missing parse widget";
			return false;
		}
		int x, y, w, h;
		if (!ResolveInt(theTokens[1], x) || !ResolveInt(theTokens[2], y) ||
			!ResolveInt(theTokens[3], w) || !ResolveInt(theTokens[4], h))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		mCurrentWidget->Resize(x, y, w, h);
		return true;
	}

	if (aCommand == "SetFont")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget);
		if (!aButton)
		{
			mLastError = mCurrentWidget ? "Incorrect widget type in parse widget" : "Missing parse widget";
			return false;
		}
		_Font* aFont = gSexyAppBase->mResourceManager->GetFont(theTokens[1]);
		if (!aFont)
		{
			mLastError = "No resource with that name";
			return false;
		}
		aButton->mFont.reset(aFont->Duplicate());
		return true;
	}

	if (aCommand == "SetLabel")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget);
		if (!aButton)
		{
			mLastError = mCurrentWidget ? "Incorrect widget type in parse widget" : "Missing parse widget";
			return false;
		}
		// Not resolved through this port's PvzpStringTranslate ([BRACKETED] localization
		// keys): that lives in the game-specific PvzpLib layer, this is a generic
		// SexyAppFramework widget with no dependency on it. This port's menu scripts happen
		// to use plain literal text for every SetLabel that reaches a real ButtonWidget
		// anyway (VSSetupMenu.txt's 'Quick Play' etc.), so the raw token is used as-is.
		aButton->mLabel = theTokens[1];
		return true;
	}

	if (aCommand == "SetLabelJustify")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget);
		if (!aButton)
		{
			mLastError = mCurrentWidget ? "Incorrect widget type in parse widget" : "Missing parse widget";
			return false;
		}
		int aJustify;
		if (!ResolveInt(theTokens[1], aJustify))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		aButton->mLabelJustify = aJustify;
		return true;
	}

	if (std::find(kRecognizedInertCommands.begin(), kRecognizedInertCommands.end(), aCommand) != kRecognizedInertCommands.end())
		return true;

	mLastError = "Unknown Menu Command: " + aCommand;
	return false;
}

bool MenuParser::ParseFile(const std::string& theSource)
{
	for (const std::vector<std::string>& aStatement : Tokenize(theSource))
	{
		if (!HandleStatement(aStatement))
			return false;
	}
	return true;
}

MenuWidget::MenuWidget() :
	mParser(this)
{
}

MenuWidget::~MenuWidget() = default;

bool MenuWidget::LoadMenuFile(const std::string& theSource)
{
	bool aResult = mParser.ParseFile(theSource);
	mLastError = mParser.mLastError;
	return aResult;
}
