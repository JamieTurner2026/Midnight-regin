# Steps that need Unreal Editor

The C++ in `unreal/MidnightReign` has not been compiled yet. Expect to send a few compiler
messages back for fixing; paste the full error text.

## 1. Build

1. Install UE 5.6+ and the C++ toolchain for your platform.
2. Right-click `MidnightReign.uproject` → *Generate project files* (switch engine version first if needed).
3. Open the `.uproject`; accept the prompt to build the `MidnightReign` module.

## 2. Get a car mesh (temporary stand-in)

Chaos Vehicles needs a **Skeletal Mesh** with one bone per wheel and a Physics Asset. Until an
original car comes out of Blender, borrow the engine's sample car:

1. Content Browser → *Add* → *Add Feature or Content Pack* → *Vehicle* → *Add to project*.
2. This adds a sports car skeletal mesh and its wheel Animation Blueprint. Its wheel bones are
   `Phys_Wheel_FL`, `Phys_Wheel_FR`, `Phys_Wheel_BL`, `Phys_Wheel_BR`, which are this project's defaults.

The sample car is a placeholder for development only. Shipping cars must be original or licensed.

## 3. Make the vehicle Blueprint

1. Content Browser → right-click → *Blueprint Class* → search `MRVehiclePawn` → name it `BP_MRVehicle`.
2. Select the **Mesh** component: set *Skeletal Mesh Asset* to the car mesh and *Anim Class* to its
   wheel Animation Blueprint.
3. (Optional) right-click → *Miscellaneous* → *Data Asset* → `MRVehicleTuningData`, name it
   `DA_StarterCar`, and assign it to **Tuning** on `BP_MRVehicle`.

## 4. Game mode and map

1. *Blueprint Class* → `MRGameMode` → `BP_MRGameMode`; set *Default Pawn Class* to `BP_MRVehicle`.
2. *File* → *New Level* → *Basic*. Scale the floor up (e.g. 200 × 200) for room to drive. Save as `Maps/L_TestTrack`.
3. *World Settings* → *GameMode Override* → `BP_MRGameMode`.
4. *Project Settings* → *Maps & Modes* → set *Editor Startup Map* and *Game Default Map* to `L_TestTrack`.
5. Place a *Player Start* slightly above the floor. Press **Play**.

## 5. What to check and report

- Car accelerates (W), brakes then reverses (S), steers (A/D), handbrake slides (Shift).
- Space boosts only while moving forward; the NITRO bar drains in about 4 s, refills after release.
- Camera follows smoothly and widens during boost; R puts a flipped car back on its wheels.
- A gamepad works with triggers / left stick / A.
- *Tools* → *Session Frontend* → *Automation* → run the `MidnightReign` tests.

Feel adjustments (too fast, too slippery, steering too twitchy) are made in `DA_StarterCar` and in
Blueprint subclasses of `MRWheelFront` / `MRWheelRear`, with no code changes.

## Commit the editor work

Commit `Content/` (the Blueprint, data asset and map). Large binary assets should use Git LFS
before the art grows: `git lfs track "*.uasset" "*.umap"`.
