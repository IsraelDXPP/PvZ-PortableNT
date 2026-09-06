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
#include "ImageWidget.h"
#include "LabelWidget.h"
#include "WidgetManager.h"
#include "graphics/Font.h"
#include "graphics/Image.h"
#include "SexyAppBase.h"
#include "misc/ResourceManager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>

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
	// Widget types used by this port's menu scripts. LawnButtonWidget maps to the generic
	// ButtonWidget (which already has mButtonImage/mOverImage support). ImageWidget and
	// LabelWidget are new minimal engine-layer widgets for the decompiled menu-script
	// grammar's image labels and text labels. HelpBarWidget is still dropped (unused).
	if (theType == "ButtonWidget" || theType == "LawnButtonWidget")
		return new ButtonWidget(theId, mOwner);
	if (theType == "ImageWidget")
		return new ImageWidget();
	if (theType == "LabelWidget")
		return new LabelWidget();
	return nullptr;
}

bool MenuParser::HandleStatement(const std::vector<std::string>& theTokens)
{
	const std::string& aCommand = theTokens[0];

	// Commands recognized by the decompiled Sexy::MenuParser::HandleCommand that this port
	// accepts syntactically but doesn't act on, for the reasons in this file's header
	// comment: SetBackground, SetVisible, SetDisabled, SetPos.
	// SetImage/SetOverImage/SetDownImage/SetDisabledImage are now handled (they resolve the
	// image name via ResourceManager and set the widget's image pointer). SetColor is now
	// handled (resolves enum name, parses (r,g,b), calls Widget::SetColor). SetAlign is now
	// handled (sets LabelWidget::mAlign or ButtonWidget::mLabelJustify). SetFont/SetLabel
	// are now handled for both ButtonWidget and LabelWidget.
	static const std::vector<std::string> kRecognizedInertCommands = {
		"SetBackground", "SetVisible", "SetDisabled", "SetPos",
		"AddAnimator", "Layout",
		"SetGameLinks", "AddHelpButton",
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
		int anId = 0;
		for (const std::string& aName : SplitGroup(theTokens[1]))
			DefineSymbol(aName, anId++);
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
		_Font* aFont = gSexyAppBase->mResourceManager->GetFont(theTokens[1]);
		if (!aFont)
		{
			mLastError = "No resource with that name";
			return false;
		}
		if (ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget))
			aButton->mFont.reset(aFont->Duplicate());
		else if (LabelWidget* aLabel = dynamic_cast<LabelWidget*>(mCurrentWidget))
			aLabel->mFont.reset(aFont->Duplicate());
		else
		{
			mLastError = mCurrentWidget ? "Incorrect widget type in parse widget" : "Missing parse widget";
			return false;
		}
		return true;
	}

	if (aCommand == "SetLabel")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		// Not resolved through this port's PvzpStringTranslate ([BRACKETED] localization
		// keys): that lives in the game-specific PvzpLib layer, this is a generic
		// SexyAppFramework widget with no dependency on it. This port's menu scripts happen
		// to use plain literal text for every SetLabel that reaches a real ButtonWidget
		// anyway (VSSetupMenu.txt's 'Quick Play' etc.), so the raw token is used as-is.
		if (ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget))
			aButton->mLabel = theTokens[1];
		else if (LabelWidget* aLabel = dynamic_cast<LabelWidget*>(mCurrentWidget))
			aLabel->mLabel = theTokens[1];
		else
		{
			mLastError = mCurrentWidget ? "Incorrect widget type in parse widget" : "Missing parse widget";
			return false;
		}
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
		// BUTTON_LABEL_WRAP_CENTER (2) is used by VSSetupSides.txt for centered multi-line
		// labels. The original binary treats 2 as left-align, but the QEWide reference
		// project maps it to BUTTON_LABEL_CENTER (0) for proper centered rendering. Our
		// ButtonWidget::Draw only handles CENTER (0) and RIGHT (1), so map 2 → 0.
		if (aJustify == 2)
			aJustify = ButtonWidget::BUTTON_LABEL_CENTER;
		aButton->mLabelJustify = aJustify;
		return true;
	}

	if (aCommand == "SetImage" || aCommand == "SetOverImage" ||
		aCommand == "SetDownImage" || aCommand == "SetDisabledImage")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		if (!mCurrentWidget)
		{
			mLastError = "Missing parse widget";
			return false;
		}
		Image* anImage = gSexyAppBase->mResourceManager->GetImage(theTokens[1]);
		// Non-fatal: the image may not be registered yet, or the path may be invalid.
		// The widget simply stays transparent for that state.
		if (ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget))
		{
			if (aCommand == "SetImage")
				aButton->mButtonImage = anImage;
			else if (aCommand == "SetOverImage")
				aButton->mOverImage = anImage;
			else if (aCommand == "SetDownImage")
				aButton->mDownImage = anImage;
			else if (aCommand == "SetDisabledImage")
				aButton->mDisabledImage = anImage;
		}
		else if (ImageWidget* anImgW = dynamic_cast<ImageWidget*>(mCurrentWidget))
		{
			if (aCommand == "SetImage")
				anImgW->mImage = anImage;
			else if (aCommand == "SetOverImage")
				anImgW->mOverImage = anImage;
		}
		// LabelWidget has no image support -- silently ignore.
		return true;
	}

	if (aCommand == "SetColor")
	{
		// SetColor COLOR_LABEL (255,255,255)  or  SetColor COLOR_LABEL_HILITE (r,g,b)
		if (theTokens.size() != 3)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		if (!mCurrentWidget)
		{
			mLastError = "Missing parse widget";
			return false;
		}
		// Resolve the color *name* against the framework's Widget/ButtonWidget color-index
		// constants (ButtonWidget::COLOR_LABEL=0 .. COLOR_BKG=5), NOT the parser's symbol
		// table: VSSetupSides.txt's own Enum(COLOR_LABEL, ...) block starts after an earlier
		// Enum block, so its numeric values (COLOR_LABEL=3) would land in the wrong slot.
		static const std::map<std::string, int> kColorIndex = {
			{ "COLOR_LABEL", 0 },
			{ "COLOR_LABEL_HILITE", 1 },
			{ "COLOR_DARK_OUTLINE", 2 },
			{ "COLOR_LIGHT_OUTLINE", 3 },
			{ "COLOR_MEDIUM_OUTLINE", 4 },
			{ "COLOR_BKG", 5 },
		};
		int aColorIdx = -1;
		auto aNameItr = kColorIndex.find(theTokens[1]);
		if (aNameItr != kColorIndex.end())
			aColorIdx = aNameItr->second;
		else if (!ResolveInt(theTokens[1], aColorIdx))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		// Parse "(r,g,b)" — the group token includes parens.
		const std::string& aGroup = theTokens[2];
		if (aGroup.size() < 5 || aGroup.front() != '(' || aGroup.back() != ')')
		{
			mLastError = "Invalid Color Format";
			return false;
		}
		std::string aBody = aGroup.substr(1, aGroup.size() - 2);
		int r = 0, g = 0, b = 0;
		if (std::sscanf(aBody.c_str(), "%d,%d,%d", &r, &g, &b) != 3)
		{
			mLastError = "Invalid Color Format";
			return false;
		}
		// Clamp to valid unsigned char range (the original game writes >255 in places, e.g.
		// COLOR_LABEL_HILITE (277,225,108), which the render path treats as 255).
		r = std::max(0, std::min(255, r));
		g = std::max(0, std::min(255, g));
		b = std::max(0, std::min(255, b));
		mCurrentWidget->SetColor(aColorIdx, Color(r, g, b));
		return true;
	}

	if (aCommand == "SetAlign")
	{
		if (theTokens.size() != 2)
		{
			mLastError = "Invalid Number of Parameters";
			return false;
		}
		int anAlign;
		if (!ResolveInt(theTokens[1], anAlign))
		{
			mLastError = "Invalid Parameter Type";
			return false;
		}
		if (LabelWidget* aLabel = dynamic_cast<LabelWidget*>(mCurrentWidget))
			aLabel->mAlign = anAlign;
		else if (ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(mCurrentWidget))
		{
			if (anAlign == 2)
				anAlign = ButtonWidget::BUTTON_LABEL_CENTER;
			aButton->mLabelJustify = anAlign;
		}
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
