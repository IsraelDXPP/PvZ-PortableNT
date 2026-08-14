# PvZ-Portable on UWP / Xbox One

This target runs the game as a **Universal Windows Platform (UWP)** app packaged
as an AppX/MSIX, which is what an **Xbox One** (retail console in Developer Mode)
and Windows 10/11 can install. It uses SDL2's **WinRT** backend with OpenGL ES 2.0
rendering through **ANGLE** (D3D11), because UWP has no desktop OpenGL.

> **Status**
>
> * Platform layer: `src/SexyAppFramework/platform/uwp/Window.cpp` (done)
> * Gamepad-driven virtual mouse: `src/SexyAppFramework/platform/default/Input.cpp` under `PVZ_UWP` (done)
> * Win32-specific code isolated: `src/main.cpp` (done)
> * AppX manifest + assets: `uwp/` (done)
> * CMake target: `-DCMAKE_SYSTEM_NAME=WindowsStore` / `PVZ_UWP` (done)
> * **Not yet verified end-to-end:** the WinRT toolchain (SDL2 + ANGLE for `x64-uwp`)
>   and packaging need at least one successful build. The `ci-uwp.yml` workflow is
>   the place to iterate on that.

## Prerequisites

* **Visual Studio 2022** with the **"Universal Windows Platform development"**
  workload installed (check "C++ Universal Windows Platform tools" and a
  "Windows 10 SDK (10.0.xxxxx.0)"). This provides `/ZW`, `makeappx`, `makepri`
  and the AppX targets.
* **vcpkg** (`x64-uwp` triplet) or prebuilt UWP libraries.
* An **Xbox One** in Developer Mode:
  1. Install the free **"Xbox Developer Mode Activation"** app on the console
     (purchase required, ~$20; you need a real Microsoft account).
  2. Run it, agree, and restart. The console now has a Developer Mode dashboard.
  3. Under **Home** you'll find the console's **IP address** and the link to its
     **Device Portal** (`http://<console-ip>:11443`).

## 1. Dependencies (vcpkg, `x64-uwp` triplet)

```powershell
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Build all the libraries the game needs for UWP (x64 is correct for Xbox One,
# which is a 64-bit AMD console)
.\vcpkg\vcpkg install `
  sdl2 libpng libjpeg-turbo zlib libogg libvorbis libopenmpt mpg123 `
  --triplet x64-uwp
```

Notes:

* The `sdl2` port for UWP builds SDL2's own `VisualC-WinRT` project (WinRT video
  driver + XInput game controllers + WASAPI audio), which is what we want.
* **ANGLE** is *not* part of SDL2. SDL2's WinRT backend renders GLES2 through
  EGL, so the package must ship `libEGL.dll`, `libGLESv2.dll` and
  `d3dcompiler_47.dll`. Try `vcpkg install angle --triplet x64-uwp`; if the port
  does not support UWP, grab a WinRT build of ANGLE and put the DLLs in the
  package (see step 4). Without working EGL the app builds but fails to create
  a context — the game logs `Failed to create OpenGL ES context (ANGLE).`

## 2. Configure & build the game

```powershell
cmake -G "Visual Studio 17 2022" -A x64 `
  -B build-uwp `
  -DCMAKE_SYSTEM_NAME=WindowsStore `
  -DCMAKE_SYSTEM_VERSION=10.0 `
  -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-uwp `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-uwp --config Release
```

This produces:

* `pvz-portable.exe` (the UWP exe, app-container linked)
* `uwp/Package.appxmanifest` (generated in the build tree)

The `PVZ_UWP` macro is defined automatically (also by `__WINRT__`, which SDL2
sets), so no extra flags are needed. You can force the target on a plain MSVC
build with `-DPVZ_UWP=ON`.

## 3. Assemble the package

Build an `AppPackages`-style layout:

```powershell
$pkg = "out\appx"
Remove-Item -Recurse -Force $pkg -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $pkg | Out-Null

# Game executable
Copy-Item build-uwp\Release\pvz-portable.exe $pkg

# SDL2 (if dynamic) + ANGLE dlls
Copy-Item $vcpkg\installed\x64-uwp\bin\SDL2.dll $pkg -ErrorAction SilentlyContinue
Copy-Item libEGL.dll, libGLESv2.dll, d3dcompiler_47.dll $pkg -ErrorAction SilentlyContinue

# Game resources: main.pak and properties/ must be bundled (install dir is read-only)
New-Item -ItemType Directory -Force $pkg\resources | Out-Null
Copy-Item main.pak $pkg\resources
Copy-Item -Recurse properties $pkg\resources

# AppX manifest + assets
Copy-Item build-uwp\uwp\Package.appxmanifest $pkg\AppxManifest.xml
Copy-Item -Recurse uwp\Assets $pkg\Assets
```

### Make the package

```powershell
$sdks = "C:\Program Files (x86)\Windows Kits\10\bin"
$tools = (Get-ChildItem "$sdks\*\x64\makeappx.exe" | Select-Object -Last 1).DirectoryName
$env:Path += ";$tools"

# Package resources (pri) — needed or registration can fail
makepri new /pr $pkg /cf "$sdks\...\AppxLayout\mappings\AppxMapping.xml" /of $pkg\resources.pri /mn $pkg\AppxManifest.xml

makeappx pack /d $pkg /p out\PvZPortable.appx
```

> A simpler path is to open the project in Visual Studio with the UWP workload
> and use the built-in **Deploy to Xbox** target — VS handles makepri/makeappx,
> signing and deployment. The manual route above is for CI/headless builds.

### Sign (dev mode)

Developer Mode does not trust the public Store signing chain, so sign with a
certificate whose CN matches the `Publisher` in the manifest:

```powershell
$cert = New-SelfSignedCertificate -Type CodeSigningCert `
  -Subject "CN=PvZPortable.Community" -CertStoreLocation Cert:\CurrentUser\My `
  -NotAfter (Get-Date).AddYears(10)

$pwd = ConvertTo-SecureString -String "password" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath out\pvzportable.pfx -Password $pwd

signtool sign /fd SHA256 /f out\pvzportable.pfx /p password out\PvZPortable.appx
```

> If you change the manifest `Publisher`, regenerate the cert with the same CN.

## 4. Deploy to the Xbox One

Open the Device Portal on your dev machine: `http://<console-ip>:11443` (accept
the self-signed cert), then:

```powershell
# Add your dev cert as a trusted root on the console
# Device Portal -> Explorer tab or:
curl.exe -k -u DevToolsUser -X POST `
  "https://<console-ip>:11443/api/app/packagemanager/upload" `
  -F "file=@out\PvZPortable.appx"

# Or deploy from Visual Studio: set a signing cert, then Build > Deploy with
# the "Xbox One (Device Portal)" remote machine selected.
```

From VS: **Project > Properties > Debugging**, set *Remote machine* to the
console IP and *Authentication mode* to *Universal (Unencrypted protocol)*, then
**Build > Deploy Solution**.

## Gamepad controls (virtual mouse)

UWP has no mouse on console, so `default/Input.cpp` synthesizes one from the
controller when `PVZ_UWP`/`__WINRT__` is defined:

| Input                | Action                                  |
| -------------------- | --------------------------------------- |
| Left stick           | Move the cursor                         |
| **A** / **X** / LB   | Left click (select / plant)             |
| **B** / **Y** / RB   | Right click (shovel)                    |
| Left / right trigger | Mouse wheel (scroll lists, almanac)     |
| View / Menu buttons  | Escape (pause)                          |
| D-pad                | Arrow keys (menu navigation)            |

Saves go to the app's `LocalState` folder (`SDL_GetPrefPath` on WinRT); game
resources are read from the package install directory.

## Troubleshooting

* **APPX0702 / missing .winmd** — a `/ZW` file must exist. `uwp/PvzpUwpMetadata.cpp`
  is compiled with `/ZW` for this; if you drop the CMake target, keep one
  `/ZW` translation unit.
* **LNK2038 `vccorlib` mismatch** — for Release add
  `/nodefaultlib:vccorlib /nodefaultlib:msvcrt vccorlib.lib msvcrt.lib` to the
  linker; for Debug the `d` variants.
* **DEP0700 registration failed `0x80073CF6`** — the app package is missing an
  `EntryPoint`/`StartPage`, or the manifest is malformed after hand-editing.
* **Controller not detected** — UWP only exposes *Xbox-compatible* controllers
  (Microsoft pads + licensed USB adapters).
* **Black screen after launch** — EGL/ANGLE isn't in the package; check the game
  log for `Failed to create OpenGL ES context (ANGLE).`

## References

* SDL2 WinRT docs: <https://wiki.libsdl.org/SDL2/README-winrt>
* Xbox UWP + SDL2 starter (has a prebuilt WinRT `SDL2.dll`):
  <https://github.com/Justin-Credible/xbox-uwp-sdl2-starter>
* UWP on Xbox One: <https://docs.microsoft.com/en-us/windows/uwp/xbox-apps/>
* vcpkg UWP triplets: <https://learn.microsoft.com/en-us/vcpkg/users/platforms/uwp>
