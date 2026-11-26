# Standalone build (test the checkpoint run outside the editor)

Written for UE 5.6+ on Windows, following the `unreal-packaging` skill. Not yet run: send back the
result or the first error.

## Before you package

1. The editor setup is done ([editor-setup.md](editor-setup.md) steps 1 to 5) and the run plays in
   the editor.
2. *Project Settings* → *Maps & Modes*: **Game Default Map** is `L_SolarisTestBlock` and the
   default game mode is `BP_MRGameMode`. `Config/DefaultEngine.ini` already sets the map; check it
   is still there after saving settings in the editor.
3. *Project Settings* → *Packaging*: add `/Game/MidnightReign/Maps/L_SolarisTestBlock` to **List
   of maps to include in a packaged build**, so it is cooked even if nothing references it.

## Package from the editor

*Platforms* → *Windows* → *Binary Configuration* → **Development** → *Package Project* → pick an
empty folder such as `C:\Builds\MidnightReign`. Development keeps the console and logs, which
you need for this first test; use Shipping only for a release.

## Or from the command line

In a terminal, from the engine's `Engine\Build\BatchFiles` folder (use your own paths):

```bat
RunUAT.bat BuildCookRun -project="C:\path\to\midnight-regin\unreal\MidnightReign\MidnightReign.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Game/MidnightReign/Maps/L_SolarisTestBlock -stage -pak -archive -archivedirectory="C:\Builds\MidnightReign"
```

## Test the build

Run `C:\Builds\MidnightReign\Windows\MidnightReign.exe` and repeat the play check from
editor-setup step 5 (controls, gates in order, RUN COMPLETE and TIME UP, the three effects, lamps
and wet road, frame rate). Open the console with the backtick key and type `stat fps`.

## What to send back

- Whether packaging finished; if not, the first `Error:` line from the Output Log or the terminal.
- Each play-check line: works / doesn't, in a few words.
- Average fps and your GPU.

## Common problems

- **Black or empty level on start**: the Game Default Map is missing or the map wasn't cooked
  (step 3 above).
- **Car falls through the road**: the track meshes need *Use Complex Collision As Simple*; the
  setup script sets it, so re-run it.
- **No effects**: the Niagara assets weren't set on `BP_MRVehicle` / `BP_MRCheckpoint`, or the
  Niagara plugin is off (the `.uproject` enables it).
