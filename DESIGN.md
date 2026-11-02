# DESIGN.md — the application layer

This document describes **what was built on top of Raven** in this repository:
the reusable instrument panel example. It is written for the next developer (or
agent) who has to extend it.

Two larger projects started here and now live in their own repositories:

* **Workbench** — a desktop application shell (menus, toolbar, panes, JSONC
  settings with a JSON Schema, a generated Preferences form, an embedded guide):
  <https://github.com/skp64-pvt01/Raven-workbench>
* **Tutorials** — four graded courses (C++, Rust, Python, TypeScript), eight
  learning steps plus a packaging step each:
  <https://github.com/skp64-pvt01/Raven-tutorials>

Their `DESIGN.md` documents them. This repository is their dependency only.

> **Scope.** The Raven library itself (`src/`, `include/`, `bindings/`, `docs/`)
> is upstream and is not described here; see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
> and [`README.md`](README.md). This layer adds files to the repository and does
> not modify the library.

---

## 1. Goals and non-goals

**Goals**

* one reusable instrument pane, implementation-complete in C++, Rust and Python;
* a small, realistic example: gauges, lamps, a chart and a simulated plant;
* every feature checked without a display or a GPU;
* code that can be copied into an application (the Workbench uses the same pane).

**Non-goals**

* a general-purpose framework — the Workbench is the template for that;
* replacing the library documentation;
* multi-window, networking or persistence;
* translations, packaging or a build system shared across the three languages.

---

## 2. Layout

```
Examples/
  InstrumentPanel/
    cpp/      panel.h, panel.cpp, main.cpp, CMakeLists.txt
    rust/     src/main.rs, Cargo.toml, .cargo/config.toml
    python/   panel.py
    README.md
```

`Examples/` is an addition in this fork; a new top-level path does not conflict
with an upstream merge. The `Workbench` copy of the panel lives at
`cpp/panel/panel.{h,cpp}` in the Workbench repository; keep the two in step.

---

## 3. The instrument panel

The pane is one module with three implementations, all with the same behaviour:

| Module | C++ | Rust | Python |
|---|---|---|---|
| the pane | `panel.h` / `panel.cpp` | `src/main.rs` | `panel.py` |
| entry point | `main.cpp` | `src/main.rs` | `panel.py` |

The public surface is the same: create/build the widget tree, simulate the
plant, refresh the labels and lamps, reset, pause, read pressure/flow/samples,
and set the alarm threshold, trend window and switches. The Workbench adds
setters for the configuration file.

Each implementation has four paint callbacks (analog gauge, digital meter, lamp,
strip chart) that draw with Raven's `Rdraw_*` and read colours with
`Rwidget_get_color`, so they follow the theme. The plant is a tiny simulated
process: a pump, a valve and an interlock; the trend stores 10 seconds at 30 Hz.

**Why three implementations.** The example is the shortest complete Raven
program per language, and the three can be compared line by line. The C++
version is the reference; the Rust and Python versions follow it.

**Why a module, not a screen.** `panel.h` is an opaque API and the drawing lives
in `panel.cpp`; the same file can be reused (as the Workbench does) or deleted
as a unit. The cost is that a new control needs an accessor.

---

## 4. Testing

Every implementation has a `--test` mode: Raven starts on the null backend, the
real widgets are driven with `Rwindow_inject_*`, and the checks assert state
(pressure, flow, samples, lamp, alarm, paint counts), never pixels. The browser,
the GPU and the display are not involved. A real window is only for looking at
the result; `--capture` writes a PPM frame.

---

## 5. Limits and future work

* The pane is a demo of drawing and simulation, not a process-control library.
* The three implementations are ports; a change belongs in all of them.
* The Workbench and Tutorials repositories carry the settings, schema and
  course material; this repository does not.

---

The library boundary: this layer calls only the public API in `include/raven/*.h`
and `include/raven_web.h`. If an extension needs something else, add it upstream
first.
