# Raven tutorials for C and C++

A graded course that takes you from an empty `main()` to a text editor with
menus, keyboard shortcuts, file dialogs and drag and drop — the shape most
desktop tools start from.

Each step is a folder with one `main.cpp` and a `README.md`. Every step builds,
runs, and checks itself without a screen, so you can follow the course on a
machine with no display, and see immediately when something is wrong.

---

## Before you start

| Need | Why |
|---|---|
| **Raven installed** | The course links against `raven::raven`. See *Installing Raven* below. |
| **CMake 3.20 or newer** | Every step is built by one CMake project. |
| **A C++17 compiler** | GCC, Clang or MSVC. Raven itself is C11; the course is C++. |
| **A Vulkan driver** | Only to open a real window. The `--test` runs need neither a driver nor a display. |

You do **not** need to know CMake, Vulkan, or any other GUI toolkit. You should
be comfortable with functions, pointers, and compiling a program from a
terminal.

### Installing Raven

Raven is a C library with a CMake package. Pick one:

```bash
# A. From a .deb built from source (see the repository README)
sudo apt install ./raven_0.2.0_amd64.deb ./raven-dev_0.2.0_amd64.deb

# B. From a source checkout
git clone https://github.com/oscar-soirey/Raven.git
cmake -S Raven -B Raven/build -DCMAKE_BUILD_TYPE=Release
cmake --build Raven/build -j"$(nproc)"
sudo cmake --install Raven/build --prefix /usr/local
```

Check that CMake can find it:

```bash
pkg-config --modversion raven     # 0.2.0
ls /usr/lib/cmake/raven           # ravenConfig.cmake
```

If Raven lives somewhere other than `/usr` or `/usr/local`, add
`-DCMAKE_PREFIX_PATH=/your/prefix` to the `cmake` command below.

---

## Build the whole course

```bash
cd "Tutorials/C++"
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

Each step becomes an executable in `build/`:

```
build/01-first-window
build/02-layout
...
build/08-capstone-notes
```

Build one step only:

```bash
cmake --build build --target 03-controls
```

Step 08 is also a standalone project: `08-capstone-notes/` has its own
`CMakeLists.txt`, so it can be copied out and built by itself.

Step 09 is a **separate CMake project** that builds step 08's source and packages
it; the course build does not include it. See
[09-packaging/README.md](09-packaging/README.md).

Run every self-check:

```bash
ctest --test-dir build --output-on-failure
```

Expected:

```
100% tests passed, 0 tests failed out of 8
```

---

## How the course is organised

Four levels. Each level depends on the one before it; you can skip a step but
not a level.

| Level | Step | You build | You learn |
|---|---|---|---|
| **1 — Beginner** | [01](01-first-window/README.md) | A window with a label and two buttons | `Rinit`, windows, widgets, callbacks, the frame loop |
| | [02](02-layout/README.md) | A page that arranges itself | Boxes, spacing, padding, `Slot.Fill`, alignment |
| **2 — Elementary** | [03](03-controls/README.md) | A form that updates a preview | Text input, checkbox, slider, combo box, reading state |
| | [04](04-actions-and-shortcuts/README.md) | A counter with keyboard shortcuts | Named actions, `Raction_fire`, shortcuts, enabling widgets |
| **3 — Intermediate** | [05](05-xml-interface/README.md) | The same interface, written in XML | XML files, `Rwidget_find`, actions from XML |
| | [06](06-menus/README.md) | A menu bar and a context menu | Menu bars, items, shortcuts, checkable entries |
| **4 — Advanced** | [07](07-text-editor/README.md) | A minimal editor that opens and saves | `TextEditor`, `Rfile_read`/`Rfile_write`, file dialogs |
| | [08](08-capstone-notes/README.md) | **A starter project: Notes** | Header bar, menus + editor + status bar, unsaved-changes guard, drag and drop |
| | [09](09-packaging/README.md) | **A DEB package and an archive of Notes** | `install()`, `GNUInstallDirs`, CPack, DEB metadata, `DESTDIR` staging |

Estimated time: half a day for levels 1–2, half a day for 3, an afternoon for
level 4 if you do the exercises.

---

## The two ways to run every step

Every program in this course understands one flag:

```bash
./build/03-controls            # opens a window (needs a display and a driver)
./build/03-controls --test     # no window, no GPU: drives the widgets and checks them
```

`--test` is not a toy. It starts Raven with the **null backend** and the null
platform, injects clicks and keys with `Rwindow_inject_*`, then asserts on what
the widgets hold — the same technique Raven's own test suite uses. It is how
you will check your work when a window is not available, and how a real project
keeps its interface logic tested.

Output of a good run:

```
06-menus: 10 checks, 0 failures
PASS
```

---

## Conventions used in the code

* **C API, C++ program.** Raven is a C library. The course writes `.cpp` files
  and calls the C functions directly: `Rwindow_create`, `Rlabel_create`. No
  wrapper to learn, and every snippet is valid C too. (Raven also ships
  `raven.hpp`, a C++ wrapper with lambdas and methods; see the note at the end
  of step 01.)
* **`Rid` is a handle.** Every window, widget and menu is a `Rid`, an opaque
  number checked on every call. `R_INVALID_ID` is the null handle.
* **The loop is yours.** Raven draws one frame per call; the program decides
  when to stop.
* **`nullptr` for unused callbacks.** Every callback of the library takes a
  `void* user_data` you can use instead of file-scope variables.

---

## Where to go after step 08

The course ends with a working skeleton, not a finished product. Two good
next reads:

* The Raven repository's `docs/guide.html` — the official manual, 28 chapters,
  from the same first program to custom widgets and shaders.
* `examples/notepad` in the repository — the same idea as step 08, with
  language detection, completion, markers and a language switcher.

Both are more detailed than this course; after step 08 you will recognise
almost everything in them.

---

## When something does not build

| Symptom | Cause and fix |
|---|---|
| `Could not find a package configuration file provided by "raven"` | Raven is not installed, or not in the prefix CMake searches. Add `-DCMAKE_PREFIX_PATH=/your/prefix`. |
| `cannot start Raven (is a Vulkan driver installed?)` | A real window was requested on a machine without a Vulkan driver. Use `--test`, or install the graphics driver. |
| `undefined reference to Rwindow_create` | `raven::raven` was not linked: the step's CMake entry is missing `target_link_libraries`. |
| A widget has a rect of zero | No frame has been drawn yet. Call `Rdraw_frame()` once before reading rectangles. |
| `cannot read window.xml` / `notes.xml` | The step was run from another directory and `RAVEN_TUTORIAL_DIR` was not set. Run the binary from `build/`, as the READMEs say. |
| `ctest` says "No tests were found" | `enable_testing()` is missing from `CMakeLists.txt`, or CMake was re-run after adding steps. Delete `build/` and configure again. |
