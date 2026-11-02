# AGENTS.md — working in this repository

You are in a fork of **Raven**, a C GUI library, with an application-and-tutorial
layer added on top of it. This file is the hand-off: what is here, how to build
and test it, and the traps that cost time the first time round.

**Read [`DESIGN.md`](DESIGN.md) before changing the layer.** It explains why the
courses and the example are shaped the way they are.

## What is here

| Path | What it is | Whose it is |
|---|---|---|
| `src/`, `include/`, `bindings/`, `docs/`, `tests/` | the Raven library and its generated bindings | upstream — do not edit unless the task is about Raven itself |
| `Tutorials/C++`, `Tutorials/Rust`, `Tutorials/Python`, `Tutorials/TypeScript` | four graded courses, 8 learning steps + 1 packaging step each | this fork |
| `Examples/InstrumentPanel` | the instrument pane as a reusable C++ module, plus Rust and Python implementations | this fork |

The application shell that used to live in `Examples/Workbench` is now its own
repository: **<https://github.com/skp64-pvt01/Raven-workbench>**. It is a desktop
shell on Raven (menu bar, toolbar, side bar, panes, JSONC settings with a JSON
Schema, embedded guide) and doubles as a template for new projects. Work on it
there; its `AGENTS.md` and `DESIGN.md` are the hand-off. This repository is its
only dependency.

Generated bindings (`bindings/*/raven.*`, `bindings/rust/src/sys.rs`) say
"generated … do not edit": they come from `tools/bindgen`. Change the C headers
and regenerate, never the generated files.

## Prerequisites

```bash
# the shared library the bindings and the guide use
cmake -S . -B build -DRAVEN_BUILD_SHARED=ON
cmake --build build -j"$(nproc)"

# the C++ example uses find_package(raven)
cmake --install build --prefix /usr/local      # or install the raven-dev package
```

Toolchains: CMake ≥ 3.20, a C++17 compiler, `cargo`, `python3`, `deno`.
The Workbench's embedded user guide needs a Chromium-family browser; it is not
built here.

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

# Example
cmake -S Examples/InstrumentPanel/cpp -B /tmp/panel && cmake --build /tmp/panel && /tmp/panel/instrument_panel --test
```

Every program takes `--test`: no window, no GPU, null backend, synthetic input.
That is the only test harness in the layer; there is no framework to install.

## Rules

1. **Every feature gets a check** in the `--test` path, asserting state (values,
   pane index, parsed file), never pixels.
2. **Never write test output into the repository.** Use `/tmp`.
3. **Do not commit build outputs.** `build/`, `target/`, `__pycache__/` are
   ignored; keep it that way.
4. **Keep the library untouched.** If a change needs a new Raven API, say so and
   stop — that is an upstream change, not a layer change.
5. **Docs move with code.** A new step means its README; a visible UI change
   means a fresh `--capture` screenshot in that README.

## Pitfalls (each one cost a debugging session)

| Symptom | Cause |
|---|---|
| A widget in a `VBox` has zero height | `Rslot_set_fill(child, 0)` in a box whose main axis is vertical is a *height* share, not "auto". Leave the fill unset and use `halign` for width. |
| `cargo test` fails with hundreds of `rust-lld: undefined symbol` | The crate asks for a dylib, the linker silently picked the static `libraven.a`, and X11 was never linked. Build `build/libraven.so`, or `RAVEN_STATIC=1 RAVEN_LIB_DIR=/usr/lib`. |
| `cargo run` cannot load `libraven.so.0` | Cargo overwrites `LD_LIBRARY_PATH` for the programs it runs. The rpath in `.cargo/config.toml` is what makes it work. |
| Python: `No module named 'raven'` / library not found | Set both `PYTHONPATH=bindings/python` and `RAVEN_LIBRARY=build/libraven.so`. The PyPI wheel is Windows-only today. |
| Deno: `Requires ffi access` | `--allow-ffi` (the `deno.json` tasks pass it). Structs are passed as `Uint8Array`; callbacks must be kept alive. |

## Where to change what

| Task | Files |
|---|---|
| Add an instrument | `Examples/InstrumentPanel/cpp/panel.h` / `panel.cpp` (paint callback, state, accessors) — and the Workbench's `cpp/panel/` copy |
| Add a tutorial step | one folder per course; update the course CMake/`check_all`/`deno.json` and the README table |
| Package | the `09-packaging` folder of each course (CPack) |
| Change the Workbench | <https://github.com/skp64-pvt01/Raven-workbench> |

## Definition of done

- Clean builds, **zero warnings** in this layer.
- `--test` passes for every course step and the example.
- The docs that describe the change are updated (README, step README).
- A visual change comes with a fresh `--capture` screenshot.
- Nothing generated, built or temporary is staged.
