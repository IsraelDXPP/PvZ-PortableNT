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

#include "Music.h"
#include "../Board.h"
#include "PlayerInfo.h"
#include "../../LawnApp.h"
#include "paklib/PakInterface.h"
#include "../../PvzpLib/PvzpDebug.h"
#include "../../PvzpLib/PvzpCommon.h"
#include "sound/SDLMusicInterface.h"

using namespace Sexy;

Music::Music()
{
	mApp = (LawnApp*)gSexyAppBase;
	mMusicInterface = gSexyAppBase->mMusicInterface.get();
	mCurMusicTune = MusicTune::MUSIC_TUNE_NONE;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
	mBurstOverride = -1;
	mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
	mQueuedDrumTrackPackedOrder = -1;
	mBaseBPM = 155;
	mBaseModSpeed = 3;
	mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
	mPauseOffset = 0;
	mPauseOffsetDrums = 0;
	mPaused = false;
	mMusicDisabled = false;
	mFadeOutCounter = 0;
	mFadeOutDuration = 0;
}

MusicFileData gMusicFileData[MusicFile::NUM_MUSIC_FILES];

struct MusicLoadEntry
{
	MusicFile					mMusicFile;
	std::string_view			mFileName;
};

struct MusicFileMapping
{
	MusicFile		mFile;
	const char*		mBaseName;
	const char*		mDefaultPath;
};

static constexpr int MUSIC_LOADING_TASK_WEIGHT = 3500;
static constexpr MusicLoadEntry MUSIC_LOADING_FILES[] = {
	{MusicFile::MUSIC_FILE_DRUMS, "sounds/mainmusic.mo3"},
	{MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN, "sounds/ZombiesOnYourLawn.ogg"}
};

static const MusicFileMapping gMusicFileMappings[] = {
	{MusicFile::MUSIC_FILE_MAIN_MUSIC, "mainmusic", "sounds/mainmusic.mo3"},
	{MusicFile::MUSIC_FILE_DRUMS, "mainmusic", "sounds/mainmusic.mo3"},
	{MusicFile::MUSIC_FILE_HIHATS, "mainmusic_hihats", "sounds/mainmusic_hihats.mo3"},
	{MusicFile::MUSIC_FILE_DAY_GRASSWALK, "Grasswalk", "Music/Grasswalk.ogg"},
	{MusicFile::MUSIC_FILE_ROOF_GRAZETHEROOF, "GrazeTheRoof", "Music/GrazeTheRoof.ogg"},
	{MusicFile::MUSIC_FILE_NIGHT_MOONGRAINS, "MoonGrains", "Music/MoonGrains.ogg"},
	{MusicFile::MUSIC_FILE_POOL_WATERYGRAVES, "WateryGraves", "Music/WateryGraves.ogg"},
	{MusicFile::MUSIC_FILE_FOG_RIGORMORMIST, "RigorMormist", "Music/RigorMormist.ogg"},
	{MusicFile::MUSIC_FILE_CHOOSE_YOUR_SEEDS, "ChooseYourSeeds", "Music/ChooseYourSeeds.ogg"},
	{MusicFile::MUSIC_FILE_ZEN_GARDEN, "ZenGarden", "Music/ZenGarden.ogg"},
	{MusicFile::MUSIC_FILE_PUZZLE_CEREBRAWL, "Cerebrawl", "Music/Cerebrawl.ogg"},
	{MusicFile::MUSIC_FILE_MINIGAME_LOONBOON, "LoonBoon", "Music/LoonBoon.ogg"},
	{MusicFile::MUSIC_FILE_CONVEYER, "UltimateBattle", "Music/UltimateBattle.ogg"},
	{MusicFile::MUSIC_FILE_FINAL_BOSS_BRAINIAC_MANIAC, "BrainiacManiac", "Music/BrainiacManiac.ogg"},
	{MusicFile::MUSIC_FILE_MAIN_MENU, "MainMenu", "Music/MainMenu.ogg"},
	{MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN, "ZombiesOnYourLawn", "sounds/ZombiesOnYourLawn.ogg"},
};

static const int gNumMusicMappings = sizeof(gMusicFileMappings) / sizeof(gMusicFileMappings[0]);

static const MusicFileMapping* FindMusicMapping(MusicFile theFile)
{
	for (int i = 0; i < gNumMusicMappings; ++i)
	{
		if (gMusicFileMappings[i].mFile == theFile)
			return &gMusicFileMappings[i];
	}
	return nullptr;
}

static bool IsPerTuneFileLoaded(MusicFile theFile)
{
	SDLMusicInterface* anSDL = (SDLMusicInterface*)gSexyAppBase->mMusicInterface.get();
	return anSDL->mMusicMap.find((int)theFile) != anSDL->mMusicMap.end();
}

const int Music::MUSIC_LOADING_TASKS = MUSIC_LOADING_TASK_WEIGHT * static_cast<int>(sizeof(MUSIC_LOADING_FILES) / sizeof(MUSIC_LOADING_FILES[0]));

bool Music::PvzpLoadMusic(MusicFile theMusicFile, std::string_view theFileName)
{
	Mix_Music* aHMusic = 0;
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface.get();
	std::string aFileName(theFileName);
	std::string anExt;

	size_t aDot = aFileName.rfind('.');
	if (aDot != std::string::npos)
		anExt = StringToLower(aFileName.substr(aDot + 1));

	PFILE* pFile = p_fopen(aFileName.c_str(), "rb");
	if (pFile == nullptr)
		return false;

	p_fseek(pFile, 0, SEEK_END);
	int aSize = p_ftell(pFile);
	p_fseek(pFile, 0, SEEK_SET);
	void* aData = operator new[](aSize);
	p_fread(aData, sizeof(char), aSize, pFile);
	p_fclose(pFile);

	aHMusic = Mix_LoadMUS_RW(SDL_RWFromMem(aData, aSize), 1);
	if (aHMusic != 0)
	{
		gMusicFileData[theMusicFile].mFileData = (unsigned int*)aData;
	}
	else
	{
		delete[] (char *)aData;
	}

	if (aHMusic == 0)
		return false;

	SDLMusicInfo aMusicInfo;
	aMusicInfo.mHMusic = aHMusic;
	anSDL->mMusicMap.insert(SDLMusicMap::value_type(theMusicFile, aMusicInfo));
	return true;
}

void Music::SetupVolumeForTune(MusicTune theMusicTune, float theDrumsVolume, float theHihatsVolume)
{
	constexpr const int TRACK_COUNT = 30;
	int aMainEnd = 29;
	int aDrumsStart = -1, aDrumsEnd = -1;
	int aHihatsStart1 = -1, aHihatsEnd1 = -1, aHihatsStart2 = -1, aHihatsEnd2 = -1;

	switch (theMusicTune)
	{
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:
		aMainEnd = 23;
		aDrumsStart = 24;	aDrumsEnd = 26;
		aHihatsStart1 = 27;	aHihatsEnd1 = 27;
		break;
	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:
		aMainEnd = 17;
		aDrumsStart = 18;	aDrumsEnd = 28;
		aHihatsStart1 = 18;	aHihatsEnd1 = 24;	aHihatsStart2 = 29;	aHihatsEnd2 = 29;
		break;
	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:
		aMainEnd = 15;
		aDrumsStart = 16;	aDrumsEnd = 22;
		aHihatsStart1 = 23;	aHihatsEnd1 = 23;
		break;
	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:
		aMainEnd = 17;
		aDrumsStart = 18;	aDrumsEnd = 20;
		aHihatsStart1 = 21;	aHihatsEnd1 = 21;
		break;
	default:
		break;
	}

	Mix_Music* aHMusic = GetMusicHandle(MusicFile::MUSIC_FILE_MAIN_MUSIC);
	for (int aTrack = 0; aTrack < TRACK_COUNT; aTrack++)
	{
		float aVolume;
		if (aTrack <= aMainEnd)
			aVolume = 1.0f;
		else
		{
			bool isDrums = (aTrack >= aDrumsStart && aTrack <= aDrumsEnd);
			bool isHihats = (aTrack >= aHihatsStart1 && aTrack <= aHihatsEnd1) ||
							(aTrack >= aHihatsStart2 && aTrack <= aHihatsEnd2);
			if (isDrums && isHihats)
				aVolume = std::max(theDrumsVolume, theHihatsVolume);
			else if (isDrums)
				aVolume = theDrumsVolume;
			else if (isHihats)
				aVolume = theHihatsVolume;
			else
				aVolume = 0.0f;
		}
		Mix_ModMusicStreamSetChannelVolume(aHMusic, aTrack, (int)(aVolume * 128));
	}
}

void Music::LoadSong(MusicFile theMusicFile, std::string_view theFileName)
{
	PvzpHesitationTrace("preloadsong");
	if (!PvzpLoadMusic(theMusicFile, theFileName))
	{
		PvzpTrace("music failed to load\n");
		// mMusicDisabled = true; // QEWide: don't disable ALL music just because one file failed
	}
	else
	{
		PvzpHesitationTrace("song '%.*s'", static_cast<int>(theFileName.size()), theFileName.data());
	}
}

void Music::MusicTitleScreenInit()
{
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MUSIC, "sounds/mainmusic.mo3");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MENU, "Music/MainMenu.ogg");
	MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME);
}

void Music::MusicInit()
{
	for (const auto& aMusic : MUSIC_LOADING_FILES)
	{
		LoadMusicFromResourcePack(aMusic.mMusicFile, aMusic.mFileName);
		mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	}
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MUSIC, "sounds/mainmusic.mo3");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_HIHATS, "sounds/mainmusic_hihats.mo3");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MENU, "Music/MainMenu.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_DAY_GRASSWALK, "Music/Grasswalk.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_NIGHT_MOONGRAINS, "Music/MoonGrains.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_POOL_WATERYGRAVES, "Music/WateryGraves.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_FOG_RIGORMORMIST, "Music/RigorMormist.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_ROOF_GRAZETHEROOF, "Music/GrazeTheRoof.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CHOOSE_YOUR_SEEDS, "Music/ChooseYourSeeds.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_ZEN_GARDEN, "Music/ZenGarden.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_PUZZLE_CEREBRAWL, "Music/Cerebrawl.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MINIGAME_LOONBOON, "Music/LoonBoon.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CONVEYER, "Music/UltimateBattle.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_FINAL_BOSS_BRAINIAC_MANIAC, "Music/BrainiacManiac.ogg");
	mApp->mCompletedLoadingThreadTasks += MUSIC_LOADING_TASK_WEIGHT;
}

void Music::MusicCreditScreenInit()
{
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface.get();
	if (anSDL->mMusicMap.find((int)MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN) == anSDL->mMusicMap.end())
		LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN, "sounds/ZombiesOnYourLawn.ogg");
}

std::string Music::FindMusicInResourcePack(const char* theBaseName)
{
	if (!mApp || mApp->mResourcePack.empty())
		return "";

	std::string aMusicPath = mApp->mResourcePackPath + "/" + mApp->mResourcePack + "/Music/";

	static const char* aAudioFormats[] = { ".ogg", ".mp3", ".wav", ".mo3" };
	static const int aNumFormats = 4;

	for (int i = 0; i < aNumFormats; ++i)
	{
		std::string aFullPath = aMusicPath + theBaseName + aAudioFormats[i];
		if (mApp->FileExists(aFullPath))
		{
			PvzpTrace("Found custom music in resource pack: %s", aFullPath.c_str());
			return aFullPath;
		}
	}

	return "";
}

static std::string FindMusicLocal(const char* theBaseName)
{
	static const char* aAudioFormats[] = { ".ogg", ".mp3", ".wav", ".mo3" };
	static const int aNumFormats = 4;

	for (int i = 0; i < aNumFormats; ++i)
	{
		std::string aFullPath = std::string("Music/") + theBaseName + aAudioFormats[i];
		if (gSexyAppBase->FileExists(aFullPath))
			return aFullPath;
	}

	return "";
}

void Music::LoadMusicFromResourcePack(MusicFile theMusicFile, std::string_view theDefaultPath)
{
	if (!mApp || !mApp->mResourceManager)
	{
		LoadSong(theMusicFile, theDefaultPath);
		return;
	}

	const MusicFileMapping* aMapping = FindMusicMapping(theMusicFile);
	if (aMapping == nullptr)
	{
		LoadSong(theMusicFile, theDefaultPath);
		return;
	}

	std::string aCustomPath = FindMusicInResourcePack(aMapping->mBaseName);
	if (!aCustomPath.empty())
	{
		PvzpTrace("Loading music from resource pack: %s", aCustomPath.c_str());
		LoadSong(theMusicFile, aCustomPath);
		return;
	}

	std::string aLocalPath = FindMusicLocal(aMapping->mBaseName);
	if (!aLocalPath.empty())
	{
		PvzpTrace("Loading music from local: %s", aLocalPath.c_str());
		LoadSong(theMusicFile, aLocalPath);
		return;
	}

	if (mApp->FileExists(std::string(theDefaultPath)))
	{
		LoadSong(theMusicFile, theDefaultPath);
	}
}

void Music::UnloadResourcePackMusic()
{
	MusicTune aCurTune = mCurMusicTune;
	UnloadAllMusic();
	RescanMusicFiles();
	if (aCurTune != MusicTune::MUSIC_TUNE_NONE)
		PlayMusic(aCurTune, -1, -1);
}

void Music::UnloadAllMusic()
{
	StopAllMusic();

	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface.get();
	if (!anSDL)
		return;

	for (auto& pair : anSDL->mMusicMap)
	{
		SDLMusicInfo& aMusicInfo = pair.second;
		if (aMusicInfo.mHMusic != nullptr)
		{
			Mix_FreeMusic(aMusicInfo.mHMusic);
			aMusicInfo.mHMusic = nullptr;
		}
	}
	anSDL->mMusicMap.clear();

	for (int i = 0; i < (int)MusicFile::NUM_MUSIC_FILES; i++)
	{
		if (gMusicFileData[i].mFileData != nullptr)
		{
			delete[] gMusicFileData[i].mFileData;
			gMusicFileData[i].mFileData = nullptr;
		}
	}

	mMusicDisabled = false;
	PvzpTrace("Unloaded all music");
}

void Music::RescanMusicFiles()
{
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MUSIC, "sounds/mainmusic.mo3");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_DRUMS, "sounds/mainmusic.mo3");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_HIHATS, "sounds/mainmusic_hihats.mo3");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN, "sounds/ZombiesOnYourLawn.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MAIN_MENU, "Music/MainMenu.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_DAY_GRASSWALK, "Music/Grasswalk.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_NIGHT_MOONGRAINS, "Music/MoonGrains.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_POOL_WATERYGRAVES, "Music/WateryGraves.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_FOG_RIGORMORMIST, "Music/RigorMormist.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_ROOF_GRAZETHEROOF, "Music/GrazeTheRoof.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CHOOSE_YOUR_SEEDS, "Music/ChooseYourSeeds.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_ZEN_GARDEN, "Music/ZenGarden.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_PUZZLE_CEREBRAWL, "Music/Cerebrawl.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_MINIGAME_LOONBOON, "Music/LoonBoon.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_CONVEYER, "Music/UltimateBattle.ogg");
	LoadMusicFromResourcePack(MusicFile::MUSIC_FILE_FINAL_BOSS_BRAINIAC_MANIAC, "Music/BrainiacManiac.ogg");
}

void Music::StopAllMusic()
{
	if (mMusicInterface != nullptr)
	{
		if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
			mMusicInterface->StopMusic(mCurMusicFileMain);
		if (mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
			mMusicInterface->StopMusic(mCurMusicFileDrums);
	}

	mCurMusicTune = MusicTune::MUSIC_TUNE_NONE;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
	mQueuedDrumTrackPackedOrder = -1;
	mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
	mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
	mPauseOffset = 0;
	mPauseOffsetDrums = 0;
	mPaused = false;
	mFadeOutCounter = 0;
}

Mix_Music* Music::GetMusicHandle(MusicFile theMusicFile)
{
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface.get();
	auto anItr = anSDL->mMusicMap.find((int)theMusicFile);
	PVZP_ASSERT(anItr != anSDL->mMusicMap.end());
	return anItr->second.mHMusic;
}

void Music::PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume)
{
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface.get();
	auto anItr = anSDL->mMusicMap.find((int)theMusicFile);
	if (anItr == anSDL->mMusicMap.end())
		return;
	SDLMusicInfo* aMusicInfo = &anItr->second;

	if (mCurMusicTune == MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN)
	{
		bool aNoLoop = theMusicFile == MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN;
		mMusicInterface->PlayMusic(theMusicFile, theOffset, aNoLoop);
	}
	else if (theMusicFile == MusicFile::MUSIC_FILE_MAIN_MUSIC || theMusicFile == MusicFile::MUSIC_FILE_DRUMS)
	{
		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		aMusicInfo->mStopOnFade = false;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap * theVolume;
		aMusicInfo->mVolumeAdd = 0.0;
		Mix_PlayMusicStream(aMusicInfo->mHMusic, -1);
		Mix_ModMusicStreamJumpToOrder(aMusicInfo->mHMusic, theOffset);
		Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
		SetupVolumeForTune(mCurMusicTune, 0, 0);
	}
	else
	{
		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		aMusicInfo->mStopOnFade = false;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap * theVolume;
		aMusicInfo->mVolumeAdd = 0.0;
		Mix_PlayMusicStream(aMusicInfo->mHMusic, -1);
		Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
	}
}

void Music::PlayMusic(MusicTune theMusicTune, int theOffset, int theDrumsOffset)
{
	if (mMusicDisabled)
		return;

	mCurMusicTune = theMusicTune;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;

	MusicFile aPerTuneFile = MusicFile::MUSIC_FILE_NONE;
	switch (theMusicTune)
	{
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:			aPerTuneFile = MusicFile::MUSIC_FILE_DAY_GRASSWALK; break;
	case MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS:		aPerTuneFile = MusicFile::MUSIC_FILE_NIGHT_MOONGRAINS; break;
	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:		aPerTuneFile = MusicFile::MUSIC_FILE_POOL_WATERYGRAVES; break;
	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:		aPerTuneFile = MusicFile::MUSIC_FILE_FOG_RIGORMORMIST; break;
	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:		aPerTuneFile = MusicFile::MUSIC_FILE_ROOF_GRAZETHEROOF; break;
	case MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS:		aPerTuneFile = MusicFile::MUSIC_FILE_CHOOSE_YOUR_SEEDS; break;
	case MusicTune::MUSIC_TUNE_ZEN_GARDEN:				aPerTuneFile = MusicFile::MUSIC_FILE_ZEN_GARDEN; break;
	case MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL:		aPerTuneFile = MusicFile::MUSIC_FILE_PUZZLE_CEREBRAWL; break;
	case MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON:		aPerTuneFile = MusicFile::MUSIC_FILE_MINIGAME_LOONBOON; break;
	case MusicTune::MUSIC_TUNE_CONVEYER:				aPerTuneFile = MusicFile::MUSIC_FILE_CONVEYER; break;
	case MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC: aPerTuneFile = MusicFile::MUSIC_FILE_FINAL_BOSS_BRAINIAC_MANIAC; break;
	case MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME: aPerTuneFile = MusicFile::MUSIC_FILE_MAIN_MENU; break;
	default: break;
	}

	if (aPerTuneFile != MusicFile::MUSIC_FILE_NONE && IsPerTuneFileLoaded(aPerTuneFile))
	{
		mCurMusicFileMain = aPerTuneFile;
		PlayFromOffset(aPerTuneFile, 0, 1.0);
		return;
	}

	switch (theMusicTune)
	{
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		if (theOffset == -1)
		{
			theOffset = 0x30;
			theDrumsOffset = 0x5C;
		}
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theDrumsOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x5E;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x7D;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xB8;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x7A;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x98;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_ZEN_GARDEN:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xDD;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xB1;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xA6;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_CONVEYER:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xD4;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x9E;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN;
		if (theOffset == -1)
			theOffset = 0;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	default:
		PVZP_ASSERT(false);
		break;
	}
}

unsigned long Music::GetMusicOrder(MusicFile theMusicFile)
{
	if (theMusicFile != MusicFile::MUSIC_FILE_MAIN_MUSIC && theMusicFile != MusicFile::MUSIC_FILE_DRUMS)
		return 0;
	PVZP_ASSERT(theMusicFile != MusicFile::MUSIC_FILE_NONE);
	return ((SDLMusicInterface*)mApp->mMusicInterface.get())->GetMusicOrder((int)theMusicFile);
}

void Music::StartBurst()
{
	if (mMusicBurstState == MusicBurstState::MUSIC_BURST_OFF)
	{
		mMusicBurstState = MusicBurstState::MUSIC_BURST_STARTING;
		mBurstStateCounter = 400;
	}
}

void Music::FadeOut(int theFadeOutDuration)
{
	if (mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
	{
		mFadeOutCounter = theFadeOutDuration;
		mFadeOutDuration = theFadeOutDuration;
	}
}

void Music::UpdateMusicBurst()
{
	if (mApp->mBoard == nullptr)
		return;
	if (mApp->mGameMode == GameMode::GAMEMODE_INTRO)
		return;
	if (mCurMusicFileMain != MusicFile::MUSIC_FILE_MAIN_MUSIC)
		return;

	int aBurstScheme;
	if (mCurMusicTune == MusicTune::MUSIC_TUNE_DAY_GRASSWALK || mCurMusicTune == MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES ||
		mCurMusicTune == MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST || mCurMusicTune == MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF)
		aBurstScheme = 1;
	else if (mCurMusicTune == MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS)
		aBurstScheme = 2;
	else
		return;

	int aPackedOrderMain = GetMusicOrder(mCurMusicFileMain);
	if (mBurstStateCounter > 0)
		mBurstStateCounter--;
	if (mDrumsStateCounter > 0)
		mDrumsStateCounter--;

	float aFadeTrackVolume = 0.0f;
	float aDrumsVolume = 0.0f;
	float aMainTrackVolume = 1.0f;
	switch (mMusicBurstState)
	{
		case MusicBurstState::MUSIC_BURST_OFF:
			if (mApp->mBoard->CountZombiesOnScreen() >= 10 || mBurstOverride == 1)
				StartBurst();
			break;
		case MusicBurstState::MUSIC_BURST_STARTING:
			if (aBurstScheme == 1)
			{
				aFadeTrackVolume = PvzpAnimateCurveFloat(400, 0, mBurstStateCounter, 0.0f, 1.0f, PvzpCurves::CURVE_LINEAR);
				if (mBurstStateCounter == 100)
				{
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
				}
				else if (mBurstStateCounter == 0)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_ON;
					mBurstStateCounter = 800;
				}
			}
			else if (aBurstScheme == 2)
			{
				if (mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_OFF)
				{
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
					mBurstStateCounter = 400;
				}
				else if (mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_ON_QUEUED)
					mBurstStateCounter = 400;
				else
				{
					aMainTrackVolume = PvzpAnimateCurveFloat(400, 0, mBurstStateCounter, 1.0f, 0.0f, PvzpCurves::CURVE_LINEAR);
					if (mBurstStateCounter == 0)
					{
						mMusicBurstState = MusicBurstState::MUSIC_BURST_ON;
						mBurstStateCounter = 800;
					}
				}
			}
			break;
		case MusicBurstState::MUSIC_BURST_ON:
			aFadeTrackVolume = 1.0f;
			if (aBurstScheme == 2)
				aMainTrackVolume = 0.0f;
			if (mBurstStateCounter == 0 && ((mApp->mBoard->CountZombiesOnScreen() < 4 && mBurstOverride == -1) || mBurstOverride == 2))
			{
				if (aBurstScheme == 1)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_FINISHING;
					mBurstStateCounter = 800;
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
				}
				else if (aBurstScheme == 2)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_FINISHING;
					mBurstStateCounter = 1100;
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_FADING;
					mDrumsStateCounter = 800;
				}
			}
			break;
		case MusicBurstState::MUSIC_BURST_FINISHING:
			if (aBurstScheme == 1)
				aFadeTrackVolume = PvzpAnimateCurveFloat(800, 0, mBurstStateCounter, 1.0f, 0.0f, PvzpCurves::CURVE_LINEAR);
			else
				aMainTrackVolume = PvzpAnimateCurveFloat(400, 0, mBurstStateCounter, 0.0f, 1.0f, PvzpCurves::CURVE_LINEAR);
			if (mBurstStateCounter == 0 && mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_OFF)
				mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
			break;
	}

	int aDrumsJumpOrder = -1;
	int aOrderMain = 0, aOrderDrum = 0;
	if (aBurstScheme == 1)
	{
		aOrderMain = aPackedOrderMain & 0xFFFF;
		aOrderDrum = mQueuedDrumTrackPackedOrder & 0xFFFF;
	}
	else if (aBurstScheme == 2)
	{
		aOrderMain = aPackedOrderMain & 0xFFFF;
		aOrderDrum = mQueuedDrumTrackPackedOrder & 0xFFFF;
	}

	switch (mMusicDrumsState)
	{
		case MusicDrumsState::MUSIC_DRUMS_ON_QUEUED:
			if (aOrderMain != aOrderDrum)
			{
				aDrumsVolume = 1.0f;
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON;
				if (aBurstScheme == 2)
					aDrumsJumpOrder = (aOrderMain % 2 == 0) ? 76 : 77;
			}
			break;
		case MusicDrumsState::MUSIC_DRUMS_ON:
			aDrumsVolume = 1.0f;
			break;
		case MusicDrumsState::MUSIC_DRUMS_OFF_QUEUED:
			aDrumsVolume = 1.0f;
			if (aOrderMain != aOrderDrum && aBurstScheme == 1)
			{
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_FADING;
				mDrumsStateCounter = 50;
			}
			break;
		case MusicDrumsState::MUSIC_DRUMS_FADING:
			if (aBurstScheme == 2)
				aDrumsVolume = PvzpAnimateCurveFloat(800, 0, mDrumsStateCounter, 1.0f, 0.0f, PvzpCurves::CURVE_LINEAR);
			else
				aDrumsVolume = PvzpAnimateCurveFloat(50, 0, mDrumsStateCounter, 1.0f, 0.0f, PvzpCurves::CURVE_LINEAR);
			if (mDrumsStateCounter == 0)
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
			break;
		case MusicDrumsState::MUSIC_DRUMS_OFF:
			break;
	}

	if (aBurstScheme == 1)
	{
		SetupVolumeForTune(mCurMusicTune, aDrumsVolume, aFadeTrackVolume);
	}
	else if (aBurstScheme == 2)
	{
		mMusicInterface->SetSongVolume(mCurMusicFileMain, aMainTrackVolume);
		mMusicInterface->SetSongVolume(mCurMusicFileDrums, aDrumsVolume);
		if (aDrumsJumpOrder != -1)
			Mix_ModMusicStreamJumpToOrder(GetMusicHandle(mCurMusicFileDrums), aDrumsJumpOrder);
	}
}

void Music::MusicUpdate()
{
	if (mFadeOutCounter > 0)
	{
		mFadeOutCounter--;
		if (mFadeOutCounter == 0)
			StopAllMusic();
		else
		{
			float aFadeLevel = PvzpAnimateCurveFloat(mFadeOutDuration, 0, mFadeOutCounter, 1.0f, 0.0f, PvzpCurves::CURVE_LINEAR);
			mMusicInterface->SetSongVolume(mCurMusicFileMain, aFadeLevel);
		}
	}

	if (mApp->mBoard == nullptr || !mApp->mBoard->mPaused)
	{
		UpdateMusicBurst();
	}
}

void Music::MakeSureMusicIsPlaying(MusicTune theMusicTune)
{
	if (mCurMusicTune != theMusicTune)
	{
		StopAllMusic();
		PlayMusic(theMusicTune, -1, -1);
	}
}

void Music::StartGameMusic()
{
	PVZP_ASSERT(mApp->mBoard);

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_ZEN_GARDEN);
	else if (mApp->IsFinalBossLevel())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC);
	else if (mApp->IsWallnutBowlingLevel() || mApp->IsWhackAZombieLevel() || mApp->IsLittleTroubleLevel() || mApp->IsBungeeBlitzLevel() ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SPEED)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON);
	else if ((mApp->IsAdventureMode() && (mApp->mPlayerInfo->GetLevel() == 10 || mApp->mPlayerInfo->GetLevel() == 20 || mApp->mPlayerInfo->GetLevel() == 30)) ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_CONVEYER);
	else if (mApp->IsStormyNightLevel())
		StopAllMusic();
	else if (mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL);
	else if (mApp->mBoard->StageIsNight())
	{
		if (mApp->mBoard->StageHasPool())
			MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST);
		else
			MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS);
	}
	else if (mApp->mBoard->StageHas6Rows())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES);
	else if (mApp->mBoard->StageHasRoof())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF);
	else
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_DAY_GRASSWALK);
}

void Music::GameMusicPause(bool thePause)
{
	if (thePause)
	{
		if (!mPaused && mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
		{
			if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE &&
				mCurMusicTune != MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN)
			{
				mPauseOffset = GetMusicOrder(mCurMusicFileMain);
			}
			if (mCurMusicTune == MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS &&
				mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
			{
				mPauseOffsetDrums = GetMusicOrder(mCurMusicFileDrums);
			}

			if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
				mMusicInterface->PauseMusic(mCurMusicFileMain);
			if (mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
				mMusicInterface->PauseMusic(mCurMusicFileDrums);

			mPaused = true;
		}
	}
	else if (mPaused)
	{
		if (mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
		{
			Mix_Music* aHandle = (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE) ?
								  GetMusicHandle(mCurMusicFileMain) : nullptr;
			if (mCurMusicTune == MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN ||
				(aHandle && Mix_PlayingMusicStream(aHandle)))
			{
				if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
					mMusicInterface->ResumeMusic(mCurMusicFileMain);
				if (mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
					mMusicInterface->ResumeMusic(mCurMusicFileDrums);
			}
			else
			{
				PlayMusic(mCurMusicTune, mPauseOffset, mPauseOffsetDrums);
			}
		}
		mPaused = false;
	}
}
