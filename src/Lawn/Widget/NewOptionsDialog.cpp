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

#include "../Board.h"
#include "GameButton.h"
#include "../Cutscene.h"
#include "AlmanacDialog.h"
#include "../LawnCommon.h"
#include "../../LawnApp.h"
#include "../System/Music.h"
#include "../../Resources.h"
#include "NewOptionsDialog.h"
#include "../../ConstEnums.h"
#include "../../PvzpLib/PvzpFoley.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "widget/Slider.h"
#include "widget/Checkbox.h"
#include "widget/EditWidget.h"
#include "../../SexyAppFramework/graphics/Font.h"

using namespace Sexy;

static const Color cOptionsTextColor(107, 109, 145);

static const int ADVANCEDOPTIONS_MAX_PAGES = 4;
static const int ADVANCEDOPTIONS_PAGE_Y = 355;
static const int ADVANCEDOPTIONS_SPEED_X = 284;
static const int ADVANCEDOPTIONS_SPEED_Y = 128;
static const int SPEED_MODIFIER_MIN = 2;
static const int SPEED_MODIFIER_MAX = 9;

NewOptionsDialog::NewOptionsDialog(LawnApp* theApp, bool theFromGameSelector, bool theAdvanced) :
	Dialog(nullptr, nullptr, Dialogs::DIALOG_NEWOPTIONS, true, "Options", "", "", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mFromGameSelector = theFromGameSelector;
	mAdvancedMode = theAdvanced;
	mAdvancedPage = theAdvanced ? 1 : 0;
	SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 100));
	mAlmanacButton = MakeButton(NewOptionsDialog::NewOptionsDialog_Almanac, this, "[VIEW_ALMANAC_BUTTON]");
	mRestartButton = MakeButton(NewOptionsDialog::NewOptionsDialog_Restart, this, "[RESTART_LEVEL]");
	mBackToMainButton = MakeButton(NewOptionsDialog::NewOptionsDialog_MainMenu, this, "[MAIN_MENU_BUTTON]");
	mAdvancedButton = MakeButton(NewOptionsDialog::NewOptionsDialog_Advanced, this, "[ADVANCED_OPTIONS_BUTTON]");

	mBackToGameButton = MakeNewButton(
		Dialog::ID_OK,
		this,
		"[BACK_TO_GAME]",
		nullptr,
		IMAGE_OPTIONS_BACKTOGAMEBUTTON0,
		IMAGE_OPTIONS_BACKTOGAMEBUTTON0,
		IMAGE_OPTIONS_BACKTOGAMEBUTTON2
	);
	mBackToGameButton->mTranslateX = 0;
	mBackToGameButton->mTranslateY = 0;
	mBackToGameButton->mTextOffsetX = -2;
	mBackToGameButton->mTextOffsetY = -5;
	mBackToGameButton->mTextDownOffsetX = 0;
	mBackToGameButton->mTextDownOffsetY = 1;
	mBackToGameButton->SetFont(FONT_DWARVENTODCRAFT36GREENINSET);
	mBackToGameButton->SetColor(ButtonWidget::COLOR_LABEL, Color::White);
	mBackToGameButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color::White);
	mBackToGameButton->mHiliteFont = FONT_DWARVENTODCRAFT36BRIGHTGREENINSET;

	mMusicVolumeSlider = new Slider(IMAGE_OPTIONS_SLIDERSLOT, IMAGE_OPTIONS_SLIDERKNOB2, NewOptionsDialog::NewOptionsDialog_MusicVolume, this);
	double aMusicVolume = theApp->GetMusicVolume();
	aMusicVolume = std::max(0.0, std::min(1.0, aMusicVolume));
	mMusicVolumeSlider->SetValue(aMusicVolume);

	mSfxVolumeSlider = new Slider(IMAGE_OPTIONS_SLIDERSLOT, IMAGE_OPTIONS_SLIDERKNOB2, NewOptionsDialog::NewOptionsDialog_SoundVolume, this);
	mSfxVolumeSlider->SetValue(theApp->GetSfxVolume() / 0.65);

	mFullscreenCheckbox = MakeNewCheckbox(NewOptionsDialog::NewOptionsDialog_Fullscreen, this, !theApp->mIsWindowed);
	mHardwareAccelerationCheckbox = MakeNewCheckbox(NewOptionsDialog::NewOptionsDialog_HardwareAcceleration, this, theApp->Is3DAccelerated());

	// Advanced page 1: Debug / Keybinds
	mDebugModeCheckbox = MakeNewCheckbox(-1, this, theApp->mCheatKeys);
	mDebugModeCheckbox->SetVisible(false);
	mBankKeybindsCheckbox = MakeNewCheckbox(-1, this, theApp->mBankKeybinds);
	mBankKeybindsCheckbox->SetVisible(false);
	m09FormatCheckbox = MakeNewCheckbox(-1, this, theApp->mZeroNineBankFormat);
	m09FormatCheckbox->SetVisible(false);

	// Advanced page 2: Gameplay Tweaks
	mSpeedEditWidget = new EditWidget(-1, this);
	mSpeedEditWidget->mMaxChars = 1;
	mSpeedEditWidget->SetFont(FONT_DWARVENTODCRAFT18GREENINSET);
	mSpeedEditWidget->AddWidthCheckFont(FONT_DWARVENTODCRAFT18GREENINSET, IMAGE_OPTIONS_CHECKBOX0->mWidth);
	mSpeedEditWidget->SetText(StrFormat("%d", theApp->mSpeedModifier));
	mSpeedEditWidget->SetVisible(false);
	mAutoCollectSunsCheckbox = MakeNewCheckbox(-1, this, theApp->mAutoCollectSuns);
	mAutoCollectSunsCheckbox->SetVisible(false);
	mAutoCollectCoinsCheckbox = MakeNewCheckbox(-1, this, theApp->mAutoCollectCoins);
	mAutoCollectCoinsCheckbox->SetVisible(false);
	mZombieHealthbarsCheckbox = MakeNewCheckbox(-1, this, theApp->mZombieHealthbars);
	mZombieHealthbarsCheckbox->SetVisible(false);
	mPlantHealthbarsCheckbox = MakeNewCheckbox(-1, this, theApp->mPlantHealthbars);
	mPlantHealthbarsCheckbox->SetVisible(false);

	// Advanced page 3: Resource Packs
	mReloadResourcePacksButton = MakeButton(NewOptionsDialog::NewOptionsDialog_ReloadResourcePacks, this, "[OPTIONS_RELOAD_RESOURCE_PACKS]");
	mReloadResourcePacksButton->SetVisible(false);
	mResourcePackButton = MakeNewButton(NewOptionsDialog::NewOptionsDialog_ResourcePack, this, mApp->GetResourcePackString(), nullptr,
		IMAGE_BLANK, IMAGE_BLANK, IMAGE_BLANK);
	mResourcePackButton->SetFont(FONT_DWARVENTODCRAFT18);
	mResourcePackButton->mColors[ButtonWidget::COLOR_LABEL] = cOptionsTextColor;
	mResourcePackButton->mColors[ButtonWidget::COLOR_LABEL_HILITE] = Color(1, 233, 1);
	mResourcePackButton->SetVisible(false);

	// Advanced page 4: Hardware / Display
	mRealHardwareAccelerationCheckbox = MakeNewCheckbox(NewOptionsDialog::NewOptionsDialog_Real_HardwareAcceleration, this, theApp->Is3DAccelerated());
	mRealHardwareAccelerationCheckbox->SetVisible(false);
	mCustomCursorCheckbox = MakeNewCheckbox(NewOptionsDialog::NewOptionsDialog_CustomCursor, this, theApp->mCustomCursorsEnabled);
	mCustomCursorCheckbox->SetVisible(false);

	// Page navigation arrows
	mLeftPageButton = MakeNewButton(NewOptionsDialog::NewOptionsDialog_LeftPage, this, "", nullptr, IMAGE_ZOMBATAR_PREV_BUTTON,
		IMAGE_ZOMBATAR_PREV_BUTTON_HIGHLIGHT, IMAGE_ZOMBATAR_PREV_BUTTON_HIGHLIGHT);
	mLeftPageButton->SetVisible(false);
	mRightPageButton = MakeNewButton(NewOptionsDialog::NewOptionsDialog_RightPage, this, "", nullptr, IMAGE_ZOMBATAR_NEXT_BUTTON,
		IMAGE_ZOMBATAR_NEXT_BUTTON_HIGHLIGHT, IMAGE_ZOMBATAR_NEXT_BUTTON_HIGHLIGHT);
	mRightPageButton->SetVisible(false);

	mAdvancedButton->SetVisible(false);

	if (mFromGameSelector)
	{
		mRestartButton->SetVisible(false);
		mBackToGameButton->SetLabel("[DIALOG_BUTTON_OK]");
		if (mApp->HasFinishedAdventure() && !mApp->IsTrialStageLocked())
		{
			mBackToMainButton->SetLabel("[CREDITS]");
		}
		else
		{
			mBackToMainButton->SetVisible(false);
		}
		mAdvancedButton->SetVisible(!theAdvanced);
	}
	else
	{
		mAdvancedButton->SetVisible(!theAdvanced);
	}

	if (mAdvancedMode)
	{
		mRestartButton->SetVisible(false);
		mAlmanacButton->SetVisible(false);
		mBackToMainButton->SetVisible(false);
		mAdvancedButton->SetVisible(false);
		mBackToGameButton->SetLabel("[DIALOG_BUTTON_BACK]");
		mBackToGameButton->mId = NewOptionsDialog::NewOptionsDialog_Back;
	}

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ICE ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN ||
		mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
	{
		mRestartButton->SetVisible(false);
	}
	if (mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO && !mApp->mBoard->mCutScene->IsSurvivalRepick())
	{
		mRestartButton->SetVisible(false);
	}
	if (!mApp->CanShowAlmanac() ||
		mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN ||
		mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM ||
		mFromGameSelector)
	{
		mAlmanacButton->SetVisible(false);
	}
	if ((!mRestartButton->mVisible || !mAlmanacButton->mVisible) && !mAdvancedMode)
	{
		LawnStoneButton* button;
		if (!mRestartButton->mVisible)
			button = mRestartButton;
		else
			button = mAlmanacButton;
		mAdvancedButton->Resize(button->mX, button->mY, button->mWidth, button->mHeight);
	}
}

NewOptionsDialog::~NewOptionsDialog()
{
	delete mMusicVolumeSlider;
	delete mSfxVolumeSlider;
	delete mFullscreenCheckbox;
	delete mHardwareAccelerationCheckbox;
	delete mAlmanacButton;
	delete mRestartButton;
	delete mBackToMainButton;
	delete mAdvancedButton;
	delete mLeftPageButton;
	delete mRightPageButton;
	delete mReloadResourcePacksButton;
	delete mResourcePackButton;
	delete mBackToGameButton;
	delete mDebugModeCheckbox;
	delete mBankKeybindsCheckbox;
	delete m09FormatCheckbox;
	delete mSpeedEditWidget;
	delete mAutoCollectSunsCheckbox;
	delete mAutoCollectCoinsCheckbox;
	delete mZombieHealthbarsCheckbox;
	delete mPlantHealthbarsCheckbox;
	delete mRealHardwareAccelerationCheckbox;
	delete mCustomCursorCheckbox;
}

int NewOptionsDialog::GetPreferredHeight(int theWidth)
{
	(void)theWidth;
	return IMAGE_OPTIONS_MENUBACK->mWidth;
}

void NewOptionsDialog::AddedToManager(Sexy::WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	AddWidget(mAlmanacButton);
	AddWidget(mRestartButton);
	AddWidget(mBackToMainButton);
	AddWidget(mAdvancedButton);
	AddWidget(mMusicVolumeSlider);
	AddWidget(mSfxVolumeSlider);
	AddWidget(mHardwareAccelerationCheckbox);
	AddWidget(mFullscreenCheckbox);
	AddWidget(mBackToGameButton);
	AddWidget(mLeftPageButton);
	AddWidget(mRightPageButton);
	AddWidget(mReloadResourcePacksButton);
	AddWidget(mResourcePackButton);
	AddWidget(mDebugModeCheckbox);
	AddWidget(mBankKeybindsCheckbox);
	AddWidget(m09FormatCheckbox);
	AddWidget(mSpeedEditWidget);
	AddWidget(mAutoCollectSunsCheckbox);
	AddWidget(mAutoCollectCoinsCheckbox);
	AddWidget(mZombieHealthbarsCheckbox);
	AddWidget(mPlantHealthbarsCheckbox);
	AddWidget(mRealHardwareAccelerationCheckbox);
	AddWidget(mCustomCursorCheckbox);
}

void NewOptionsDialog::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mAlmanacButton);
	RemoveWidget(mMusicVolumeSlider);
	RemoveWidget(mSfxVolumeSlider);
	RemoveWidget(mHardwareAccelerationCheckbox);
	RemoveWidget(mFullscreenCheckbox);
	RemoveWidget(mBackToMainButton);
	RemoveWidget(mRestartButton);
	RemoveWidget(mAdvancedButton);
	RemoveWidget(mLeftPageButton);
	RemoveWidget(mRightPageButton);
	RemoveWidget(mReloadResourcePacksButton);
	RemoveWidget(mResourcePackButton);
	RemoveWidget(mDebugModeCheckbox);
	RemoveWidget(mBankKeybindsCheckbox);
	RemoveWidget(m09FormatCheckbox);
	RemoveWidget(mSpeedEditWidget);
	RemoveWidget(mAutoCollectSunsCheckbox);
	RemoveWidget(mAutoCollectCoinsCheckbox);
	RemoveWidget(mZombieHealthbarsCheckbox);
	RemoveWidget(mPlantHealthbarsCheckbox);
	RemoveWidget(mRealHardwareAccelerationCheckbox);
	RemoveWidget(mCustomCursorCheckbox);
}

void NewOptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mMusicVolumeSlider->Resize(199, 116, 135, 40);
	mSfxVolumeSlider->Resize(199, 143, 135, 40);
	mHardwareAccelerationCheckbox->Resize(283, 175, 46, 45);
	mFullscreenCheckbox->Resize(284, 206, 46, 45);
	mAlmanacButton->Resize(107, 241, 209, 46);
	mRestartButton->Resize(mAlmanacButton->mX, mAlmanacButton->mY + 43, 209, 46);
	mBackToMainButton->Resize(mRestartButton->mX, mRestartButton->mY + 43, 209, 46);
	mAdvancedButton->Resize(mRestartButton->mX, mRestartButton->mY + 43, 209, 46);
	mBackToGameButton->Resize(30, 381, mBackToGameButton->mWidth, mBackToGameButton->mHeight);

	// Page navigation
	mLeftPageButton->Resize(100, ADVANCEDOPTIONS_PAGE_Y - 25, IMAGE_ZOMBATAR_PREV_BUTTON->mWidth, IMAGE_ZOMBATAR_PREV_BUTTON->mHeight);
	mRightPageButton->Resize(280, ADVANCEDOPTIONS_PAGE_Y - 25, IMAGE_ZOMBATAR_NEXT_BUTTON->mWidth, IMAGE_ZOMBATAR_NEXT_BUTTON->mHeight);

	// Page 1: Debug / Keybinds
	mDebugModeCheckbox->Resize(284, 148, 46, 39);
	mBankKeybindsCheckbox->Resize(mDebugModeCheckbox->mX, mDebugModeCheckbox->mY + 40, 46, 39);
	m09FormatCheckbox->Resize(mBankKeybindsCheckbox->mX, mBankKeybindsCheckbox->mY + 40, 46, 39);

	// Page 2: Gameplay Tweaks
	mSpeedEditWidget->Resize(ADVANCEDOPTIONS_SPEED_X + 9, ADVANCEDOPTIONS_SPEED_Y - 4, IMAGE_OPTIONS_CHECKBOX0->mWidth, IMAGE_OPTIONS_CHECKBOX0->mHeight + 4);
	mAutoCollectSunsCheckbox->Resize(284, ADVANCEDOPTIONS_SPEED_Y + 40, 46, 39);
	mAutoCollectCoinsCheckbox->Resize(mAutoCollectSunsCheckbox->mX, mAutoCollectSunsCheckbox->mY + 40, 46, 39);
	mZombieHealthbarsCheckbox->Resize(mAutoCollectCoinsCheckbox->mX, mAutoCollectCoinsCheckbox->mY + 40, 46, 39);
	mPlantHealthbarsCheckbox->Resize(mZombieHealthbarsCheckbox->mX, mZombieHealthbarsCheckbox->mY + 40, 46, 39);

	// Page 3: Resource Packs
	mReloadResourcePacksButton->Resize(mWidth / 2 - 130, ADVANCEDOPTIONS_SPEED_Y, 260, 46);
	mResourcePackButton->Resize(mWidth / 2 + 15, mReloadResourcePacksButton->mY + 50, 0, FONT_DWARVENTODCRAFT18->GetHeight());
	ResizeResourcePackButton();

	// Page 4: Hardware / Display
	mRealHardwareAccelerationCheckbox->Resize(ADVANCEDOPTIONS_SPEED_X, ADVANCEDOPTIONS_SPEED_Y, 46, 39);
	mCustomCursorCheckbox->Resize(mRealHardwareAccelerationCheckbox->mX, mRealHardwareAccelerationCheckbox->mY + 40, 46, 39);

	if ((!mRestartButton->mVisible || !mAlmanacButton->mVisible) && !mAdvancedMode)
	{
		LawnStoneButton* button;
		if (!mRestartButton->mVisible)
			button = mRestartButton;
		else
			button = mAlmanacButton;
		mAdvancedButton->Resize(button->mX, button->mY, button->mWidth, button->mHeight);
	}

	if (mFromGameSelector)
	{
		mMusicVolumeSlider->mY += 5;
		mSfxVolumeSlider->mY += 10;
		mHardwareAccelerationCheckbox->mY += 15;
		mFullscreenCheckbox->mY += 20;
	}

	if (mAdvancedMode)
	{
		mMusicVolumeSlider->SetVisible(false);
		mSfxVolumeSlider->SetVisible(false);
		mHardwareAccelerationCheckbox->SetVisible(false);
		mFullscreenCheckbox->SetVisible(false);
		mLeftPageButton->SetVisible(true);
		mRightPageButton->SetVisible(true);
		UpdateAdvancedPage();
	}

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
	{
		mAlmanacButton->mY += 43;
	}
}

void NewOptionsDialog::Draw(Sexy::Graphics* g)
{
	g->DrawImage(IMAGE_OPTIONS_MENUBACK, 0, 0);

	int aMusicOffset = 0;
	int aSfxOffset = 0;
	int a3DAccelOffset = 0;
	int aFullScreenOffset = 0;
	if (mFromGameSelector)
	{
		aMusicOffset = 5;
		aSfxOffset = 10;
		a3DAccelOffset = 15;
		aFullScreenOffset = 20;
	}

	if (!mAdvancedMode)
	{
		float aFontScale = static_cast<float>(mApp->GetDouble("OPTION_DLG_LABEL_FONT_SCALE", 1.0));
		int aSliderLabelsX = mApp->GetInteger("OPTION_DLG_SLIDER_LABELS_OFFSET_X", 186);
		int aCheckboxLabelsX = mApp->GetInteger("OPTION_DLG_CHECKBOX_LABELS_OFFSET_X", 274);
		if (aFontScale != 1.0f)
			g->SetScale(aFontScale, aFontScale, 0.0f, 0.0f);
		PvzpDrawString(g, mApp->GetString("OPTIONS_MUSIC_LABEL", "Music"), aSliderLabelsX, 140 + aMusicOffset, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
		PvzpDrawString(g, mApp->GetString("OPTIONS_SOUNDFX", "Sound FX"), aSliderLabelsX, 167 + aSfxOffset, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
		PvzpDrawString(g, mApp->GetString("OPTIONS_3D_ACCELERATION", "3D Acceleration"), aCheckboxLabelsX, 197 + a3DAccelOffset, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
		PvzpDrawString(g, mApp->GetString("OPTIONS_FULL_SCREEN", "Full Screen"), aCheckboxLabelsX, 229 + aFullScreenOffset, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
		if (aFontScale != 1.0f)
			g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		switch (mAdvancedPage)
		{
		case 1:
			PvzpDrawString(g, mApp->GetString("OPTIONS_DEBUG_MODE", "Debug Mode"), mDebugModeCheckbox->mX - 6, mDebugModeCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_SEED_BANK_KEYBINDS", "Seed Bank Keybinds"), mBankKeybindsCheckbox->mX - 6, mBankKeybindsCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			{
				std::string aFormatLabel = m09FormatCheckbox->mChecked ? "1-0" : "0-9";
				PvzpDrawString(g, mApp->GetString("OPTIONS_SEED_BANK_KEYBIND", "Seed Bank Format") + " (" + aFormatLabel + ")", m09FormatCheckbox->mX - 6, m09FormatCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			}
			PvzpDrawString(g, mApp->GetString("OPTIONS_SHOVEL_KEYBIND", "Shovel Keybind"), mWidth / 2, m09FormatCheckbox->mY + 55, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_CENTER);
			break;
		case 2:
			g->DrawImage(IMAGE_OPTIONS_CHECKBOX0, ADVANCEDOPTIONS_SPEED_X, ADVANCEDOPTIONS_SPEED_Y);
			PvzpDrawString(g, mApp->GetString("OPTIONS_SPEED_MODIFIER", "Speed Modifier"), ADVANCEDOPTIONS_SPEED_X - 6, ADVANCEDOPTIONS_SPEED_Y + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_AUTO_COLLECT_SUNS", "Auto Collect Suns"), mAutoCollectSunsCheckbox->mX - 6, mAutoCollectSunsCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_AUTO_COLLECT_COINS", "Auto Collect Coins"), mAutoCollectCoinsCheckbox->mX - 6, mAutoCollectCoinsCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_ZOMBIE_HEALTHBARS", "Zombie Healthbars"), mZombieHealthbarsCheckbox->mX - 6, mZombieHealthbarsCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_PLANT_HEALTHBARS", "Plant Healthbars"), mPlantHealthbarsCheckbox->mX - 6, mPlantHealthbarsCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			break;
		case 3:
			PvzpDrawString(g, mApp->GetString("OPTIONS_RESOURCE_PACK", "Resource Pack"), mResourcePackButton->mX - 6, mResourcePackButton->mY + 23, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			break;
		case 4:
			PvzpDrawString(g, mApp->GetString("OPTIONS_ACTUAL_ACCELERATION", "Real Hardware Acceleration"), mRealHardwareAccelerationCheckbox->mX - 6, mRealHardwareAccelerationCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			PvzpDrawString(g, mApp->GetString("OPTIONS_CUSTOM_CURSOR", "Custom Cursor"), mCustomCursorCheckbox->mX - 6, mCustomCursorCheckbox->mY + 22, FONT_DWARVENTODCRAFT18, cOptionsTextColor, DrawStringJustification::DS_ALIGN_RIGHT);
			break;
		}
		PvzpDrawString(g, "Page " + std::to_string(mAdvancedPage) + " / " + std::to_string(ADVANCEDOPTIONS_MAX_PAGES), mWidth / 2, ADVANCEDOPTIONS_PAGE_Y, FONT_DWARVENTODCRAFT18GREENINSET, Color::White, DrawStringJustification::DS_ALIGN_CENTER);
	}
}

void NewOptionsDialog::SliderVal(int theId, double theVal)
{
	switch (theId)
	{
	case NewOptionsDialog::NewOptionsDialog_MusicVolume:
		mApp->SetMusicVolume(theVal);
		mApp->mSoundSystem->RehookupSoundWithMusicVolume();
		break;

	case NewOptionsDialog::NewOptionsDialog_SoundVolume:
		mApp->SetSfxVolume(theVal * 0.65);
		mApp->mSoundSystem->RehookupSoundWithMusicVolume();
		if (!mSfxVolumeSlider->mDragging)
		{
			mApp->PlaySample(SOUND_BUTTONCLICK);
		}
		break;
	}
}

void NewOptionsDialog::CheckboxChecked(int theId, bool checked)
{
	switch (theId)
	{
	case NewOptionsDialog::NewOptionsDialog_Fullscreen:
		if (!checked && mApp->mForceFullscreen)
		{
			mApp->DoDialog(
				Dialogs::DIALOG_COLORDEPTH_EXP,
				true,
				mApp->GetString("NO_WINDOWED_MODE", "No Windowed Mode"),
				mApp->GetString("INVALID_WINDOWS_MODE",
					"Windowed mode is only available if your desktop was running in either\n"
					"16 bit or 32 bit color mode when you started the game.\n\n"
					"If you'd like to run in Windowed mode then you need to quit the game and switch your desktop to 16 or 32 bit color mode."),
				"[DIALOG_BUTTON_OK]",
				Dialog::BUTTONS_FOOTER
			);

			mFullscreenCheckbox->SetChecked(true, false);
		}
		break;

	case NewOptionsDialog::NewOptionsDialog_HardwareAcceleration:
		if (checked)
		{
			if (!mApp->Is3DAccelerationSupported())
			{
				mHardwareAccelerationCheckbox->SetChecked(false, false);
				mApp->DoDialog(
					Dialogs::DIALOG_INFO,
					true,
					mApp->GetString("NOT_SUPPORTED", "Not Supported"),
					mApp->GetString("HARDWARE_ACCELERATION_NOT_SUPPORTED",
						"Hardware Acceleration cannot be enabled on this computer.\n\n"
						"Your video card does not\n"
						"meet the minimum requirements\n"
						"for this game."),
					"[DIALOG_BUTTON_OK]",
					Dialog::BUTTONS_FOOTER
				);
			}
			else if (!mApp->Is3DAccelerationRecommended())
			{
				mApp->DoDialog(
					Dialogs::DIALOG_INFO,
					true,
					mApp->GetString("DIALOG_WARNING", "Warning"),
					mApp->GetString("SLOW_PERFORMANCE",
						"Your video card may not fully support this feature.\n\n"
						"If you experience slower performance, please disable Hardware Acceleration."),
					"[DIALOG_BUTTON_OK]",
					Dialog::BUTTONS_FOOTER
				);
			}
		}
		break;
	}
}

void NewOptionsDialog::EditWidgetText(int theId, const std::string& theString)
{
	(void)theId;
	int aVal = 2;
	try { aVal = std::stoi(theString); } catch (...) {}
	aVal = std::max(SPEED_MODIFIER_MIN, std::min(SPEED_MODIFIER_MAX, aVal));
	mSpeedEditWidget->SetText(StrFormat("%d", aVal));
	mApp->mSpeedModifier = aVal;
}

void NewOptionsDialog::KeyDown(Sexy::KeyCode theKey)
{
	if (mApp->mBoard)
	{
		mApp->mBoard->DoTypingCheck(theKey);
	}

	if (theKey == KeyCode::KEYCODE_SPACE || theKey == KeyCode::KEYCODE_RETURN)
	{
		if (mAdvancedMode)
			ButtonDepress(NewOptionsDialog::NewOptionsDialog_Back);
		else
			Dialog::ButtonDepress(Dialog::ID_OK);
	}
	else if (theKey == KeyCode::KEYCODE_ESCAPE)
	{
		Dialog::ButtonDepress(Dialog::ID_CANCEL);
	}
}

void NewOptionsDialog::UpdateAdvancedPage()
{
	if (mAdvancedPage <= 1)
		mLeftPageButton->SetVisible(false);
	else
		mLeftPageButton->SetVisible(true);

	if (mAdvancedPage >= ADVANCEDOPTIONS_MAX_PAGES)
		mRightPageButton->SetVisible(false);
	else
		mRightPageButton->SetVisible(true);

	// Hide all advanced widgets
	mDebugModeCheckbox->SetVisible(false);
	mBankKeybindsCheckbox->SetVisible(false);
	m09FormatCheckbox->SetVisible(false);
	mSpeedEditWidget->SetVisible(false);
	mAutoCollectSunsCheckbox->SetVisible(false);
	mAutoCollectCoinsCheckbox->SetVisible(false);
	mZombieHealthbarsCheckbox->SetVisible(false);
	mPlantHealthbarsCheckbox->SetVisible(false);
	mReloadResourcePacksButton->SetVisible(false);
	mResourcePackButton->SetVisible(false);
	mRealHardwareAccelerationCheckbox->SetVisible(false);
	mCustomCursorCheckbox->SetVisible(false);

	switch (mAdvancedPage)
	{
	case 1:
		mDebugModeCheckbox->SetVisible(true);
		mBankKeybindsCheckbox->SetVisible(true);
		m09FormatCheckbox->SetVisible(true);
		break;
	case 2:
		mSpeedEditWidget->SetVisible(true);
		mAutoCollectSunsCheckbox->SetVisible(true);
		mAutoCollectCoinsCheckbox->SetVisible(true);
		mZombieHealthbarsCheckbox->SetVisible(true);
		mPlantHealthbarsCheckbox->SetVisible(true);
		break;
	case 3:
		mReloadResourcePacksButton->SetVisible(true);
		mResourcePackButton->SetVisible(true);
		break;
	case 4:
		mRealHardwareAccelerationCheckbox->SetVisible(true);
		mCustomCursorCheckbox->SetVisible(true);
		break;
	}
}

void NewOptionsDialog::ResizeResourcePackButton()
{
	mResourcePackButton->Resize(mResourcePackButton->mX, mResourcePackButton->mY, mResourcePackButton->mFont->StringWidth(PvzpStringTranslate(mResourcePackButton->mLabel)), mResourcePackButton->mHeight);
}

void NewOptionsDialog::ButtonPress(int theId)
{
	(void)theId;
	mApp->PlaySample(SOUND_GRAVEBUTTON);
}

void NewOptionsDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	switch (theId)
	{
	case NewOptionsDialog::NewOptionsDialog_Almanac:
	{
		AlmanacDialog* aDialog = mApp->DoAlmanacDialog(SeedType::SEED_NONE, ZombieType::ZOMBIE_INVALID);
		aDialog->WaitForResult(true);
		break;
	}

	case NewOptionsDialog::NewOptionsDialog_Advanced:
	{
		mApp->KillNewOptionsDialog();
		mApp->DoNewOptions(mFromGameSelector, true);
		mApp->PlaySample(SOUND_BUTTONCLICK);
		break;
	}

	case NewOptionsDialog::NewOptionsDialog_MainMenu:
	{
		if (mFromGameSelector)
		{
			mApp->KillNewOptionsDialog();
			mApp->KillGameSelector();
			mApp->ShowAwardScreen(AwardType::AWARD_CREDITS_ZOMBIENOTE, false);
		}
		else if (mApp->mBoard && mApp->mBoard->NeedSaveGame())
		{
			mApp->DoConfirmBackToMain();
		}
		else if (mApp->mBoard && mApp->mBoard->mCutScene && mApp->mBoard->mCutScene->IsSurvivalRepick())
		{
			mApp->DoConfirmBackToMain();
		}
		else
		{
			mApp->mBoardResult = BoardResult::BOARDRESULT_QUIT;
			mApp->DoBackToMain();
		}
		break;
	}

	case NewOptionsDialog::NewOptionsDialog_Restart:
	{
		if (mApp->mBoard)
		{
			std::string aDialogTitle;
			std::string aDialogMessage;
			if (mApp->IsPuzzleMode())
			{
				aDialogTitle = "[RESTART_PUZZLE_HEADER]";
				aDialogMessage = "[RESTART_PUZZLE_BODY]";
			}
			else if (mApp->IsChallengeMode())
			{
				aDialogTitle = "[RESTART_CHALLENGE_HEADER]";
				aDialogMessage = "[RESTART_CHALLENGE_BODY]";
			}
			else if (mApp->IsSurvivalMode())
			{
				aDialogTitle = "[RESTART_SURVIVAL_HEADER]";
				aDialogMessage = "[RESTART_SURVIVAL_BODY]";
			}
			else
			{
				aDialogTitle = "[RESTART_LEVEL_HEADER]";
				aDialogMessage = "[RESTART_LEVEL_BODY]";
			}

			LawnDialog* aDialog = (LawnDialog*)mApp->DoDialog(Dialogs::DIALOG_CONFIRM_RESTART, true, aDialogTitle, aDialogMessage, "", Dialog::BUTTONS_YES_NO);
			aDialog->mLawnYesButton->mLabel = mApp->GetString("RESTART_LABEL", "RESTART");
			aDialog->mLawnNoButton->mLabel = PvzpStringTranslate("[DIALOG_BUTTON_CANCEL]");

			if (aDialog->WaitForResult(true) == Dialog::ID_YES)
			{
				mApp->mMusic->StopAllMusic();
				mApp->mSoundSystem->CancelPausedFoley();
				mApp->KillNewOptionsDialog();
				mApp->mBoardResult = BoardResult::BOARDRESULT_RESTART;
				mApp->mSawYeti = mApp->mBoard->mKilledYeti;
				mApp->PreNewGame(mApp->mGameMode, false);
			}
		}
		break;
	}

	case NewOptionsDialog::NewOptionsDialog_Update:
		mApp->CheckForUpdates();
		break;

	case NewOptionsDialog::NewOptionsDialog_ReloadResourcePacks:
		mApp->ReloadResourcePacks();
		mResourcePackButton->mLabel = mApp->GetResourcePackString();
		ResizeResourcePackButton();
		break;

	case NewOptionsDialog::NewOptionsDialog_ResourcePack:
		mApp->SwitchResourcePack();
		mResourcePackButton->mLabel = mApp->GetResourcePackString();
		ResizeResourcePackButton();
		break;

	case NewOptionsDialog::NewOptionsDialog_LeftPage:
		mAdvancedPage--;
		UpdateAdvancedPage();
		break;

	case NewOptionsDialog::NewOptionsDialog_RightPage:
		mAdvancedPage++;
		UpdateAdvancedPage();
		break;

	case NewOptionsDialog::NewOptionsDialog_Back:
		mApp->KillNewOptionsDialog();
		mApp->DoNewOptions(mFromGameSelector, false);
		mApp->PlaySample(SOUND_BUTTONCLICK);
		break;
	}
}
