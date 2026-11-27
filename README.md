# Dots and Boxes — CMPS 241 Course Project

A console version of Dots and Boxes in C, built to run on a text-only Linux VM
(we use Ubuntu in VirtualBox) and start automatically on boot.

| Sprint | Content | Status |
|---|---|---|
| 1 | Two players (A vs B), 4×5 board, Makefile, tests, GDB/Valgrind, VM auto-start | ✅ |
| 2 | Hard-level bot (minimax + alpha-beta pruning) + complexity analysis | ✅ |
| 3 (bonus) | Network play + multithreading | ⏳ |

## Build and run

```sh
make                 # builds ./dotsandboxes (warnings are errors)
./dotsandboxes       # play: choose 1 (two players) or 2 (against the bot)
./dotsandboxes --plain               # compact spec layout, no colours / clearing
./dotsandboxes --rows 2 --cols 3     # other board sizes (1..9)
make test            # unit tests, bot tests + full scripted games
make valgrind        # unit tests, bot tests and every scripted game under Valgrind
make arena           # measure the bot: wins, time and search depth per move
make gdb             # start the game in GDB
```

**How to play:** type the row and column of two neighbouring dots, e.g.
`0 0 0 1` draws the line from dot (0,0) to dot (0,1). Dots can be given in either
order. Completing a box claims it, and you play again. Most boxes wins.

**Against the bot:** choose `2` at the menu. You are Player A and move first; the
bot is Player B. After each of its moves the board is redrawn and the bot's move is
printed (e.g. `The bot played 1 0 1 1`). How the bot works: [docs/BOT.md](docs/BOT.md).

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
  main.c       command-line options, game mode menu, "play again" loop
  game.c/.h    turn order, bonus turns, bot turns, end of game
  board.c/.h   board state + rules (NO input/output), copy + undo for the bot
  bot.c/.h     the hard bot: minimax with alpha-beta pruning
  display.c/.h everything printed to the screen (colours, board drawing)
  input.c/.h   safe line-based input parsing (fgets + strtol)
tests/
  test_units.c       242 checks on the rules, copy/undo and the parser
  test_bot.c         bot tests, incl. comparing it with plain minimax
  bot_arena.c        bot vs random and greedy players (make arena)
  run_scripted.sh    plays complete games from tests/scripts/*.in
  gdb_demo.gdb       GDB walkthrough used in the presentation
  gdb_bot.gdb        GDB: watch the bot's search depth by depth
scripts/
  install_autostart.sh   start the game on boot (Ubuntu systemd; also supports Alpine)
docs/
  BOT.md           the bot's strategy and its complexity
  DEBUGGING.md     our GDB/Valgrind strategy, with real session logs
  VM_SETUP.md      Ubuntu VM setup, step by step, and problems we fixed
.github/workflows/ci.yml   build + tests + Valgrind on every push
```

Module dependencies (arrows = "includes"):

```
main ──► game ──► board
  │        ├────► bot ──► board
  │        ├────► display ──► board
  │        └────► input
  └────► display, input
```

`board` depends on nothing else, which is why it can be unit-tested alone. The
bot only uses `board` too: it never prints or reads anything, so it can be tested
without the rest of the game.

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

The bot's strategy and its complexity (minimax O(b^d), alpha-beta about
O(b^(d/2)), our budget O(N·E) per move, with measurements) are in
[docs/BOT.md](docs/BOT.md).

## Testing

* `tests/test_units.c`: board creation, every invalid-move type, single box,
  **one line completing two boxes**, a full 4×5 board (49 lines → 20 boxes), and the
  input parser (letters, missing numbers, extra numbers, numbers too big for an int),
  and copying a board / undoing lines (undoing all 49 lines gives back an empty board).
* `tests/test_bot.c`: the bot takes free boxes, gives the same answers as a plain
  minimax with no pruning, only plays free lines and never changes the real board.
* `tests/run_scripted.sh`: 4 complete random games (A wins, B wins, a 10–10 tie, and a
  game with invalid input mixed in), an aborted game, a full game against the bot on 2×2,
  and junk typed at the menu. Each game is saved with its known final
  score, and the tests check that the game ends with that score.
* We checked that the tests can fail: breaking the bonus-turn rule makes 2 scripted
  games fail.

## Debugging

See [docs/DEBUGGING.md](docs/DEBUGGING.md): our step-by-step routine, three debugging
case studies (Valgrind out-of-bounds, Valgrind leak, GDB conditional breakpoint) with
the real tool output in `docs/logs/`, and a GDB demo script.

## Team

<!-- Names, IDs and who did what -->
- Malek Sakr (GitHub: Malek177)
- Jason Matta (Github: jasonlmatta)
