cmake_minimum_required(VERSION 3.12...4.0)

project(pvz-portable LANGUAGES C CXX)

include(CMake/ProjectVersion.cmake)

# Universal Windows Platform (Windows 10 / Xbox One via SDL2's winrt backend).
# Set PVZ_UWP=ON, or configure with -DCMAKE_SYSTEM_NAME=WindowsStore
# (vcpkg x64-uwp triplet does this automatically).
option(PVZ_UWP "Build for the Universal Windows Platform (Xbox One / UWP)" OFF)
if(PVZ_UWP OR CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	set(PVZ_IS_UWP TRUE)
else()
	set(PVZ_IS_UWP FALSE)
endif()

option(BUILD_STATIC "Statically link dependencies for a standalone exe (Windows MinGW, Linux)" OFF)
if(BUILD_STATIC)
	if(WIN32 AND NOT MSVC)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
		set(BUILD_SHARED_LIBS OFF)
		add_link_options(-static)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
		set(BUILD_SHARED_LIBS OFF)
	elseif(MSVC)
		message(WARNING "BUILD_STATIC is not supported for MSVC. Use a vcpkg -static triplet instead.")
		set(BUILD_STATIC OFF)
	else()
		message(WARNING "BUILD_STATIC is only supported on Windows with MinGW-based toolchains and on Linux.")
		set(BUILD_STATIC OFF)
	endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (CMAKE_GENERATOR MATCHES "Ninja")
	set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

if(MSVC)
	if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "-static")
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	elseif(DEFINED VCPKG_TARGET_TRIPLET)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()
endif()

file(GLOB SOURCES CONFIGURE_DEPENDS
	src/*.cpp
	src/SexyAppFramework/*.cpp
	src/SexyAppFramework/graphics/*.cpp
	src/SexyAppFramework/imagelib/*.cpp
	src/SexyAppFramework/misc/*.cpp
	src/SexyAppFramework/paklib/*.cpp
	src/SexyAppFramework/sound/*.cpp
	src/SexyAppFramework/widget/*.cpp
	src/SexyAppFramework/fcaseopen/*.c
	src/PvzpLib/*.cpp
	src/Lawn/*.cpp
	src/Lawn/Widget/*.cpp
	src/Lawn/System/*.cpp
)

set(LOW_MEMORY OFF CACHE BOOL "Enable low memory mode")
if(NINTENDO_SWITCH)
	find_library(OGG_LIBRARY NAMES ogg libogg REQUIRED)
	find_library(VORBIS_LIBRARY NAMES vorbis libvorbis REQUIRED)
	find_library(VORBISFILE_LIBRARY NAMES vorbisfile libvorbisfile REQUIRED)
	find_library(OPENMPT_LIBRARY NAMES openmpt libopenmpt REQUIRED)
	find_library(MPG123_LIBRARY NAMES mpg123 libmpg123 REQUIRED)

	set(VORBIS_LIB ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})
	set(OGG_LIB ${OGG_LIBRARY})
	set(OPENMPT_LIB ${OPENMPT_LIBRARY})
	set(MPG123_LIB ${MPG123_LIBRARY})

	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/switch
	)
elseif(EMSCRIPTEN)
	# libopenmpt has no Emscripten port; must be pre-built with emmake.
	find_library(OPENMPT_LIBRARY NAMES openmpt libopenmpt REQUIRED)
	set(OPENMPT_LIB ${OPENMPT_LIBRARY})

	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/emscripten/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/default/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/emscripten
	)
elseif(PVZ_IS_UWP)
	# UWP (Xbox One / Windows 10): SDL2 winrt backend, GLES2 via ANGLE.
	# Input is the shared SDL event loop + virtual gamepad cursor.
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/uwp/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/default/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/uwp
	)
else()
	if(VCPKG_TOOLCHAIN)
		find_package(Vorbis CONFIG REQUIRED)
		find_package(Ogg CONFIG REQUIRED)
		find_package(libopenmpt CONFIG REQUIRED)
		find_package(mpg123 CONFIG REQUIRED)
		set(VORBIS_LIB Vorbis::vorbis Vorbis::vorbisfile)
		set(OGG_LIB Ogg::ogg)
		set(OPENMPT_LIB libopenmpt::libopenmpt)
		set(MPG123_LIB MPG123::libmpg123)
	else()
		find_library(OGG_LIBRARY NAMES ogg libogg REQUIRED)
		find_library(VORBIS_LIBRARY NAMES vorbis libvorbis REQUIRED)
		find_library(VORBISFILE_LIBRARY NAMES vorbisfile libvorbisfile REQUIRED)
		find_library(OPENMPT_LIBRARY NAMES openmpt libopenmpt REQUIRED)
		find_library(MPG123_LIBRARY NAMES mpg123 libmpg123 REQUIRED)

		set(VORBIS_LIB ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})
		set(OGG_LIB ${OGG_LIBRARY})
		set(OPENMPT_LIB ${OPENMPT_LIBRARY})
		set(MPG123_LIB ${MPG123_LIBRARY})
	endif()
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/default/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/default/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/default
	)
endif()

if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
	set(WIN_CONSOLE_DEFAULT ON)
	set(PVZ_DEBUG_DEFAULT ON)
else()
	set(WIN_CONSOLE_DEFAULT OFF)
	set(PVZ_DEBUG_DEFAULT OFF)
endif()

option(PVZ_DEBUG "Enable PVZ_DEBUG macro to enable cheats, hidden features, etc." ${PVZ_DEBUG_DEFAULT})
option(CONSOLE "Show console on Windows" ${WIN_CONSOLE_DEFAULT})
option(DO_FIX_BUGS "Define DO_FIX_BUGS macro (Community fixes for original game bugs of 1.2.0.1073 GOTY Edition)" OFF)

if(EMSCRIPTEN)
	list(PREPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_SDL=2 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -sUSE_OGG=1 -sUSE_VORBIS=1 -sUSE_MPG123=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -sUSE_OGG=1 -sUSE_VORBIS=1 -sUSE_MPG123=1 -fexceptions")
else()
	find_package(Threads REQUIRED)
	find_package(ZLIB REQUIRED)
	find_package(JPEG REQUIRED)
	find_package(PNG REQUIRED)
	find_package(SDL2 REQUIRED)
endif()

function(_pvz_patch_sdl2_iface_libs target)
	get_target_property(_libs ${target} INTERFACE_LINK_LIBRARIES)
	if(NOT _libs)
		return()
	endif()
	set(_patched)
	foreach(_lib IN LISTS _libs)
		if(_lib STREQUAL "SDL2::SDL2")
			list(APPEND _patched SDL2::SDL2-static)
		else()
			list(APPEND _patched "${_lib}")
		endif()
	endforeach()
	set_target_properties(${target} PROPERTIES INTERFACE_LINK_LIBRARIES "${_patched}")
endfunction()

if(BUILD_STATIC AND TARGET SDL2::SDL2-static)
	if(TARGET SDL2::SDL2main)
		_pvz_patch_sdl2_iface_libs(SDL2::SDL2main)
	endif()
	set(SDL2_LINK_TARGET SDL2::SDL2-static)
else()
	set(SDL2_LINK_TARGET SDL2::SDL2)
endif()

set(SDL_MIXER_X_STATIC ON CACHE BOOL "Build static library of SDL Mixer X")
set(SDL_MIXER_X_SHARED OFF CACHE BOOL "Build shared library of SDL Mixer X")
set(USE_MIDI OFF CACHE BOOL "SDL Mixer X midi")
set(USE_MODPLUG OFF CACHE BOOL "SDL Mixer X modplug")
set(USE_XMP OFF CACHE BOOL "SDL Mixer X xmp")
set(USE_OPUS OFF CACHE BOOL "SDL Mixer X opus")
set(USE_DRFLAC OFF CACHE BOOL "SDL Mixer X flac")
set(USE_PXTONE OFF CACHE BOOL "SDL Mixer X pxtone")
add_subdirectory(src/SexyAppFramework/sound/SDL-Mixer-X)

# Patch SDL-Mixer-X's transitive SDL2 dep without touching vendored CMake files
if(BUILD_STATIC AND TARGET SDL2::SDL2-static)
	_pvz_patch_sdl2_iface_libs(SDL2_mixer_ext_Static)
endif()

# Android requires a shared library loaded by SDLActivity; other platforms use an executable.
if(ANDROID)
	add_library(pvz-portable SHARED ${SOURCES})
elseif(IOS)
	add_executable(pvz-portable MACOSX_BUNDLE ${SOURCES})
else()
	add_executable(pvz-portable ${SOURCES})
endif()

target_include_directories(pvz-portable PRIVATE
	${PLAT_INCLUDES}
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/SexyAppFramework
	${PROJECT_SOURCE_DIR}/src/SexyAppFramework/sound/SDL-Mixer-X/include
	${SDL2_INCLUDE_DIRS}
)
target_compile_definitions(pvz-portable PRIVATE
	$<$<BOOL:${PVZ_DEBUG}>:PVZ_DEBUG>
	$<$<BOOL:${DO_FIX_BUGS}>:DO_FIX_BUGS>
)
target_compile_features(pvz-portable PRIVATE cxx_std_20)
set_target_properties(pvz-portable PROPERTIES CXX_EXTENSIONS OFF)

if (LOW_MEMORY)
	target_compile_definitions(pvz-portable PRIVATE LOW_MEMORY)
endif()

if(EMSCRIPTEN)
	set_property(TARGET pvz-portable APPEND PROPERTY LINK_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/wasm/shell.html"
	)
	target_link_libraries(pvz-portable PRIVATE
		SDL2_mixer_ext_Static
		${OPENMPT_LIB}
	)
	target_link_options(pvz-portable PRIVATE
		"SHELL:-sUSE_SDL=2"
		"SHELL:-sUSE_ZLIB=1"
		"SHELL:-sUSE_LIBPNG=1"
		"SHELL:-sUSE_LIBJPEG=1"
		"SHELL:-sUSE_OGG=1"
		"SHELL:-sUSE_VORBIS=1"
		"SHELL:-sUSE_MPG123=1"
		"SHELL:-fexceptions"
		"SHELL:-sASYNCIFY"
		"SHELL:-sALLOW_MEMORY_GROWTH=1"
		"SHELL:-sMAXIMUM_MEMORY=1073741824"
		"SHELL:-sGROWABLE_ARRAYBUFFERS=0"
		"SHELL:-sFORCE_FILESYSTEM=1"
		"SHELL:-sEXPORTED_RUNTIME_METHODS=[\"FS\",\"callMain\"]"
		"SHELL:-sINVOKE_RUN=0"
		"SHELL:-lidbfs.js"
		"SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/wasm/shell.html"
	)
	set_target_properties(pvz-portable PROPERTIES SUFFIX ".html")
else()
	target_link_libraries(pvz-portable PRIVATE
		SDL2_mixer_ext_Static 
		${OPENMPT_LIB} 
		${MPG123_LIB} 
		${VORBIS_LIB} 
		${OGG_LIB} 
		PNG::PNG
		JPEG::JPEG
		ZLIB::ZLIB
		$<$<NOT:$<BOOL:${PVZ_IS_UWP}>>:SDL2::SDL2main>
		${SDL2_LINK_TARGET}
		Threads::Threads
	)
endif()

if (WIN32 AND NOT PVZ_IS_UWP)
	if(MSVC)
		target_compile_options(pvz-portable PRIVATE /utf-8)
	endif()

	target_sources(pvz-portable PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/LawnProject.rc)

	set_source_files_properties(
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/LawnProject.rc
		PROPERTIES
			COMPILE_DEFINITIONS "APP_ICON_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/icon.ico\""
			OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
	)

	target_link_libraries(pvz-portable PRIVATE ws2_32 user32 gdi32 winmm imm32 shlwapi)
	target_compile_definitions(pvz-portable PRIVATE WINDOWS)

	if(NOT CONSOLE)
		if(MSVC)
			set_target_properties(pvz-portable PROPERTIES WIN32_EXECUTABLE TRUE)
		else()
			target_link_options(pvz-portable PRIVATE -mwindows)
		endif()
	endif()
elseif (NINTENDO_SWITCH)
	target_link_libraries(pvz-portable PRIVATE GLESv2 EGL glapi drm_nouveau)

	nx_generate_nacp(pvz-portable.nacp
		NAME "PvZ Portable"
		AUTHOR "wszqkzqk, etc., a community-driven port"
		VERSION "${PVZP_VERSION_PLAIN}"
	)

	nx_create_nro(pvz-portable
		NACP pvz-portable.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon-switch.jpg
	)
elseif (ANDROID)
	target_link_libraries(pvz-portable PRIVATE GLESv2 EGL android log)
	set_target_properties(pvz-portable PROPERTIES OUTPUT_NAME "main")
elseif (IOS)
	target_link_libraries(pvz-portable PRIVATE
		"-framework Foundation"
		"-framework UIKit"
		"-framework OpenGLES"
		"-framework QuartzCore"
		"-framework CoreGraphics"
		"-framework CoreMotion"
		"-framework GameController"
		"-framework AVFoundation"
		"-framework AudioToolbox"
		"-framework Metal"
	)

	if(IOS_IPADMINI1)
		target_compile_definitions(pvz-portable PRIVATE _LIBCPP_DISABLE_AVAILABILITY)
		target_link_options(pvz-portable PRIVATE "-F${CMAKE_CURRENT_SOURCE_DIR}/dummy_frameworks" "-lobjc" "-Wl,-undefined,dynamic_lookup")
		target_sources(pvz-portable PRIVATE
			"${CMAKE_CURRENT_SOURCE_DIR}/src/ios_polyfills.c"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/ios_cxx_polyfills.cpp"
		)
	endif()

	if(NOT DEFINED IOS_DEPLOYMENT_TARGET)
		set(IOS_DEPLOYMENT_TARGET "15.0" CACHE STRING "iOS Deployment Target")
	endif()

	configure_file(ios/Info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)
	set_target_properties(pvz-portable PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.wszqkzqk.pvz-portable"
		MACOSX_BUNDLE_BUNDLE_NAME "PvZ Portable"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
		XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}"
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.github.wszqkzqk.pvz-portable"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
	)

	# Add launch storyboard to the bundle
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard")
		target_sources(pvz-portable PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard")
		set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard"
			PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	endif()

	# Add asset catalog (app icon)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets")
		target_sources(pvz-portable PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets")
		set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets"
			PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	endif()
elseif(PVZ_IS_UWP)
	# --- UWP (Xbox One / Windows 10) target setup ---
	target_compile_definitions(pvz-portable PRIVATE
		PVZ_UWP
		WINAPI_FAMILY=WINAPI_FAMILY_APP
		_WIN32_WINNT=0x0A00
		UNICODE
		_UNICODE
	)
	target_compile_options(pvz-portable PRIVATE /utf-8)
	set_target_properties(pvz-portable PROPERTIES WIN32_EXECUTABLE TRUE)

	# UWP apps link against the app-container system library only
	target_link_libraries(pvz-portable PRIVATE WindowsApp.lib)

	# App-container linker mode: selects the UWP CRT init instead of the
	# desktop one (fixes LNK2038 vccorlib/msvcrt mismatch + LNK2005).
	target_link_options(pvz-portable PRIVATE /APPCONTAINER)

	# Metadata anchor so the linker emits the app's .winmd (required by
	# C++/CX-built packages). Compiled with /ZW (Windows Runtime extensions).
	target_sources(pvz-portable PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/uwp/PvzpUwpMetadata.cpp
	)
	set_source_files_properties(
		${CMAKE_CURRENT_SOURCE_DIR}/uwp/PvzpUwpMetadata.cpp
		# /std:c++17 must come AFTER cxx_std_20 (MSVC uses the last /std flag;
		# /ZW is incompatible with /std:c++20).
		PROPERTIES COMPILE_OPTIONS "/std:c++17" "/ZW"
	)

	# UWP entry point (SDL provides no SDL2main.lib for Windows Store) plus
	# the WinRT cursor resources. Vendored from SDL's src/main/winrt (zlib).
	target_sources(pvz-portable PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/uwp/SDL_winrt_main_NonXAML.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/uwp/SDL2-WinRTResources.rc
	)
	set_source_files_properties(
		${CMAKE_CURRENT_SOURCE_DIR}/src/SexyAppFramework/platform/uwp/SDL_winrt_main_NonXAML.cpp
		PROPERTIES COMPILE_OPTIONS "/std:c++17" "/ZW"
	)

	# Generate the AppX manifest (version is padded to the 4-part form)
	if(PVZP_VERSION_PLAIN)
		string(REPLACE "." ";" _mver_parts "${PVZP_VERSION_PLAIN}")
		list(LENGTH _mver_parts _mver_len)
		set(_mver "")
		foreach(_i RANGE 0 3)
			if(_i LESS _mver_len)
				list(GET _mver_parts ${_i} _mcomp)
			else()
				set(_mcomp "0")
			endif()
			string(APPEND _mver "${_mcomp}")
			if(_i LESS 3)
				string(APPEND _mver ".")
			endif()
		endforeach()
		set(PVZP_MANIFEST_VERSION "${_mver}")
	endif()
	if(NOT PVZP_MANIFEST_VERSION)
		set(PVZP_MANIFEST_VERSION "0.1.0.0")
	endif()
	if(NOT PVZP_UWP_PUBLISHER)
		set(PVZP_UWP_PUBLISHER "CN=PvZPortable.Community")
	endif()
	set(PVZP_UWP_EXE "pvz-portable.exe")

	configure_file(uwp/Package.appxmanifest.in
		"${CMAKE_CURRENT_BINARY_DIR}/uwp/Package.appxmanifest" @ONLY)
endif()
