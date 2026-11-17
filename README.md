# Dots and Boxes — CMPS 241 Course Project

A console version of Dots and Boxes in C, built to run on a text-only Linux VM
(we use Ubuntu in VirtualBox) and start automatically on boot.

| Sprint | Content | Status |
|---|---|---|
| 1 | Two players (A vs B), 4×5 board, Makefile, tests, GDB/Valgrind, VM auto-start | ✅ |
| 2 | Hard-level bot + complexity analysis | ⏳ |
| 3 (bonus) | Network play + multithreading | ⏳ |

## Build and run

```sh
make                 # builds ./dotsandboxes (warnings are errors)
./dotsandboxes       # play (colours, bigger board centred on screen)
./dotsandboxes --plain               # compact spec layout, no colours / clearing
./dotsandboxes --rows 2 --cols 3     # other board sizes (1..9)
make test            # unit tests + full scripted games
make valgrind        # every scripted game under Valgrind
make gdb             # start the game in GDB
```

**How to play:** type the row and column of two neighbouring dots, e.g.
`0 0 0 1` draws the line from dot (0,0) to dot (0,1). Dots can be given in either
order. Completing a box claims it, and you play again. Most boxes wins.

```
  0 1 2 3 4 5
0 .-.-. . . .
  |A|B|
1 .-.-. . . .
```

Running on the VM: see [docs/VM_SETUP.md](docs/VM_SETUP.md).

## Project structure

```
src/
  main.c       command-line options, "play again" loop
  game.c/.h    turn order, bonus turns, end of game
  board.c/.h   board state + rules (NO input/output) ← reused by the bot in Sprint 2
  display.c/.h everything printed to the screen (colours, board drawing)
  input.c/.h   safe line-based input parsing (fgets + strtol)
tests/
  test_units.c       177 checks on the rules and the parser
  run_scripted.sh    plays complete games from tests/scripts/*.in
  gdb_demo.gdb       GDB walkthrough used in the presentation
scripts/
  install_autostart.sh   start the game on boot (Ubuntu systemd; also supports Alpine)
docs/
  DEBUGGING.md     our GDB/Valgrind strategy, with real session logs
  VM_SETUP.md      Ubuntu VM setup, step by step, and problems we fixed
.github/workflows/ci.yml   build + tests + Valgrind on every push
```

Module dependencies (arrows = "includes"):

```
main ──► game ──► board
  │        ├────► display ──► board
  │        └────► input
  └────► display, input
```

`board` depends on nothing else, which is why it can be unit-tested alone and
reused unchanged by the Sprint 2 bot.

## Data structure

A board of R×C boxes has (R+1)×(C+1) dots. The `Board` struct (opaque: other
files can only use the functions in `board.h`) stores three flat, heap-allocated arrays:

| Array | Size (4×5 board) | Meaning |
|---|---|---|
| `h` | (R+1)·C = 25 | horizontal line from dot (r,c) to (r,c+1) drawn? |
| `v` | R·(C+1) = 24 | vertical line from dot (r,c) to (r+1,c) drawn? |
| `owner` | R·C = 20 | `PLAYER_NONE`, `PLAYER_A` or `PLAYER_B` |

Box (r,c) has sides `h[r][c]` (top), `h[r+1][c]` (bottom), `v[r][c]` (left), `v[r][c+1]` (right).
It also keeps the two scores and a `lines_drawn` counter.

## Complexity analysis

Let R×C be the board size (boxes) and E = R(C+1) + (R+1)C the number of lines
(E = 49 for 4×5).

| Operation | Our approach | Cost | Naive approach |
|---|---|---|---|
| Validate a move | range check + adjacency + one array lookup | **O(1)** | — |
| **Does a move complete a box?** | check only the ≤ 2 boxes touching the new line, 4 sides each (≤ 8 lookups) | **O(1)** | rescan all boxes: O(R·C) per move |
| Is the game over? | compare `lines_drawn` with E | **O(1)** | scan every line or box: O(E) |
| Draw the board | visit every dot, line and box once | O(R·C) | — |
| Whole game | E moves × O(1) rules | **O(R·C)** for the rules (O((R·C)²) including redrawing the screen every move) | O((R·C)²) for the rules alone |
| Memory | three arrays | O(R·C) | — |

The key observation: a new line can only affect the boxes on its two sides,
so there is no need to look at the rest of the board.

## Testing

* `tests/test_units.c`: board creation, every invalid-move type, single box,
  **one line completing two boxes**, a full 4×5 board (49 lines → 20 boxes), and the
  input parser (letters, missing numbers, extra numbers, numbers too big for an int).
* `tests/run_scripted.sh`: 4 complete random games (A wins, B wins, a 10–10 tie, and a
  game with invalid input mixed in) plus an aborted game. Each game is saved with its known final
  score, and the tests check that the game ends with that score.
* We checked that the tests can fail: breaking the bonus-turn rule makes 2 scripted
  games fail.

## Debugging

See [docs/DEBUGGING.md](docs/DEBUGGING.md): our step-by-step routine, three debugging
case studies (Valgrind out-of-bounds, Valgrind leak, GDB conditional breakpoint) with
the real tool output in `docs/logs/`, and a GDB demo script.

## Team

<!-- Names, IDs and who did what -->
