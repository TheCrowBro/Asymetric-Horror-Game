# Mansion: 5v1 asymmetric horror (UE 5.5, greybox)

Five Survivors play in first person and try to escape a haunted mansion. One **House** player watches from above like a board-game overseer and lures monsters onto them. The House only sees a Survivor's exact position when:

- a monster sees them,
- they camp in one spot too long, or
- they stand in darkness.

The full design and roadmap are in [`Docs/Design.md`](Docs/Design.md).

## Status

| Milestone | State |
|---|---|
| 1. Skeleton: roles, pawns, input, 6-player listen server | Done |
| 2. Greybox mansion: 2 floors, rooms, ramp, House cutaway camera | Done (procedural; doors/windows as actors are still to come) |
| 3. Gameplay lighting, Exposure (reveal), Fear | Done |
| 4–11. Monsters, furniture, GAS/Dread, abilities, items, match flow, mystery, voice | Next |

## Setup

1. Install **Unreal Engine 5.5** (or newer) and a C++ toolchain: Visual Studio 2022 with the *Game development with C++* workload, or Xcode/Rider.
2. Right-click `Mansion.uproject` → **Generate Visual Studio project files**.
3. Open `Mansion.sln`, then build the **Development Editor** target. Alternatively, open the `.uproject` and let it compile.
4. In the editor, create the test map:
   - **File → New Level → Empty Level**.
   - Place **Greybox Mansion Builder** at `0,0,0`. Search for it in *Place Actors*. Leave it unrotated.
   - Optional: add a dim **Sky Light** (intensity about 0.1). The mansion is meant to be dark.
   - Save the map as `Content/Maps/L_Mansion_Greybox`, then set it as the *Editor Startup Map* and *Game Default Map* in Project Settings → Maps & Modes.
   - The game mode is already set globally to `MansionGameMode`.

## Playtesting in the editor

- **Play** dropdown → *Number of Players* = **6**, *Net Mode* = **Play As Listen Server**.
- By default the **first client becomes the House** and the host is a Survivor (cvar `mansion.HouseSlot`, default `1`).
  - For solo testing of the House view, run `mansion.HouseSlot 0` before pressing Play.
  - A player can also join with the URL option `?Role=House`.
- The match opens in **PreGame** (60 s; Survivors are frozen). Next comes **Playing** (15 min), and then **MagicDoorOpen**. Shorten these under *Project Settings → Game → Mansion Tuning*.

### Controls

| Survivor | House |
|---|---|
| WASD move, mouse look | WASD pan |
| Shift sprint, Ctrl/C crouch | Mouse wheel zoom |
| F flashlight | Q / E floor down / up |

### Debug console variables

| Cvar | Shows |
|---|---|
| `mansion.ShowExposure 1` | Each Survivor's illumination, camping timer and reveal reasons, drawn on the listen-server view |
| `mansion.ShowFear 1` | Raw Fear, roll bonus and light value on the Survivor HUD |
| `mansion.ShowLights 1` | Gameplay light radii and room bounds |

### Things to try

- **Dark reveal:** stand in an unlit room. The Survivor HUD warns "too dark", and the House sees a **purple** beacon. Turning on your flashlight (it counts as half self-illumination) is often enough to hide.
- **Camping:** stand still for 20 s. The House sees a **yellow** beacon.
- **Ghosts:** when a reveal ends, the House sees a shrinking last-known-position ghost.
- **Fear:** rises in darkness and drains quickly in light. It adds up to +7.5% to future stumble and drop rolls.
- **Cutaway:** as the House, press Q/E. Everything above the current floor is hidden, and so is the roof.

## Tests

Open *Tools → Session Frontend → Automation*, filter for `Mansion.Logic` and run the tests. They cover:

- illumination falloff and cone,
- the Fear cap (+7.5%) and Fear gain/decay,
- camping, darkness, monster-sight linger, the 3 s Voice Snip streak and forced reveals.

From the command line:

```
UnrealEditor-Cmd.exe Mansion.uproject -ExecCmds="Automation RunTests Mansion.Logic; Quit" -unattended -nullrhi
```

## Code map

```
Source/Mansion/
  Core/      GameMode (roles, phases), GameState (phase timer), PlayerState (role/status),
             MansionTuning (all balance numbers), MansionInput (runtime Enhanced Input setup)
  Lighting/  IlluminationSubsystem (gameplay light query), GameplayLightComponent, RoomVolume
  Survivor/  SurvivorCharacter, ExposureComponent (reveal rules), FearComponent
  House/     HousePawn (top-down camera, cutaway, reveal markers), RevealMarker
  World/     GreyboxMansionBuilder (procedural greybox), LightFixture
  UI/        MansionHUD (canvas debug HUD)
  Tests/     Automation tests for the pure logic
```

### Notes and deviations from the design doc

- **Tuning** lives in a `UDeveloperSettings` class (Project Settings), not a DataTable. It can be edited without creating any assets.
- **Input** actions and mapping contexts are built in C++ at runtime, so no `IA_`/`IMC_` assets are needed for the greybox.
- **Survivor visibility on the House client:** pawns and flashlight glow are hidden *locally* on the House's machine. A House player who hosts could cheat, so keep the House on a client (the default). Server-side relevancy filtering comes later.
- **Monster sight ignores light** by design. The darkness check is only for revealing Survivors to the House.

**Design questions this build surfaced:**
- Should the House see the *glow* of Survivors' lights? Right now it doesn't.
- Should a Survivor's own flashlight count against the darkness reveal? Right now it gives 0.5× self-illumination.
