# Plan: Asymmetric 5v1 Mansion Horror Game (UE 5.5+, greybox)

## Context
This is a new Unreal project; the working directory is empty. Five **Survivors** play in first person and try to escape a trapped mansion. One **House** player sees the mansion from above, like a board game, and uses indirect powers to lure AI monsters onto the Survivors. The House sees a Survivor's exact position only when one of these is true:
1. a monster has an unobstructed line of sight to them (in light *or* dark),
2. they have stayed in one spot too long, or
3. they are standing in a very dark area.

Decisions already made: C++ core plus Blueprints, listen server (LAN, Steam later), UE 5.5+, greybox art, and indirect House powers paid for from a **Dread meter** that fills faster when Survivors camp. The design is still growing; you'll add more later. Everything below is data-driven so new items and abilities slot in without restructuring.

**Environment caveat:** this container can't run Unreal. I'll write the C++ module, config, Blueprint specs and data-asset definitions here. You compile and build the greybox maps in the editor on your machine. I recommend a GitHub repo so we can keep iterating.

---

## 1. Core design (the rules the code enforces)

### Match flow
1. **Pre-game (House only, about 60 s; Survivors see a loading or briefing screen):**
   - The game engine scatters items at random spawn points. The House also places its own allotment.
   - The House **moves up to 3 items** to other valid spawn points.
   - The House **chooses the Magic Door location** from a set of candidate spots, which stay hidden from Survivors.
2. **Match:** Survivors search, hide and survive. The House builds Dread and spends it.
3. **Two endings:**
   - **A. Ritual (early win):** Survivors solve the mystery of *who died and how the house became haunted*. Once they have enough **evidence** and the **deceased's bones**, they perform a **ritual** that frees the soul and ends the haunting. Every Survivor still in play wins.
   - **B. Timer expires (Final Hunt):** the **Magic Door** opens at the House-chosen spot and Survivors can escape through it. At the same moment the House's **Dread meter fills completely and stays full**: no costs, cooldowns cut down to a minimum, everything available to eliminate players before they reach the door.
4. **End:** a scoreboard of escaped, freed and eliminated Survivors.

### The mystery (ritual ending)
- At match start the server picks one **case** from a data table (`DT_Cases`), for example: victim, cause of death, room where it happened, bone location.
- **Evidence items** (diary pages, photos, letters and so on) spawn around the house. Each one reveals a clue about *this* case, and some are red herrings that belong to other cases.
- **Bones** are a single heavy carryable item. While carrying them a Survivor moves slower and can't sprint or use two-handed weapons. *Assumption: the bones spawn in a location tied to the case.*
- **Ritual:** take the bones and at least N pieces of evidence to the ritual site (the case's death room) and channel for about 20–30 s. While channelling:
  - it makes noise,
  - it **reveals all channelling Survivors to the House**,
  - it is interrupted if they're hit, downed or possessed. Interrupted progress decays slowly rather than resetting.
- *Assumption:* the "solve" is collect-based (gather enough of the right evidence), not a deduction screen where players pick the answer. A deduction step can be added later.
- The House's pre-game item moves also apply to evidence and bones, so the House can hide the bones far from the ritual room.

### Information and light
- **Monsters see in the dark.** Their sight is blocked only by geometry: walls, doors, beds, pillars, wardrobes and any other movable furniture. This uses a dedicated `MonsterSight` trace channel that all occluders block.
- **Darkness** reveals a Survivor to the House directly.
- **Light** doesn't hide you from monsters. It keeps you hidden from the House and quickly lowers Fear. So light is about *information and nerve*, and *cover* is what protects you from monsters.
- **Camping** in one spot triggers the reveal timer and speeds up Dread gain.
- **What the House always sees:** floor plans (per-floor cutaway), doors (open, closed, locked or broken), lights, monsters, furniture, and noise pings that show the approximate room. When a reveal ends, a last-known-position ghost stays behind and fades.

### Fear meter (per Survivor, 0–100)
- **Rises from:**
  - darkness,
  - a monster nearby or in view,
  - House abilities used on or near the Survivor,
  - a teammate being downed.
- **Falls quickly in light** (from any gameplay light source, held lanterns included).
- **Effect:** adds a bonus of `Fear/100 × 7.5` percentage points (**capped at +7.5%**) to:
  - the **stumble chance** (rolled while sprinting and when the House uses Trip),
  - the **drop-item chance** (rolled when startled, for example by a door slam nearby, a monster sighting or getting hit).
  Which actions trigger which roll is set per event in data.
- Survivor HUD: subtle feedback (heartbeat, vignette, hand shake) rather than a raw number.

### Dread meter (House resource)
Dread fills from a passive rate, a bonus for each camping Survivor and a small bonus per monster sighting. Abilities cost Dread and have cooldowns.

### House abilities and Survivor counters
| House ability | Effect | Counter |
|---|---|---|
| **Drag** | Pulls a revealed Survivor about 3 m | Salt pouch (brief immunity) |
| **Slam Door** | Shuts a door with a loud noise that lures monsters | Door wedge |
| **Shatter Glass** | Makes a noise and raises the room's ambient noise floor, which masks monster audio | Earplugs / tuning fork |
| **Trip** | Makes a sprinting, revealed Survivor stumble (base chance plus the Fear bonus) | Steady tonic |
| **Lock Door** | Locks a door temporarily | **Keys**: unlock it, each use has a 33% chance the key breaks, unlimited uses otherwise |
| **Possess Monster** | The House steers one monster for about 5 s | none (costs a lot of Dread) |
| **Possess Survivor** | The House overrides a revealed Survivor's movement for about 2 s. The victim's **vision turns heavy** (dark vignette, blur, desaturation, hidden HUD), so they can't tell teammates where they're going | Charm / crucifix (breaks it, consumed) |
| **Voice Snip** | Captures a short clip (about 3–5 s) of a Survivor's recent voice chat. The House can later **replay it from any spot in the house** to mislead Survivors or lure them. *(Assumption about what the clip is used for; confirm.)* | Allowed only on a target that is **camping past the threshold**, or that a **monster has seen and targeted for more than 3 s straight** |

**Possession restriction:** while possessing a monster or a Survivor, the House's camera switches to **that pawn's first-person view only**. The top-down board, the markers and all other abilities are unavailable until the possession ends.

Counters are GameplayTags granted by item effects. Abilities list them as blocking tags, so adding new counters is a data-only change.

### Survivor tools and weapons
| Item | Behaviour |
|---|---|
| Lantern | Omni light in an area, lowers Fear for the Survivor and anyone nearby. Fuel optional (tunable). |
| Flashlight | Cone light. Battery optional. |
| Glow stick | Throwable or placeable light that lasts a set time. Keeps a dark corner out of "dark reveal". |
| Revolver | 3 shots. **Stuns** a monster (no kills). |
| Double-barrel shotgun | 2 shots. Long stun plus knockback. |
| Tracker | Signals when a monster is within a set radius. No direction at base level. |
| Keys | Unlock House-locked doors. 33% chance to break on each use. |
| Counter items | Salt, wedge, earplugs, tonic, charm (from the table above). |

Items are `UPrimaryDataAsset` definitions (`DA_Item_*`) plus a GAS ability or effect. The inventory is a small slot count (tunable), and the item drop roll uses the Fear bonus.

### Furniture and barricades
- Wardrobes, nightstands, beds and so on carry `UMovableFurnitureComponent`, which holds a **Mass** value and an optional **HP**.
- A Survivor *grabs* the piece and pushes or pulls it. Each Survivor contributes push strength, and **several Survivors can assist** on the same piece:
  - Speed = f(sum of strength − mass).
  - If the strength is too low the piece doesn't move.
  - Heavy pieces need 2 or 3 people.
- Movement is **server-authoritative kinematic sweeps**, not simulated physics, so replication stays smooth and pieces can't be launched by accident.
- Furniture blocks the `MonsterSight` channel, so it hides Survivors from monsters and can be used as a barricade.

### Monsters
- Hearing is driven by noise events; sight uses the `MonsterSight` channel and works regardless of light.
- StateTree states: Patrol → Investigate → Chase → Attack/Down → **Breach** → Stunned → Return.
- **Breach:** when its path is blocked by a closed or locked door, or by furniture, the monster hits it. Each hit takes HP away and makes a noise. At 0 HP the object breaks (in greybox: swapped for rubble, or for an opened, broken door).
- Stuns from guns pause the StateTree for a set time, and the monster then resumes Chase or Investigate.

---

## 2. Technical architecture

### Frameworks
- **Gameplay Ability System (GAS):** House abilities, Dread and Fear as attributes, items, counters, stuns.
- **Enhanced Input:** separate input setups for Survivor, House and House pre-game.
- **AIPerception** plus **StateTree** for monsters.
- **Lumen** is for visuals only; gameplay uses its own light system.

### Key systems
- **Gameplay lighting:**
  - `UGameplayLightComponent` attaches to fixtures, lanterns, flashlights and glow sticks.
  - `UIlluminationSubsystem` keeps a registry of these lights. `GetIlluminationAt(Location)` uses room ambient light plus each light's falloff, with a trace for occlusion, evaluated at 4–5 Hz.
  - Darkness reveal and Fear decay both use this one value.
- **`UExposureComponent` (server):**
  - reveal reasons stored as a bitmask: `Camping`, `Dark`, `SeenByMonster`,
  - `LastKnownLocation`,
  - the stationary timer.
- **`UFearComponent` / Fear attribute:** gain and decay each tick. It exposes `GetStumbleBonus()` and `GetDropBonus()`, both capped at 0.075.
- **House visibility:**
  - Survivor meshes are hidden locally on the House client.
  - `ARevealMarker` and the ghost markers show only while the Survivor is revealed.
  - **Caveat:** a House player who hosts has the full game state, which is fine for a prototype. The later fix is a dedicated server or Replication Graph relevancy filtering.
- **Pre-game:**
  - `AMansionGameState` holds the match phase (`PreGame`, `Playing`, `MagicDoorOpen`, `Ended`).
  - `AItemSpawnPoint` actors are spawn points for items.
  - `AMagicDoorCandidate` actors are the possible Magic Door locations.
  - Server RPCs on the House PlayerController check every action: the 3-move limit, valid points only and the phase.
- **Possession:**
  - **Monster:** the StateTree pauses and the House's input reaches the server by RPC.
  - **Survivor:** the server sends a replicated input override, and the owning client feeds it in as movement input. The client also applies a "heavy vision" post-process material and hides its HUD.
- **Destructibles:** `UBreakableComponent`, a shared HP and on-break component used by doors, glass and furniture.
- **Possession camera:** the House PlayerController calls `SetViewTargetWithBlend` to switch to the possessed pawn's camera. A gameplay tag, `State.House.Possessing`, blocks every other House ability and hides the House HUD and markers.
- **Final Hunt:** when the phase changes to `MagicDoorOpen`, the server applies an infinite GameplayEffect that sets Dread to its maximum and removes costs. The final-phase cooldowns live in `DT_Tuning`.
- **Mystery:** `UCaseDefinition` (a data-table row) plus `AEvidencePickup`, `ABonesPickup` and `ARitualSite`. The ritual is a GAS channel ability with shared progress stored on the `ARitualSite`.
- **Proximity voice and Voice Snip** *(technical risk; do a spike early):*
  - Voice chat goes over UE VOIP (Online Subsystem voice, replicated through the server) with `UVOIPTalker` attenuation for proximity.
  - For Voice Snip, the server keeps a **ring buffer of each Survivor's last ~10 s of compressed voice packets**. Snipping copies a slice of that buffer. Replaying sends the packets to clients, which play them through a fake talker at the chosen world location.
  - Fallback if packet capture is too hard: route each talker into a dedicated submix and record it locally with the submix recording API, then send the result to the server.
  - Only snip-eligible players' audio is ever kept. The buffer stays in memory and is never written to disk.

## 3. Project layout (C++ module `Mansion`)
```
Source/Mansion/
  Core/       MansionGameMode, MansionGameState (phase, timer), MansionPlayerState (role, status), MansionTypes.h
  Survivor/   SurvivorCharacter, SurvivorPlayerController, ExposureComponent, FearComponent, InventoryComponent
  House/      HousePawn, HousePlayerController (pre-game + match), HouseAttributeSet (Dread), TopDownTargetActor
  Abilities/  GA_HouseBase, GA_Drag, GA_SlamDoor, GA_ShatterGlass, GA_Trip, GA_LockDoor, GA_Possess*,
              GA_Fire (revolver/shotgun), GA_UseItem, GA_Unlock
  Lighting/   GameplayLightComponent, IlluminationSubsystem, RoomVolume
  AI/         MonsterCharacter, MonsterAIController, StateTree tasks (Investigate, Chase, Breach, Stunned)
  World/      MansionDoor (slam/lock/wedge/breakable), GlassPane, LightFixture, MovableFurnitureComponent,
              BreakableComponent, ItemSpawnPoint, ItemPickup, MagicDoorCandidate, MagicDoor
  UI/         ViewModels for the Survivor HUD, House HUD and House pre-game (UMG in Blueprints)
Content/  Maps/L_Mansion_Greybox, Data/(DA_Item_*, DT_Tuning, GE_*), Blueprints/, PostProcess/M_PossessedVision
```
All tunable numbers live in `DT_Tuning` or the data assets, so you can balance without recompiling.

## 4. Build order (milestones)
1. **Skeleton:** roles, the two pawns, input, and 6-player listen-server PIE.
2. **Greybox mansion:** 2 floors, rooms, doors, lights, windows, the House cutaway camera.
3. **Lighting, Exposure and Fear:** reveal markers and ghosts on the House client; Fear gain and decay. *First playtest.*
4. **Monsters:** `MonsterSight` channel, perception, StateTree, sight reveal, down, revive, eliminate.
5. **Furniture:** grab, push, assisted push, sight blocking; monster breach and breakables.
6. **GAS and Dread:** Slam, Shatter, Lock Door; keys.
7. **Targeted abilities:** Drag, Trip (with Fear), Possess monster and Survivor (heavy vision).
8. **Items and weapons:** inventory, lights, glow sticks, guns and stun, tracker, counters, Fear-based drop chance.
9. **Match flow:** pre-game phase (item placement, 3 moves, Magic Door choice), timer, Magic Door and Final Hunt, end screen.
10. **Mystery and ritual:** case data, evidence, bones, ritual channel.
11. **Online:** sessions (Null for LAN, then Steam), proximity voice, Voice Snip. *Do a voice-capture spike as early as milestone 1, because this is the riskiest feature.*

## 5. Verification
- **PIE:** 6 players as a listen server, plus Network Emulation.
- **Debug cvars:**
  - `mansion.ShowExposure`: illumination, timer and reveal reasons.
  - `mansion.ShowFear`: the Fear value and current roll bonuses.
  - `mansion.ShowMonsterSight`: sight traces.
- **Automation tests:**
  - illumination falloff and occlusion,
  - exposure transitions,
  - Fear bonus capped at 7.5%,
  - the key's 33% break rate (statistically, over N rolls),
  - assisted push strength against mass,
  - the pre-game 3-move limit.
- **Scenario checks:**
  - A monster sees a Survivor in a dark room, but not behind a wardrobe.
  - The House client never shows an unrevealed Survivor.
  - The House can't move a 4th item.
  - The Magic Door opens at the chosen spot when the timer ends.
  - A possessed Survivor's screen goes heavy and their HUD is hidden.
  - While possessing, the House sees only the possessed pawn's view and can't cast anything else.
  - Voice Snip is rejected on a target that isn't eligible, and is accepted after 3 s of continuous monster targeting.
  - The ritual completes with enough evidence plus the bones, and is interrupted when a channeller is hit.
  - When the timer expires, Dread jumps to full and stays there.

## 6. Open questions (defaults used until you decide)
- **Win condition:** how many escapes (Magic Door) count as a Survivor win? Does a ritual free everyone, including eliminated players? *Default: 3 or more escapes; the ritual is a full team win.*
- **Downed Survivors:** *Default: revivable for 30 s, then eliminated and spectating.*
- **Voice Snip:** is the clip replayed as a lure or to deceive, as assumed above? Can monsters hear the replay?
- **Mystery depth:** is collecting the right evidence enough to solve the case, or should Survivors have to pick the answer on a deduction screen? *Default: collecting is enough.*
