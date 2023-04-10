# BL3/WL Dehotfixer

Tool for BL3/WL speedruns which allows replaying old hotfixes, and maniupating the timed events.

# Building

1. Clone the repo (including submodules).
   ```
   git clone --recursive https://github.com/apple1417/dehotfixer.git
   ```

2. (OPTIONAL) Copy `postbuild.template`, and edit it to copy files to your game install directories.

3. Choose a preset, and run CMake. Most IDEs will be able to do this for you,
   ```
   cmake . --preset msvc-debug
   cmake --build out/build/msvc-debug
   ```

4. (OPTIONAL) If you're debugging a game on Steam, add a `steam_appid.txt` in the same folder as the
   executable, containing the game's Steam App Id.

   Normally, games compiled with Steamworks will call
   [`SteamAPI_RestartAppIfNecessary`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_RestartAppIfNecessary),
   which will drop your debugger session when launching the exe directly - adding this file prevents
   that. Not only does this let you debug from entry, it also unlocks some really useful debugger
   features which you can't access from just an attach (i.e. Visual Studio's Edit and Continue).
