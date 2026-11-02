# AGENTS.md — working in this repository

You are in a fork of **Raven**, a C GUI library, with an application example
added on top of it. This file is the hand-off: what is here, how to build and
test it, and the traps that cost time the first time round.

**Read [`DESIGN.md`](DESIGN.md) before changing the example.** It explains how
the reusable instrument pane is put together.

## What is here

| Path | What it is | Whose it is |
|---|---|---|
| `src/`, `include/`, `bindings/`, `docs/`, `tests/` | the Raven library and its generated bindings | upstream — do not edit unless the task is about Raven itself |
| `Examples/InstrumentPanel` | the instrument pane as a reusable C++ module, plus Rust and Python implementations | this fork |

Two larger projects live in their own repositories:

* **Workbench** — a desktop shell on Raven (menus, toolbar, panes, JSONC
  settings, schema-driven Preferences, embedded guide):
  <https://github.com/skp64-pvt01/Raven-workbench>
* **Tutorials** — four graded courses (C++, Rust, Python, TypeScript), eight
  steps and a packaging step each:
  <https://github.com/skp64-pvt01/Raven-tutorials>

This repository is their dependency only; work on them there.

Generated bindings (`bindings/*/raven.*`, `bindings/rust/src/sys.rs`) say
"generated … do not edit": they come from `tools/bindgen`. Change the C headers
and regenerate, never the generated files.

## Prerequisites

```bash
# the shared library the bindings use
cmake -S . -B build -DRAVEN_BUILD_SHARED=ON
cmake --build build -j"$(nproc)"

# the C++ example uses find_package(raven)
cmake --install build --prefix /usr/local      # or install the raven-dev package
```

Toolchains: CMake ≥ 3.20, a C++17 compiler, `cargo`, `python3`.

## Build and test

```bash
# C++ implementation
cmake -S Examples/InstrumentPanel/cpp -B /tmp/panel
cmake --build /tmp/panel
/tmp/panel/instrument_panel --test

# Rust implementation (from Examples/InstrumentPanel/rust)
cargo run --release -- --test

# Python implementation (from Examples/InstrumentPanel/python)
PYTHONPATH=../../../bindings/python RAVEN_LIBRARY=../../../build/libraven.so \
    python3 panel.py --test
```

Every program takes `--test`: no window, no GPU, null backend, synthetic input.
That is the only test harness in the layer; there is no framework to install.

## Rules

1. **Every feature gets a check** in the `--test` path, asserting state (values,
   paint counts, parsed files), never pixels.
2. **Never write test output into the repository.** Use `/tmp`.
3. **Do not commit build outputs.** `build/`, `target/` and `__pycache__/` are
   ignored; keep it that way.
4. **Keep the library untouched.** If a change needs a new Raven API, say so and
   stop — that is an upstream change, not a layer change.
5. **Docs move with code.** A visible change means the example README updated.

## Pitfalls

| Symptom | Cause |
|---|---|
| A widget in a `VBox` has zero height | `Rslot_set_fill(child, 0)` in a box whose main axis is vertical is a *height* share, not "auto". Leave the fill unset and use `halign` for width. |
| `cannot start Raven (is a Vulkan driver installed?)` | A windowed run on a machine without a driver. Use `--test`. |

The bindings have their own traps (cargo static/shared, Python `PYTHONPATH`,
Deno `--allow-ffi`); they are documented in the Workbench and Tutorials
repositories, where those bindings are used in full.

## Where to change what

| Task | Files |
|---|---|
| Add an instrument | `Examples/InstrumentPanel/cpp/panel.h` / `panel.cpp` (paint callback, state, accessors) — and the other implementations |
| Change the Workbench | <https://github.com/skp64-pvt01/Raven-workbench> |
| Change a tutorial | <https://github.com/skp64-pvt01/Raven-tutorials> |

## Definition of done

* Clean builds, **zero warnings** in this layer.
* `--test` passes in the C++, Rust and Python implementations.
* The example README is updated with the change.
* Nothing generated, built or temporary is staged.
