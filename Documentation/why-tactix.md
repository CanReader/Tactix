# Why Tactix? {#why}

@tableofcontents

This page makes the full case for the plugin: the problem it addresses, what
Unreal already gives you and where it stops, what Tactix adds on top, and the
design decisions that make it worth adopting instead of rolling your own for the
third time. If you just want the headline, read the next section and skip the
rest.

## The short version

Tactical and squad-based game AI keeps needing the same handful of systems:
utility scoring, a planner or two, cover reasoning, a spatial influence map, a
fading memory of what each agent has perceived, and squad coordination. Unreal
gives you the *scaffolding* to run AI (Behavior Trees, EQS, Perception, the
navmesh) but none of those systems themselves. So teams build them over and over,
usually fused to engine types, hard to test, and thrown away at the end of the
project.

Tactix is those systems, built once, as a clean library. The decision-making
core has no Unreal dependency at all, so it is fast and unit-testable; a thin
integration layer wires it into Behavior Trees, EQS, and data assets so designers
can use it without touching C++. You get the brains; you keep your Behavior Tree.

## The problem, in more detail

Writing believable tactical AI is one of the most repeated, least reused jobs in
game development. Look at almost any cover-shooter, squad tactics game, or
stealth game and you will find the same building blocks under the hood:

- A way to weigh many possible actions against the current situation and pick the
  most sensible one, instead of a brittle tree of hard-coded `if` statements.
- A way to *plan* a sequence of actions toward a goal, so the AI can string
  together "move to cover, reload, then peek" without a designer scripting every
  permutation.
- Knowledge of where cover is, which piece blocks a given threat, and which
  pieces are already taken, so two enemies don't dive for the same crate.
- A spatial sense of danger and control across the level, so agents can prefer
  safe routes and contest objectives.
- A short-term memory so an agent stays wary of where it last saw you, rather
  than going blind the instant you break line of sight.
- Group logic so a squad moves and fights as a unit with assigned roles.

These are well-understood techniques with decades of literature behind them. The
problem isn't that they're mysterious; it's that they're *laborious*, easy to get
subtly wrong, and almost never carried from one project to the next because the
last implementation was welded to a specific game's classes.

The result is predictable: schedules lose weeks to re-implementing utility
scoring or an A* planner, the code ends up untested because it can only run inside
a live game, and performance suffers because everything was built on `UObject`s
and per-frame ticks without much thought for budget.

## What Unreal already provides, and where it stops

Unreal's AI offering is genuinely good, but it is intentionally a *framework*, not
a brain. It is worth being precise about the boundary:

| Unreal gives you | Unreal does **not** give you |
|---|---|
| **Behavior Trees** to structure and sequence decisions | A way to *score* options or *plan* multi-step action chains |
| **EQS** to ask spatial questions ("which point scores best?") | A cover model, or a danger/control map to score against |
| **AI Perception** stimuli (sight/hearing/damage) | A decaying per-agent memory you can reason over |
| **Navigation / navmesh** for pathing | Squad roles, formations, or group tactics |
| **Blackboards** to share values between nodes | Any of the decision systems that would write to them |

So the gap is not "Unreal can't do AI." It is that the actual decision systems,
the parts that make an enemy feel like it is *thinking*, are left for you to
build. Tactix fills exactly that gap, and it does so by cooperating with the
Unreal pieces rather than replacing them: it ships Behavior Tree nodes and EQS
providers, so the systems below slot into the workflow your designers already use.

## What Tactix adds

Each system maps directly onto one of the gaps above.

### Utility AI

Score-based action selection. Each action carries a set of *considerations*; each
consideration reads one input from the agent (health, ammo, distance to threat,
speed, in-cover) and runs it through a tunable **response curve** to produce a
value in `[0, 1]`. The action's score is the product of its considerations, with
a compensation factor so an action isn't unfairly penalised just for having more
considerations than its rivals. The highest scorer wins.

This replaces sprawling decision trees with something designers can actually
tune: change a curve, change the behaviour. See @ref Tactix::FTactixUtilitySelector
and @ref Tactix::FTactixCurve.

### GOAP planner

Goal-Oriented Action Planning. The world is described as a small set of boolean
facts (@ref Tactix::FTactixWorldState); each action declares its preconditions,
its effects, and a cost. The planner runs **A\*** over the space of world states
and returns the cheapest sequence of actions that reaches the goal. The AI works
out *how* to achieve "enemy neutralised" on its own, and re-plans when the world
changes. See @ref Tactix::FTactixGOAPPlanner.

### HTN planner

Hierarchical Task Network planning. Instead of searching a state graph, you
describe how a high-level task *decomposes* into smaller ones. Compound tasks
offer several methods in priority order; the planner tries them top to bottom,
backtracking when one fails, until everything bottoms out in directly-executable
primitive tasks. It is the natural fit when you can express your AI as "to do X,
do A then B then C." See @ref Tactix::FTactixHTNPlanner.

GOAP and HTN both exist because they suit different problems: GOAP discovers
plans, HTN encodes authored ones. Tactix gives you both and lets you pick per
behaviour.

### Cover system

A registry of cover points (@ref Tactix::FTactixCoverPoint) with the geometry and
ownership the AI needs: where to stand, which way the cover faces, whether it is
already claimed, and by whom. You can ask for the best cover near an agent that
blocks a specific threat, and claim it so squadmates pick something else. See
@ref Tactix::FTactixCoverSystem.

### Influence map

A multi-channel 2D heatmap over the level (@ref Tactix::FTactixInfluenceMap).
Agents and events *stamp* values into channels (danger, control, resource), the
values *fade* over time, and AI *samples* the field to make spatial choices,
avoid hot zones, hold contested ground, route around threats. It plugs into EQS
so you can score query points by influence directly.

### Perception memory

A per-agent ring buffer of timestamped stimuli whose intensity decays with a
configurable half-life (@ref Tactix::FTactixPerceptionMemory). This is what lets
an agent keep acting on a threat it can no longer see, and gradually forget it,
which is most of what separates "alert" AI from "goldfish" AI.

### Squad and formation

Group coordination: a squad is a roster with roles, a leader, and a current
tactic (@ref Tactix::FTactixSquad); a formation turns slot indices into world
positions relative to the leader's heading, in line / column / wedge / box /
diamond shapes (@ref Tactix::FTactixFormation).

### The integration layer

None of the above requires C++ to *use*, because Tactix ships the Unreal glue:

- **Behavior Tree nodes**: run a utility selection and write the winner to the
  blackboard; execute a GOAP or HTN plan one step per tick; query and claim
  cover; gate subtrees on "in cover?" or "plan active?"; sample the influence map
  into a blackboard key every tick.
- **EQS**: a generator that emits cover points as query items, and a test that
  scores items by an influence channel.
- **Data assets**: designers author actions, considerations, and squad config in
  the editor and tune them without a rebuild.
- **One component** to make any pawn an agent, an **AIController** base that owns
  the planning memory, and a **world subsystem** that holds the shared cover and
  influence data.

## Why the architecture matters

It would be possible to ship all those features as a tangle of `UObject`s and
still call it a plugin. Tactix doesn't, and the reason is the part most worth
understanding.

The plugin is split into four modules with a **strict, upward-only dependency
chain**:

```
TactixDebug    (editor only)  -- overlays, profiler, recorder, debugger tab
TactixUE       (runtime)      -- BT nodes, EQS, components, controller, subsystem
TactixSystems  (runtime)      -- utility, GOAP, HTN, cover, influence, squad
TactixCore     (runtime)      -- handles, pools, arena, math, curves, blackboard
```

Nothing ever depends downward, and the bottom two layers (`TactixCore` and
`TactixSystems`) include **zero Unreal headers**. That single constraint pays for
itself three times over.

### It is testable

Game AI has a reputation for being untestable because it usually can only run
inside a live game. By keeping the reasoning engine-free, Tactix can compile it
with plain CMake and exercise it under Google Test, completely outside the editor.
You can assert that a GOAP plan reaches its goal, that a curve produces the right
score, that the cover query rejects a blocked point, all in milliseconds, in CI,
without launching Unreal. That is a different and much faster development loop than
"change code, recompile the editor, play, watch."

### It is fast

The hot path is built deliberately:

- **No `new` / `delete` while running.** Objects come from fixed pools
  (@ref Tactix::FTactixPool) and planners scratch into a bump
  @ref Tactix::FTactixArena that resets between passes. No allocator churn, no
  fragmentation.
- **No `UObject` overhead** in the core. The reasoning is plain structs and
  arrays, cache-friendly and reflection-free.
- **Agents are referenced by 32-bit generational handles**
  (@ref Tactix::FTactixHandle), not pointers. A stale handle fails safely instead
  of dangling into a recycled object.
- **Thinking is budgeted across frames.** The scheduler
  (@ref Tactix::FTactixAgentScheduler) hands out tick slots under a per-frame
  microsecond budget, so a crowd of agents degrades gracefully instead of
  blowing the frame.

### It is reusable

Because only the thin `TactixUE` layer knows about Unreal, the valuable part, the
actual AI, is portable. The same core could back a tools process, a headless
simulation, or another engine, with a new integration layer on top. You are not
locked into rebuilding the brain if the surroundings change.

## For designers and for debugging

A system only earns its keep if the people balancing the game can use it and the
people fixing it can see inside it.

- **Designers** stay in the editor: actions and considerations live in data
  assets, behaviours are wired in Behavior Trees, spatial scoring goes through
  EQS. Tuning an enemy is editing curves and trees, not recompiling.
- **Debugging** is first-class: a Gameplay Debugger tab shows vitals, state, plan
  progress, and the last decision; world-space overlays draw vital bars, threat
  lines, and plan steps; the influence map renders as a coloured heat grid; a
  decision recorder keeps a rolling history so you can answer "why did it do
  that?" after the fact; and a microsecond profiler tells you which agents and
  which phases cost the most.

## Build it yourself, or use this?

Rolling your own is always an option, and sometimes the right one. The honest
trade-off:

- If you need *one* simple behaviour for *one* enemy type, a hand-written state
  machine is less overhead than adopting a library. Tactix is aimed at games
  where AI is a feature, not a footnote.
- If you expect several enemy archetypes, squads, cover, and spatial tactics,
  the build-it-yourself cost is exactly the weeks-of-reimplementation problem
  described at the top, plus the testing and performance work you'd do from
  scratch. That is the case Tactix is built to remove.

Compared to gluing together assorted marketplace plugins, the advantage here is
coherence: the planners, cover, influence, and utility systems share one design,
one memory model, and one integration layer, instead of each bringing its own
conventions.

## Honest status

Tactix is at version **0.1.0**. It is a real, working implementation with a full
test suite, not a sketch, but it is an early release rather than a
years-hardened, shipped-in-a-AAA-title product. A few designer-facing knobs are
not fully wired yet (for example, the influence-map size fields on the world
subsystem are read once and not yet applied at runtime). None of that changes the
core value, but you should adopt it knowing it is a strong foundation you may
extend, rather than a sealed black box.

If that fits how your team works, the payoff is concrete: you skip the
reimplementation tax, you get AI logic you can actually test, and you get
performance and tooling designed in from the start rather than bolted on later.
