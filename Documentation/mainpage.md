# Tactix {#mainpage}

**Tactix** is a production-grade, modular Game AI plugin for Unreal Engine 5, by
Sleak Software. It bundles the decision-making systems a tactical shooter or
squad game tends to need, built so the core reasoning compiles and unit-tests
without any engine dependency.

## Why use Tactix?

Tactical game AI keeps needing the same systems: utility scoring, planners, cover
reasoning, an influence map, perception memory, and squad logic. Unreal gives you
the scaffolding to *run* AI (Behavior Trees, EQS, Perception, navmesh) but not
those systems themselves, so teams build them again on every project, usually
welded to engine types and impossible to unit-test.

Tactix is that toolbox, built once. The decision-making core has zero engine
dependency, so it's fast and testable; a thin layer wires it into Behavior Trees,
EQS, and data assets so designers use it without writing C++. You get the brains
and keep your Behavior Tree.

For the full rationale, the gaps it fills, the architecture, and when *not* to use
it, see @subpage why.

## What's inside

- **Utility AI** — score-based action selection with response curves.
- **GOAP planner** — A* over a symbolic world-state graph.
- **HTN planner** — hierarchical task decomposition with backtracking.
- **Cover system** — dynamic cover query, scoring, and claim/release.
- **Influence map** — multi-channel 2D spatial heatmaps (danger, control, resource).
- **Perception memory** — per-agent fading stimulus memory.
- **Squad & formation** — role assignment and dynamic movement formations.

## Architecture

Tactix is split into four modules along a strict, upward-only dependency chain.
Nothing ever depends downward, which is what keeps the lower layers engine-free
and testable.

| Module | Layer | Depends on | Notes |
|--------|-------|------------|-------|
| **TactixCore**    | Foundation + primitives | (nothing engine-side) | Handles, pools, arenas, math, curves, agent context, blackboard. No UE headers. |
| **TactixSystems** | AI systems              | TactixCore            | Utility, GOAP, HTN, cover, influence, squad. Still engine-free. |
| **TactixUE**      | Engine integration      | Core + Systems + UE   | AIController, Behavior Tree nodes, EQS, data assets, the agent component. |
| **TactixDebug**   | Tooling (editor only)   | all of the above      | Visual overlays, influence renderer, profiler, decision recorder. |

The one bridge between Unreal and the pure-C++ core is
@ref UTactixAgentComponent, which adapts a pawn to the @ref Tactix::ITactixAgent
interface. Below that line, every system refers to an agent by a
@ref Tactix::FTactixHandle, never an actor pointer.

## Where to start reading

- **Foundation:** @ref Tactix::FTactixPool, @ref Tactix::FTactixArena,
  @ref Tactix::FTactixHandle, @ref Tactix::FTactixCurve.
- **Decision-making:** @ref Tactix::FTactixUtilitySelector,
  @ref Tactix::FTactixGOAPPlanner, @ref Tactix::FTactixHTNPlanner.
- **Spatial:** @ref Tactix::FTactixCoverSystem, @ref Tactix::FTactixInfluenceMap.
- **Engine side:** @ref ATactixAIController, @ref UTactixWorldSubsystem.

Use the sidebar to browse modules, classes, and files, or the search box to jump
straight to a type.
