# DESIGN.md — the application and tutorial layer

This document describes **what was built on top of Raven**: four tutorial
courses and a reusable instrument panel. It is written for the next developer
(or agent) who has to extend them.

The Workbench application shell that started here now lives in its own
repository, <https://github.com/skp64-pvt01/Raven-workbench>; its `DESIGN.md` is
the document for it.

> **Scope.** The Raven library itself (`src/`, `include/`, `bindings/`, `docs/`)
> is upstream and is not described here; see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
> and [`README.md`](README.md). This layer adds files to the repository and does
> not modify the library.

---

## 1. Goals and non-goals

**Goals**

* teach Raven end to end, in four languages, in steps that always run;
* one reusable instrument pane, implementation-complete in C++, Rust, Python;
* no dependency beyond what each language already needs;
* every feature checked without a display or a GPU.

**Non-goals**

* a general-purpose application framework — the Workbench repository is the
  template for that;
* replacing the library documentation;
* multi-window, multi-user, network or persistence;
* translations, accessibility work beyond what Raven gives;
* packaging beyond what each course's `09-packaging` step teaches.

---

## 2. Layout

```
Tutorials/
  C++/           9 steps, common/tutorial.h, CMake
  Rust/          8 packages + a CMake packaging step, .cargo/config.toml
  Python/        9 steps, tutorial.py, check_all.py
  TypeScript/    9 steps, raven.ts (FFI binding), tutorial.ts, deno.json
Examples/
  InstrumentPanel/  cpp/{panel.h,panel.cpp,main.cpp}, rust/, python/
```

`Tutorials/` and `Examples/` are additions in this fork; new top-level paths do
not conflict with an upstream merge.

---

## 3. The tutorial suite

### 3.1 Shape

Four courses, each with **eight learning steps and one packaging step**, graded
by level:

| Level | Steps | Theme |
|---|---|---|
| 1 — Beginner | 01, 02 | window, widgets, callbacks, layout |
| 2 — Elementary | 03, 04 | controls and state, actions and shortcuts |
| 3 — Intermediate | 05, 06 | XML interfaces, menus |
| 4 — Advanced | 07, 08, 09 | text editor and files, capstone editor, packaging |

All four courses build the **same application** — a small text editor with
menus, shortcuts, file dialogs and an unsaved-changes guard — so a reader can
compare languages side by side.

| Course | Steps | Checks | Binding |
|---|---|---|---|
| C++ | 8 + packaging | 67 | the C API directly |
| Rust | 8 + packaging | 56 | `raven-gui` crate (path dependency) |
| Python | 8 + packaging | 55 | `bindings/python/raven.py` (ctypes) |
| TypeScript | 8 + packaging | 53 | `raven.ts`, written for this course (Deno FFI) |

### 3.2 Anatomy of a step

* one executable (`main.cpp` / `main.rs` / `main.py` / `main.ts`) and a README;
* the program runs windowed, or with `--test` on the null backend;
* `--test` drives the real widgets with synthetic input and asserts state;
* the README has: goal, what you learn, walkthrough, build, run, test,
  checkpoint, exercises, troubleshooting.

The shared harness differs per language because each has its own idiom:

| Language | Harness | Test entry |
|---|---|---|
| C++ | `common/tutorial.h` (inline helpers) | CTest calls each binary with `--test` |
| Rust | `src/harness.rs` copied per package | `tests/check.rs` spawns the binary; `cargo test` |
| Python | `tutorial.py`, imported by path insert | `check_all.py` runs each script |
| TypeScript | `tutorial.ts` | `check_all.ts`, `deno task test` |

### 3.3 Decisions

* **XML-first for the bindings.** Rust and Python bindings are action-centric
  (closures/functions per action name); building widgets in code would mean
  passing raw C callbacks everywhere. Rust, Python and TypeScript therefore
  describe the interface in XML from step 01 and bind actions; the C++ course
  starts with the widget tree in code (it is the API being taught) and moves to
  XML at step 05.
* **`--test` on the null backend.** A GUI is untestable until it can run
  headless; Raven's null backend plus `Rwindow_inject_*` makes every step a
  normal unit-testable program. The alternative — a mock layer — would test
  nothing real.
* **Packaging is a separate step, not a build feature.** Each `09-packaging`
  project builds the step-08 sources and installs/packs them (CPack: DEB + TGZ),
  which teaches that packaging is a build concern and keeps the application
  sources untouched.
* **Bindings from the checkout, not a registry.** The Rust crate is a path
  dependency (the crates.io name belongs to another project); Python on Linux
  uses `bindings/python` (the wheel is Windows-only); Deno uses the hand-written
  FFI binding. Each README states the one line to change.

---

## 4. The Workbench (separate repository)

The application shell that was developed here — menu bar, toolbar, side bar,
panes, JSONC settings with a JSON Schema, a generated Preferences form, an
embedded user guide, context menus — now lives in its own repository:

> <https://github.com/skp64-pvt01/Raven-workbench>

It is a desktop shell on Raven and doubles as a template for new projects. Raven
is its only dependency: it is used from an installed package, or from a checkout
with `-DRAVEN_SOURCE_DIR=/path/to/Raven` (nothing is vendored). Its architecture,
decisions, extension recipes and limits are documented there, in its own
`DESIGN.md`.

The instrument pane is shared: `Examples/InstrumentPanel/cpp/panel.{h,cpp}` here
and `cpp/panel/panel.{h,cpp}` in the Workbench are copies of the same module.
Change one, change the other.

---

The library boundary: this layer calls only the public API in `include/raven/*.h`
and `include/raven_web.h`. If an extension needs something else, add it upstream
first.
