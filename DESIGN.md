# DESIGN.md — the application and tutorial layer

This document describes **what was built on top of Raven**: four tutorial
courses, a reusable instrument panel, and the Workbench application shell. It is
written for the next developer (or agent) who has to extend it.

> **Scope.** The Raven library itself (`src/`, `include/`, `bindings/`, `docs/`)
> is upstream and is not described here; see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
> and [`README.md`](README.md). This layer adds files to the repository and does
> not modify the library.

---

## 1. Goals and non-goals

**Goals**

* teach Raven end to end, in four languages, in steps that always run;
* a realistic desktop shell — menu bar, toolbar, side bar, panes, status bar;
* configuration that a user can edit by hand *and* through a form;
* documentation inside the application;
* no dependency beyond what each language already needs;
* every feature checked without a display or a GPU.

**Non-goals**

* a general-purpose application framework (it is one application, copied);
* replacing the library documentation;
* multi-window, multi-user, network or persistence beyond the settings file;
* translations, accessibility work beyond what Raven gives;
* packaging the Workbench itself (the courses show how to package).

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
  Workbench/        cpp/{main,config,json,schema,preferences}.{h,cpp}
                    cpp/panel/ (vendored copy), workbench.jsonc,
                    workbench.schema.json, help/, its own AGENTS.md + DESIGN.md
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

## 4. The Workbench

The Workbench folder is **self-contained apart from Raven** and is meant to be
forked: it carries its own hand-off (`Examples/Workbench/AGENTS.md`), its own
copy of this design (`Examples/Workbench/DESIGN.md`), and a vendored copy of the
instrument pane in `cpp/panel/`. The two DESIGN files share this architecture;
when one changes, change the other. The panel exists twice on purpose (the
standalone InstrumentPanel example and the Workbench template) — keep the copies
in sync, or treat the Workbench's as canonical once it is its own project.

### 4.1 Architecture

```
                    ┌──────────────────────── main.cpp ───────────────────────┐
                    │ window · menu bar · toolbar · side bar · status bar     │
                    │ actions (view.*, plant.*, edit.*, help.*, clipboard.*)  │
                    │ frame callback · context menus · test                  │
                    └───────┬───────────────┬────────────────┬────────────────┘
                            │               │                │
              ┌─────────────▼──┐   ┌────────▼───────┐  ┌─────▼──────────┐
              │ preferences    │   │ schema         │  │ panel (module) │
              │ (form, search) │◄──┤ (schema.cpp)   │  │ panel.h/.cpp   │
              └───────┬────────┘   └────────▲───────┘  └─────┬──────────┘
                      │                     │                │
              ┌───────▼────────┐   ┌────────┴───────┐  ┌─────▼──────────┐
              │ config         │   │ workbench      │  │ Instr. panel   │
              │ (get/set/save) │◄──┤ .schema.json   │  │ example shares │
              └───────┬────────┘   └────────────────┘  └────────────────┘
                      │
              ┌───────▼────────┐   ┌────────────────┐
              │ json (JSONC)   │   │ help/ (WebView)│
              └────────────────┘   └────────────────┘
```

| Module | Responsibility |
|---|---|
| `main.cpp` | the shell: panes, menus, toolbar, side bar, status bar, actions, frame, context menus, tests |
| `panel.h/.cpp` | the instrument pane as an opaque reusable module (gauges, lamps, chart, plant) |
| `config.h/.cpp` | the `Config` struct, JSONC reading, per-key get/set, comment-preserving save |
| `json.h/.cpp` | a JSONC reader with value spans (used by config and schema) |
| `schema.h/.cpp` | reads `workbench.schema.json` into groups of settings |
| `preferences.h/.cpp` | the form: search, rows, controls, save/reset |

### 4.2 Data flow

1. **Startup.** `config_load(path)` fills `Config`; the schema is loaded once;
   the window is built with the theme, toolbar and first pane from `Config`.
2. **Actions.** Every menu item, toolbar button and context menu entry carries
   an action name; `Raction_bind` maps each name to one function. Checkable menu
   items toggle themselves first and the handler reads `Rmenu_item_is_checked`.
3. **Panes.** A `Switcher` holds four pages: Application, Instrument panel, User
   guide, Preferences. `set_pane()` changes the page and synchronises the menu
   checks, the side bar highlight and the status bar.
4. **Settings.** The Preferences pane builds itself from the schema, edits
   `Config` through `config_get`/`config_set`, applies live settings through a
   callback, and writes the file only on Save.
5. **Guide.** A `raven_web` `WebView` is created on the first Help open and
   loads local HTML with the theme in the URL; the browser is headless and
   optional.
6. **Frame.** Only the instrument panel costs frames: it simulates, refreshes
   its labels and requests a redraw. The other panes are static.

### 4.3 Decisions

**D1 — Panes are hidden, not rebuilt.** A `Switcher` keeps every page alive.
*Why:* the panel keeps its trend, switches and animation state; reopening is
instant. *Cost:* memory for all pages; the browser starts lazily to avoid the
worst of it.

**D2 — One action vocabulary for menu, toolbar and context menu.** *Why:* three
entry points that cannot disagree; a context menu is five lines once the action
exists. *Cost:* the action names are strings — a typo fails silently, so tests
click the real widgets.

**D3 — The instrument panel is a module, not a screen.** `panel.h` exposes
`panel_create/build/simulate/reset` and accessors; the standalone example and
the Workbench compile the same `panel.cpp`. *Why:* the pane is the point of the
example; duplicating it would rot. *Cost:* an opaque API means new controls need
an accessor.

**D4 — The Preferences pane is generated from a JSON Schema.** *Why:* the form,
the documentation and the file cannot drift; a new key appears without widget
code. The editor reads a documented subset (`properties`, `title`,
`description`, `type`, `enum`, `minimum`, `maximum`, `default`); the rest of the
document stays valid for external editors. *Cost:* mapping from schema path to a
`Config` field still needs `config_get`/`config_set`, and an unknown key shows
"not used by this build" rather than inventing a control.

**D5 — Saving preserves the file as written.** The reader records each value's
byte span; the writer replaces only those spans, inserts missing keys before the
object's brace, and re-parses the result before writing. *Why:* a configuration
file is the user's text — comments explain values. *Cost:* inserted keys use the
canonical indentation, and a file with a syntax error is refused rather than
overwritten.

**D6 — Documentation is a real web view.** `raven_web` drives an installed
headless Chromium and hands Raven a texture, so the guide is real HTML with real
links inside the pane. *Why:* writing a Markdown renderer would be more code for
less fidelity. *Cost:* a browser must exist (the pane says so), pages arrive a
frame late, and captures need a few seconds.

**D7 — Context menus belong to panes.** A menu is attached to the pane root;
Raven walks up from the widget under the pointer, so a label, a tile or the
chart all reach it. Panels are `NO_HIT` by default, so menu-bearing areas call
`Rwidget_set_hit_testable`. *Why:* one menu instead of one per child. *Cost:*
the area becomes a hit target for the pointer (children still win).

**D8 — Live where possible, next start otherwise.** Theme, alarm threshold,
trend window and switches apply at once; toolbar and first pane are read while
building the window, so the pane says "next start". *Why:* rebuilding the window
to toggle a toolbar would lose state.

### 4.4 Extension recipes

* **New pane** — add a value to `Pane`, build its page, add it to the switcher,
  `set_pane` (switcher + checks + side bar + status), a side bar button, a View
  menu action, a context menu entry, and a test that opens it.
* **New setting** — `Config` field, `config_get`/`config_set`, the schema entry
  (title, description, type, enum/range, default), the shipped `workbench.jsonc`,
  the live apply in `main.cpp` if it can apply at once, and a test.
* **New menu or context entry** — name an action, bind it, add the entry
  (`Rmenu_add_item` + `Rwidget_set_action`). Reuse an existing action where one
  fits.
* **New help page** — add an HTML file under `help/`, link it from `index.html`,
  optionally a Help menu action; the WebView loads whatever the URL names.
* **New instrument** — state in `Panel`, a paint callback, an accessor for the
  shell, a `panel_set_*` if it is configurable, and `examples/InstrumentPanel`
  reuses it for free.
* **New tutorial step** — a folder per course, wire it into the course build
  (`CMakeLists.txt`, workspace members, `check_all.py`, `deno.json`), write the
  README, make `--test` meaningful, update the course index.

### 4.5 Testing

* **Headless only.** `--test` starts the null backend, injects clicks, keys and
  text, and asserts values: pane index, labels, parsed config, clipboard,
  schema counts.
* **The browser is excluded from tests.** The test points
  `Rweb_set_browser_path` at a missing program: the pane and its actions are
  tested, Chromium is not.
* **Writes go to `/tmp`.** The save/config tests never touch the shipped
  `workbench.jsonc`.
* **Visual checks are captures, not assertions.** `--capture` writes a PPM; the
  screenshots in the READMEs come from it. A visual change is reviewed by looking
  at a capture, not by pixel tests.

### 4.6 Limits and future work

* Toolbar visibility and the first pane apply at the next start.
* The schema drives the form, but validation beyond type/ranges/enums belongs to
  an external validator; `additionalProperties` is not enforced at load.
* One window; docking (`DockArea`) would replace the side bar + switcher.
* No settings for lists/arrays; the schema subset has no `$ref`, `oneOf`, etc.
* The guide is local HTML; search across pages, or a live manual, is not built.
* The Workbench is not packaged; the courses show the CPack pattern.

### 4.7 File map

| File | Lines (approx.) | Contents |
|---|---|---|
| `main.cpp` | 1180 | shell, actions, panes, menus, context menus, frame, tests |
| `config.cpp` | 370 | load, get/set, comment-preserving save |
| `json.cpp` | 270 | JSONC reader with spans |
| `schema.cpp` | 110 | schema reader |
| `preferences.cpp` | 365 | form, controls, search, save/reset |
| `panel.cpp` | 560 | instruments, plant, paint callbacks |
| `workbench.schema.json` | 100 | the settings schema |
| `workbench.jsonc` | 30 | the shipped configuration |
| `help/*.html` | 240 + CSS | the user guide |

The library boundary: this layer calls only the public API in `include/raven/*.h`
and `include/raven_web.h`. If an extension needs something else, add it upstream
first.
