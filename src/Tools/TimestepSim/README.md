# timestep-sim

Runs CoD4 movement offline so the timestep can be compared against a vanilla client without the
game, a server, a map or a pair of hands.

It is not a model of the game. The parts that decide the answer are the real thing:

| Piece | Where it comes from |
| --- | --- |
| `PlanSteps` | `src/Game/System/Schedule.cpp`, compiled in |
| Q3 / Q3CPM movement | `src/Game/Player/Movements/Q3.cpp`, compiled in |
| CS movement | `src/Game/Player/Movements/CS.cpp`, compiled in |
| CoD4 movement helpers | `src/Game/Player/Movements/CoD4.cpp`, compiled in |
| Command builder, frame limiter, `PmoveSingle`, CoD4 native movement, collision | ported from KisakCOD |
| `Timestep::Split` | mirrored in `Sim/Timestep.cpp`, kept line for line |

`Shim/Game/Base.hpp` answers the movement sources' include of `Game/Base.hpp` with simulator types,
which is what lets them compile in unmodified rather than being copied and drifting.

Everything runs on a virtual clock. There is no sleeping, no real time and no randomness, so a run
is reproducible and two runs differ only by what is being compared.

## Building

```bash
cmake -S src/Tools/TimestepSim -B src/Tools/TimestepSim/build -G Ninja
```

```bash
cmake --build src/Tools/TimestepSim/build
```

glm is found in the repo's `build/vcpkg_installed` tree by default; pass
`-DGLM_INCLUDE_DIR=<path>` if the repo has not been configured yet. The tool is also part of the
main build, under Tools, where it picks up the `glm::glm` target instead.

## Using it

The self test is the first thing to run. A split whose render rate equals its movement rate has
nothing to split, so it must produce identical movement to vanilla. It covers every mode, both
`g_speed` values, every key combination and a set of `com_maxfps` schedules that change mid run:

```bash
./TimestepSim --self-test
```

Compare one configuration head to head:

```bash
./TimestepSim --compare --mode q3cpm --com-maxfps 333 --sr-maxfps 144
```

A strafe only gains inside a narrow band of sweep rates, and the band moves with the mode and the
rate, so comparing two clients at one arbitrary rate measures the rate rather than the clients.
`--matrix` sweeps the rate and reports each client at its own best, which is the honest answer to
"can the split reach a speed vanilla cannot":

```bash
./TimestepSim --matrix --com-maxfps 333 --sr-maxfps 144
```

Dump every command, to compare against the game's own `iw3sr/Logs/timestep_*.csv`:

```bash
./TimestepSim --timestep --com-maxfps 333 --sr-maxfps 144 --csv on.csv
```

## What decides the answer

`--sleep-us` is the parameter everything turns on: what a one millisecond sleep really costs.
`Com_Frame` busy-waits `while (msec < minMsec) NET_Sleep(1)`, so a sleep that overruns makes vanilla
frames land above the target and the achieved rate fall below `com_maxfps`. At `--sleep-us 1000` a
vanilla client hits 333 exactly and the split has no advantage; at 1200 it manages 285 while an even
grid still delivers 333. The mod measures it on the real machine at startup and reports it in the timestep status.

`--split limiter` is what the game does and the default. `--split grid` is the even division the
mod used to do; it exists only here, so the regression can still show what it cost, and is not
reachable in game.

## The hand

Both clients are driven by one `Hand`: a fixed key schedule and a mouse whose *cumulative* count at
a time is a pure function of that time. Draining any two disjoint spans adds up to draining the
whole, so a client that asks every 3 ms and a client that asks every 7 ms see the same motion,
sampled differently. That is the honest comparison, and it is the point of the tool.

## Known divergences from the game, on purpose

- `Types.hpp` carries `PMF_PRONE` as the mod's `BIT(14) | BIT(0)` rather than the engine's `BIT(0)`,
  because the movement sources compiled in here read the mod's value. `Engine/Pmove.cpp` binds the
  engine's bit for the engine half. Measured inert either way.
- The world is one flat brush. Nothing here says anything about geometry, stairs or wall clipping.
- No weapon is loaded, so the ADS and weapon move-speed scales are unreachable and `PMF_WALKING` is
  never set.
