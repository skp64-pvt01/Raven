# AGENTS.md — working in this repository

You are in a fork of **Raven**, a C GUI library, with an application-and-tutorial
layer added on top of it. This file is the hand-off: what is here, how to build
and test it, and the traps that cost time the first time round.

**Read [`DESIGN.md`](DESIGN.md) before changing the workbench.** It explains why
the modules are shaped the way they are.

## What is here

| Path | What it is | Whose it is |
|---|---|---|
| `src/`, `include/`, `bindings/`, `docs/`, `tests/` | the Raven library and its generated bindings | upstream — do not edit unless the task is about Raven itself |
| `Tutorials/C++`, `Tutorials/Rust`, `Tutorials/Python`, `Tutorials/TypeScript` | four graded courses, 8 learning steps + 1 packaging step each | this fork |
| `Examples/InstrumentPanel` | the instrument pane as a reusable C++ module, plus Rust and Python implementations | this fork |
| `Examples/Workbench` | the application shell: menu bar, toolbar, side bar, panes, config, schema, preferences, embedded guide, context menus. Self-contained apart from Raven: its own `AGENTS.md` / `DESIGN.md` and a vendored `cpp/panel/` copy, because it is meant to be forked | this fork |

Generated bindings (`bindings/*/raven.*`, `bindings/rust/src/sys.rs`) say
"generated … do not edit": they come from `tools/bindgen`. Change the C headers
and regenerate, never the generated files.

## Prerequisites

```bash
# the shared library the bindings and the guide use
cmake -S . -B build -DRAVEN_BUILD_SHARED=ON
cmake --build build -j"$(nproc)"

# the C++ examples use find_package(raven)
cmake --install build --prefix /usr/local      # or install the raven-dev package
```

Toolchains: CMake ≥ 3.20, a C++17 compiler, `cargo`, `python3`, `deno`.
The embedded user guide needs a Chromium-family browser (Chrome, Chromium,
Edge, Brave); without one the pane says so and everything else works.

## Build and test

```bash
# C++ course — 8 steps
cmake -S Tutorials/C++ -B /tmp/cpp && cmake --build /tmp/cpp && ctest --test-dir /tmp/cpp

# Rust course — 8 cargo packages (09 is a CMake packaging project), no env vars
cd Tutorials/Rust && cargo test --workspace

# Python course — needs both the module and the library
cd Tutorials/Python
PYTHONPATH=../../bindings/python RAVEN_LIBRARY=../../build/libraven.so python3 check_all.py

# TypeScript course — FFI, so --allow-ffi (the tasks pass it)
cd Tutorials/TypeScript
RAVEN_LIBRARY=../../build/libraven.so deno task check
RAVEN_LIBRARY=../../build/libraven.so deno task test

# Examples
cmake -S Examples/InstrumentPanel/cpp -B /tmp/panel && cmake --build /tmp/panel && /tmp/panel/instrument_panel --test
cmake -S Examples/Workbench/cpp -B /tmp/wb && cmake --build /tmp/wb && /tmp/wb/workbench --test
```

Every program takes `--test`: no window, no GPU, null backend, synthetic input.
That is the only test harness in the layer; there is no framework to install.

## Rules

1. **Actions are the interface.** Every menu entry, toolbar button and context
   menu entry names an action (`Rwidget_set_action` / `OnClicked`); the handler
   is bound once with `Raction_bind`. Never call a handler directly from a
   widget callback if a user could reach it another way.
2. **A setting lives in four places**: the field in `Config` (`config.h`), the
   `config_get`/`config_set` branch, `workbench.schema.json`, and
   `workbench.jsonc`. Miss one and the form, the file or the reader disagree.
3. **Every feature gets a check** in the `--test` path, asserting state (values,
   pane index, clipboard, parsed file), never pixels.
4. **Never write test output into the repository.** Use `/tmp`; the workbench
   test points `g.config.path` at a temporary file before saving.
5. **Do not commit build outputs.** `build/`, `target/`, `__pycache__/` are
   ignored; keep it that way.
6. **Keep the library untouched.** If a change needs a new Raven API, say so and
   stop — that is an upstream change, not a layer change.
7. **Docs move with code.** A UI change that a screenshot shows means a new
   screenshot; a new setting means README + `help/` page + schema.

## Pitfalls (each one cost a debugging session)

| Symptom | Cause |
|---|---|
| A widget in a `VBox` has zero height | `Rslot_set_fill(child, 0)` in a box whose main axis is vertical is a *height* share, not "auto". Leave the fill unset and use `halign` for width. |
| `cargo test` fails with hundreds of `rust-lld: undefined symbol` | The crate asks for a dylib, the linker silently picked the static `libraven.a`, and X11 was never linked. Build `build/libraven.so`, or `RAVEN_STATIC=1 RAVEN_LIB_DIR=/usr/lib`. |
| `cargo run` cannot load `libraven.so.0` | Cargo overwrites `LD_LIBRARY_PATH` for the programs it runs. The rpath in `.cargo/config.toml` is what makes it work. |
| Python: `No module named 'raven'` / library not found | Set both `PYTHONPATH=bindings/python` and `RAVEN_LIBRARY=build/libraven.so`. The PyPI wheel is Windows-only today. |
| Deno: `Requires ffi access` | `--allow-ffi` (the `deno.json` tasks pass it). Structs are passed as `Uint8Array`; callbacks must be kept alive. |
| Right-clicking a pane shows no menu | Panels are pointer-transparent by default (`NO_HIT`). Call `Rwidget_set_hit_testable(area, R_TRUE)` on the area that owns the menu; children still get clicks first. |
| A context menu does not close after a choice | Menus open on right-button **release** and walk **up** the parent chain; inject both press and release, and attach the menu to the pane, not to each child. |
| Saving settings deleted the comments | The writer patches value spans; if you rewrite the file wholesale you lose them. `config_save` re-parses its own result before writing. |
| A help capture is blank | Headless Chromium needs time; wait several seconds of frames. The tests point the browser at a missing program on purpose. |

## Where to change what

| Task | Files |
|---|---|
| Add a pane | `Examples/Workbench/cpp/main.cpp` (`Pane`, `set_pane`, switcher, side bar, View menu) |
| Add a setting | `config.h`, `config.cpp`, `../workbench.schema.json`, `../workbench.jsonc`, plus the apply code in `main.cpp` |
| Add a menu or context entry | `main.cpp` (`build_window`, `attach_context_menus`) and an action binding |
| Add a help page | `Examples/Workbench/help/`, link it from `index.html` |
| Add an instrument | `Examples/InstrumentPanel/cpp/panel.h` / `panel.cpp` (paint callback, state, accessors) |
| Add a tutorial step | one folder per course; update the course CMake/`check_all`/`deno.json` and the README table |
| Package | the `09-packaging` folder of each course (CPack) |

## Definition of done

- Clean builds, **zero warnings** in this layer.
- `--test` passes for every course step and both examples.
- The docs that describe the change are updated (README, `help/`, schema).
- A visual change comes with a fresh `--capture` screenshot.
- Nothing generated, built or temporary is staged.
