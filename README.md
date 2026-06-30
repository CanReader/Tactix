# Tactix

Game AI plugin for Unreal Engine 5. Utility AI, GOAP, HTN planning, cover system, influence maps, squads, formations — all integrated with UE's Behavior Tree and EQS through BT tasks and decorators you just drop in.

Built by Sleak Software.

---

## Why use it?

Tactical game AI keeps needing the same systems — utility scoring, planners,
cover reasoning, an influence map, perception memory, squad logic. Unreal gives
you the scaffolding to *run* AI (Behavior Trees, EQS, Perception, navmesh) but
not those systems themselves, so teams rebuild them on every project, usually
welded to engine types and impossible to unit-test.

Tactix is that toolbox, built once. The decision-making core (`TactixCore` +
`TactixSystems`) has zero engine dependency, so it's fast and testable; a thin
layer wires it into Behavior Trees, EQS, and data assets so designers use it
without writing C++. You get the brains and keep your Behavior Tree.

- **Skip the reimplementation tax** — utility AI, GOAP, HTN, cover, influence,
  perception, squads/formations are all here and share one design.
- **Actually testable** — the reasoning runs under Google Test with no editor.
- **Built for budget** — pooled/arena memory, no `UObject` in the hot path,
  handle-based agent references, and a frame-budgeted scheduler.
- **Designer- and debugger-friendly** — data-asset tuning, BT/EQS nodes, plus
  overlays, a profiler, a decision recorder, and a Gameplay Debugger tab.

Best fit for games where AI is a real feature (several enemy types, squads,
cover, spatial tactics). For a single simple enemy, a hand-written state machine
may be less overhead. Currently **v0.1.0**: a working, tested foundation, early
rather than AAA-hardened.

---

## What's in here

Four modules with a strict layer ordering — each one only knows about the ones below it:

```
TactixDebug   (editor only)     ← overlays, profiler, Gameplay Debugger tab
TactixUE      (runtime)         ← everything UE: BT tasks, AIController, subsystem, EQS
TactixSystems (runtime)         ← Utility AI, GOAP, HTN, cover, influence, squads
TactixCore    (runtime)         ← math, handles, pool, arena, curves, event bus
```

`TactixCore` and `TactixSystems` have zero UE headers in their public API. They compile standalone through CMake and have a full Google Test suite you can run without opening the editor.

---

## Installation

Copy `Plugins/Tactix/` into your project, add it to your `.uproject`:

```json
{ "Name": "Tactix", "Enabled": true }
```

Regenerate project files and rebuild. That's it — no engine modification needed.

---

## Basic setup

Add `UTactixAgentComponent` to your AI character. It adapts the actor to `ITactixAgent` and exposes the properties the rest of the plugin reads from:

```cpp
UPROPERTY(VisibleAnywhere)
TObjectPtr<UTactixAgentComponent> TactixAgent;

// constructor
TactixAgent = CreateDefaultSubobject<UTactixAgentComponent>(TEXT("TactixAgent"));
```

Keep the properties current (on tick, or on events, wherever makes sense for your game):

```cpp
TactixAgent->CurrentHealth  = GetHP();
TactixAgent->CurrentAmmo    = GetAmmo();
TactixAgent->bInCover       = IsInCover();
TactixAgent->bHasThreat     = CanSeePlayer();
TactixAgent->ThreatLocation = LastKnownPlayerPos;
```

Derive your AI controller from `ATactixAIController`. It owns the scratch arena used by the planners and tracks the active plan state that BT decorators read.

---

## Utility AI

Score-based action selection. Each action has N considerations; each consideration maps a raw input through a response curve to [0, 1]. Final score uses geometric-mean compensation so you don't penalise actions that just have more considerations.

Create a `UTactixUtilityAsset` in the Content Browser and configure actions + considerations in the Details panel. Then drop `UTactixBTTask_RunUtility` into your behavior tree and point it at the asset. It writes the winning action name to a Blackboard FName key.

In C++ you can drive the selector directly:

```cpp
Tactix::FTactixUtilitySelector Selector;
Selector.AddAction(MakeUnique<FFlankAction>(), { ConsiderationA, ConsiderationB });

int32 Winner = Selector.SelectBest(AgentCtx);
```

Five curve shapes: `Linear`, `Quadratic`, `Logistic`, `Exponential`, `Custom`. Configured per-consideration.

---

## GOAP

A* over a 64-bit boolean world state. Finds the cheapest action sequence to reach a goal state. Plans are cached per `(agentHandle, goalHash)` and only recomputed when the world state changes enough to invalidate them.

```cpp
Tactix::FTactixWorldState Current, Goal;
Current.SetBool(HasAmmo, true);
Goal.SetBool(EnemyDead, true);

Tactix::FTactixGOAPPlan Plan = Planner.Plan(Current, Goal, ScratchArena);
if (Plan.bValid)
{
    Controller->ActiveGOAPPlan  = Plan;
    Controller->GOAPPlanStep    = 0;
    Controller->bGOAPPlanActive = true;
}
```

`UTactixBTTask_ExecutePlan` steps through the plan one action per tick. `UTactixBTDecorator_PlanActive` blocks the subtree when no valid plan is running.

---

## HTN

SHOP-style hierarchical task decomposition. Compound tasks have ordered methods; the planner tries them top-to-bottom and backtracks if decomposition fails. The output is a flat primitive sequence, same execution path as a GOAP plan.

```
Task: AssaultEnemy
├── Method 0  (condition: bHasStealth && EnemyCount == 1)
│   └── [StealthKill, Reposition]
├── Method 1  (condition: CurrentAmmo > 0)
│   └── [TakeCover, Shoot, Reload, Shoot]
└── Method 2  (always)
    └── [Retreat, RadioForHelp]
```

---

## Cover system

Fixed-pool registry (512 slots, adjustable via template parameter). Register cover points at BeginPlay on your cover volumes; unregister at EndPlay.

```cpp
auto& CoverSys = Subsystem->GetCoverSystem();
auto [Handle, Ptr] = CoverSys.Register(CoverPoint);
// store Handle — needed to unregister and for generational validity checks
```

`UTactixBTTask_ClaimCover` queries the best unclaimed point for the current threat direction and claims it automatically. `UTactixBTDecorator_InCover` checks whether the agent still holds a claim.

For EQS-driven queries, `UTactixEQSGenerator_Cover` emits all registered points as candidates, and `UTactixEQSTest_Influence` can score them by influence channel value.

---

## Influence maps

Multi-channel float grid (4 channels: Danger, Control, Resource, one spare). Lives in `UTactixWorldSubsystem`.

```cpp
auto& Map = Subsystem->GetInfluenceMap();

Map.Stamp(0 /*Danger*/, EnemyWorldPos, Radius, 1.0f);
Map.Decay(0, 0.05f, DeltaTime);   // 5% per second
float Danger = Map.Sample(0, AgentWorldPos);
```

`UTactixBTService_InfluenceQuery` samples a configured channel at the agent's position every 0.25 s and writes the result to a Blackboard float key. No extra code needed.

---

## Squads and formations

```cpp
Tactix::FTactixSquad<8> Squad;
Squad.AddMember(Handle, ETactixSquadRole::Assault);
Squad.SetTactic(ETactixSquadTactic::Flank);

Tactix::FTactixFormation<8> Formation;
Formation.SetKind(ETactixFormationKind::Wedge);
Formation.Reshape(MemberCount);

FTactixVec3 SlotPos = Formation.GetSlotPosition(SlotIndex, LeaderPos, LeaderYaw);
```

Formation types: Wedge, Line, StaggeredColumn, Box.

---

## Perception memory

Per-agent ring buffer of timestamped stimuli with configurable decay. Extensible via `ITactixPerceptionChannel` for sight, sound, damage, custom types.

```cpp
Tactix::FTactixPerceptionMemory Memory(32 /*capacity*/, 5.0f /*decay half-life seconds*/);
Memory.Record(Stimulus);
float MaxThreat = Memory.GetMaxIntensity(CurrentTime, 3.0f /*lookback window*/);
```

---

## Debug

**Gameplay Debugger** — press apostrophe in PIE, tab to "Tactix". Shows vitals, state flags, plan progress, and the last recorded decision. Replicated automatically. No setup.

**Visual overlays** — call `FTactixVisualDebugger::DrawAgent(...)` from your controller. Draws vital bars, threat lines, and plan steps in world space. Compiled out in Shipping.

**Influence heatmap** — `FTactixInfluenceRenderer::DrawAllChannels(...)` draws each channel as a colour-coded box grid in the world.

**Decision recorder** — `FTactixDecisionRecorder::Get().Record(...)` stores every decision in a 512-slot ring buffer with timestamps and a vitals snapshot. `DumpToLog(N)` prints the last N to the output log.

**Profiler** — wrap any code block with `FTactixTimerScope(AgentHandle, FName("GOAPPlan"))`. Microsecond resolution. `FTactixAgentProfiler::Get().Report(5)` logs the 5 slowest agents.

---

## Running the tests

```bash
cd Plugins/Tactix/Tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && ctest --output-on-failure
```

Tests cover all of TactixCore and TactixSystems and have no UE dependency.

---

## A few rules the code follows

- No `new`/`delete` on the hot path. Pool and arena allocators only.
- No UE headers in TactixCore or TactixSystems public headers.
- No STL containers in L1/L2/L3. `TArray` only in TactixUE.
- `FTactixHandle<T>` is the only way to reference an agent across systems. No raw pointers stored across frames.
- Consideration evaluators take `FTactixAgentContext` by const ref — zero side effects in scoring.

---

## License

Copyright © Sleak Software. All rights reserved. Not for redistribution.
