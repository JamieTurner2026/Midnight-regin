# Steps that need Unreal Editor

Everything that could be prepared without the editor is already in the repo: C++ gameplay code,
the car, parts and track as FBX files, and a script that imports them and builds the test map.

Neither the C++ nor the import script has been run in Unreal yet. Expect to send some error
messages back for fixing; paste the full text from the Output Log.

## 1. Build the code

1. Install UE 5.6+ and the C++ toolchain for your platform.
2. Right-click `unreal/MidnightReign/MidnightReign.uproject` → *Generate project files* (use
   *Switch Unreal Engine version* first if you are not on 5.6).
3. Open the `.uproject` and accept the prompt to build the `MidnightReign` module. The editor
   will say the startup map is missing and open an empty level; that is expected until step 2.

## 2. Run the visual setup script

*Tools* → *Execute Python Script…* → choose `unreal/MidnightReign/Scripts/setup_visuals.py`.

It creates, under `Content/MidnightReign`:

- neon materials (`Materials/MI_MR_*`)
- the Solaris GT skeletal mesh, `DA_SolarisGT` tuning asset and `BP_MRVehicle`
- the customization part meshes
- the track meshes and the map `Maps/L_SolarisTestBlock` with checkpoint arches, night lighting
  and a Player Start
- `BP_MRGameMode`, set as the map's game mode

Open *Window* → *Output Log* and look for the `[MR setup] summary`. Each step reports `OK` or
`FAILED` with the manual alternative. The script can be run again safely.

## 3. Two manual steps (about five minutes)

**Physics asset.** Open `Vehicles/SolarisGT/SK_SolarisGT_PhysicsAsset`. Delete the bodies on the
four `Phys_Wheel_*` bones so only the `Root` body remains (wheels are simulated by Chaos, not by
physics bodies). Select the `Root` body and check it roughly covers the car body; if not,
right-click → *Regenerate Bodies* with *Primitive Type: Box*.

**Wheel animation.** Without this the car drives but the wheels do not visibly spin or steer.

1. Right-click in `Vehicles/SolarisGT` → *Animation* → *Animation Blueprint*. Pick the
   `SK_SolarisGT_Skeleton`, parent class **VehicleAnimationInstance**, name it `ABP_SolarisGT`.
2. In its AnimGraph: add *Mesh Space Ref Pose* → *Wheel Controller for WheeledVehicle* →
   *Component To Local* → *Output Pose*. Compile and save.
3. Open `BP_MRVehicle`, select **Mesh**, set *Anim Class* to `ABP_SolarisGT`.

## 4. Play

Open `Maps/L_SolarisTestBlock` and press **Play**.

- W/S/A/D or arrows drive, Space is nitro, Left Shift is the handbrake, R resets the car.
- Gamepad: triggers, left stick, A for nitro.
- The NITRO bar drains in about 4 seconds and refills after you release Space.
- *Tools* → *Session Frontend* → *Automation* → run the `MidnightReign` tests.

Handling changes (too fast, too slippery, twitchy steering) are made in `DA_SolarisGT` and in
Blueprint subclasses of `MRWheelFront` / `MRWheelRear`, with no code changes.

## 5. Checkpoint run (the first playable goal)

Written, not compiled: the C++ and script changes below have not run in Unreal yet.

**The goal.** Drive through three gates in order before 60 seconds run out: `MR_Gate_0` (a third of
the lap), `MR_Gate_1` (two thirds) and `MR_Gate_2_Finish` (the start/finish line). The clock starts
when you press Play. The top of the screen shows the time left (pink in the last 10 s) and the next
gate; at the end you get **RUN COMPLETE** with your time, or **TIME UP**. **Enter** or gamepad
**Start** puts you back on the line and starts again. The other three arches are scenery.

The setup script (step 2) already places the gates as `BP_MRCheckpoint` with Order 0, 1, 2, adds a
warm light under each of the 20 street lamps, and sets `BP_MRGameMode` > *Time Limit* from
`track_layout.json`. Change the time there if 60 s is too easy or too hard.

**Three Niagara effects (about 15 minutes).** The code plays them; you create the assets. In
`Content/MidnightReign/FX`, right-click → *FX* → *Niagara System* → *New system from a template*:

1. `NS_MR_CheckpointFlash` from **Simple Sprite Burst** (one burst, no loop). Colour neon cyan
   `#00FFFF`, a large sprite size, about 0.5 s lifetime. Open `BP_MRCheckpoint`, select
   **PassEffect**, set *Niagara System Asset* to it.
2. `NS_MR_NitroFlame` from **Fountain** (looping). Velocity pointing back (−X), short lifetime
   (0.1–0.2 s), colour neon pink `#FF1493` fading to blue. In `BP_MRVehicle`, select **NitroFlame**
   and set the asset. It turns on and off with the nitro (`OnBoostStarted` / `OnBoostStopped`).
3. `NS_MR_TyreSparks` from **Fountain** (looping). Low, wide spray with gravity, orange-white, plus a
   grey smoke emitter if you like. In `BP_MRVehicle`, select **DriftSparks** and set the asset. It
   runs while the handbrake is held above 35 km/h (*Drift Sparks Min Kmh*).

**Blueprint touches (optional).** In `BP_MRCheckpoint`, implement **On Next Changed** (make the
next gate's neon brighter, e.g. raise *EmissiveStrength* on a dynamic material instance) and
**On Passed** (play a sound; *Is Finish* is true on the last gate). `BP_MRGameMode` exposes
**On Run Started / On Checkpoint Passed / On Run Won / On Run Lost** for music or camera moments.

**Play check.** Report back what happened at each line:

- [ ] Throttle, brake, steer, nitro, handbrake and reset (R) work; Enter restarts the run.
- [ ] The clock counts down; passing a gate out of order does nothing; the three gates in order give
  RUN COMPLETE; waiting past 60 s gives TIME UP.
- [ ] The flash plays at each gate, the nitro flame shows while boosting, sparks show in a handbrake slide.
- [ ] The street lamps light the road and the wet road reflects them and the neon.
- [ ] Frame rate stays smooth (*stat fps* in the console).
- [ ] *Session Frontend* → *Automation*: the `MidnightReign` tests pass (now including `Game.CheckpointRunWiring`).

Then build the standalone version: [standalone-build.md](standalone-build.md).

## If the script fails completely

Do it by hand: import `assets/export/SK_SolarisGT.fbx` as a Skeletal Mesh, import the
`assets/export/track/*.fbx` files as Static Meshes with *Collision Complexity: Use Complex
Collision As Simple*, drag the track meshes into a new level at 0,0,0, create `BP_MRVehicle`
(parent `MRVehiclePawn`) with the mesh assigned, and `BP_MRGameMode` (parent `MRGameMode`) with
that pawn. As a last resort the engine's sample car (*Add* → *Add Feature or Content Pack* →
*Vehicle*) uses the same wheel bone names and works as a stand-in.

## Commit the editor work

Commit the `Content/` folder. Set up Git LFS before it grows: `git lfs track "*.uasset" "*.umap"`.

## Not covered yet

Sound for nitrous and the gates (the events are ready), on-screen touch controls, opponents and
lap timing over several laps (Milestone 2), and attaching customization parts at runtime (Milestone 4).
