# Raven tutorials for Rust

A graded course that takes you from an empty `fn main()` to a text editor with
menus, keyboard shortcuts, file dialogs and drag and drop — the shape most
desktop tools start from.

Each step is a Cargo package with one `src/main.rs` and a `README.md`. Every
step builds, runs, and checks itself without a screen, so the course works on a
machine with no display and no GPU.

---

## Before you start

| Need | Why |
|---|---|
| **Rust 1.70 or newer** | `cargo` and `rustc`. |
| **Raven built as a shared library** | The binding links `libraven.so` / `raven.dll` / `libraven.dylib`. |
| **A Vulkan driver** | Only to open a real window. `--test` needs neither a driver nor a display. |

You do **not** need to know any other GUI toolkit, nor the Raven C API. You
should be comfortable with ownership, `Rc`/`RefCell`, and `unsafe` blocks — this
course explains the ones it uses.

### Building the Raven library

The course lives inside the Raven repository, so build its **shared** library
once, from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=ON
cmake --build build -j"$(nproc)"
```

That is all. `Tutorials/Rust/.cargo/config.toml` points the linker at `build/`
(`RAVEN_LIB_DIR`) and records the same place as an **rpath** in the binary, so
`cargo test` and `cargo run` need no environment variables. (An exported
`LD_LIBRARY_PATH` would not survive: Cargo overwrites it for the programs it
starts, which is exactly why the rpath is there.)

Using the `raven-dev` package instead, which ships the static `libraven.a`:

```bash
RAVEN_STATIC=1 RAVEN_LIB_DIR=/usr/lib cargo test --workspace
```

Another Raven build? Edit the two paths in `Tutorials/Rust/.cargo/config.toml`
(the link directory and the rpath value at the end of the file).

### The dependency, and where it points

Every step depends on the binding by **path**:

```toml
[dependencies]
raven-gui.workspace = true        # ../../bindings/rust, in the workspace root
```

`Cargo.toml` at the root of the course holds that path:

```toml
[workspace.dependencies]
raven-gui = { path = "../../bindings/rust" }
```

If your Raven checkout is somewhere else — or you copied a single step out of
the course — edit that one line in the step's `Cargo.toml`:

```toml
raven-gui = { path = "/home/you/code/Raven/bindings/rust" }
```

The crate is not on crates.io under this name (that name belongs to an
unrelated project), so the path is the way in.

---

## Build and test the whole course

```bash
cd "Tutorials/Rust"
cargo build --workspace
cargo test --workspace          # runs every step's --test, headless
```

Step 09 is not a cargo package: it is a **CMake project** that builds step 08
with cargo and packages the result with CPack. See
[09-packaging/README.md](09-packaging/README.md).

Expected at the end:

```
test result: ok. 1 passed; 0 failed; ...   # one per step
```

Run one step:

```bash
cargo run -p step-03-controls               # a window
cargo run -p step-03-controls -- --test     # no window, no GPU
```

---

## The course

| Level | Step | You build | You learn |
|---|---|---|---|
| **1 — Beginner** | [01](01-first-window/README.md) | A window with a label and two buttons | `Rinit_ex`, XML, `widget_find`, `action_bind`, the frame loop |
| | [02](02-layout/README.md) | A page that arranges itself | Boxes, `Slot.Fill`, padding, alignment, spacers |
| **2 — Elementary** | [03](03-controls/README.md) | A form that updates a preview | Text input, checkbox, slider, combo box, reading state |
| | [04](04-actions/README.md) | A counter with shortcuts | Actions, `Raction_fire`, `Rwindow_register_shortcut`, enabling |
| **3 — Intermediate** | [05](05-xml-files/README.md) | The same interface, in a file | `window.xml`, `CARGO_MANIFEST_DIR`, live reload |
| | [06](06-menus/README.md) | A menu bar with sub-menus | Menus in XML, shortcuts, checkable entries, built-in actions |
| **4 — Advanced** | [07](07-text-editor/README.md) | A minimal editor that opens and saves | `TextEditor`, `Rfile_read`/`Rfile_write`, dialogs, a C callback trampoline |
| | [08](08-capstone-notes/README.md) | **A starter project: Notes** | Header bar, menus, status bar, unsaved-changes guard, drag and drop |
| | [09](09-packaging/README.md) | **A DEB package and an archive of Notes** | CMake driving cargo, `install()`, CPack, DEB metadata, static linking |

Estimated time: half a day for levels 1–2, half a day for 3, an afternoon for
level 4 with the exercises.

---

## The two ways to run every step

```bash
cargo run -p step-05-xml-files            # opens a window
cargo run -p step-05-xml-files -- --test  # null backend and null platform
```

The `--test` run starts Raven with `R_BACKEND_NULL` and `headless`, injects
clicks and keys with `Rwindow_inject_*`, and checks what the widgets hold. It is
how a real Rust GUI keeps its interface logic tested, and how this course
verifies that every step still works.

Output of a good run:

```
06-menus: 7 checks, 0 failures
PASS
```

---

## Conventions used in the code

* **The C API, unchanged.** `raven::sys` is the whole C surface; the crate root
  re-exports it. Functions keep their C names and are `unsafe`.
* **`cstr` and `text`.** `cstr("Hi")` makes a `CString` for the library;
  `text(pointer)` turns a text of the library into a `String`. Keep the
  `CString` alive for the duration of the call —
  `Rlabel_set_text(label, cstr("Hi").as_ptr())` is one statement, so it is.
* **`Rid` is a handle**: an opaque `u32`, checked on every call.
  `R_INVALID_ID` is the null handle.
* **UI in XML, behaviour in Rust.** `OnClicked="name"` fires the action `name`;
  `action_bind("name", closure)` answers it. A widget the code must touch gets a
  `Name` and is found with `widget_find`.
* **State in `Rc<RefCell<_>>`.** Closures registered with `action_bind` are
  `FnMut` and outlive `main`'s locals, so the shared state is handed to them by
  clone.
* **The loop is yours**: `while unsafe { Rwait_frame(-1.0) } != 0 {}`.

---

## When something does not build

| Symptom | Cause and fix |
|---|---|
| `error: linking with cc failed` / `cannot find -lraven` | Raven's shared library is not built. From the repository root: `cmake -S . -B build -DRAVEN_BUILD_SHARED=ON && cmake --build build`. |
| `rust-lld: error: undefined symbol: XSetSelectionOwner` (and hundreds more) | The linker found an installed **static** `libraven.a` instead of the shared library, so the X11 libraries were never linked. Build `build/libraven.so`, or use `RAVEN_STATIC=1`. |
| `error while loading shared libraries: libraven.so.0` | The rpath is missing: the crate was built outside `.cargo/config.toml`, or `build/` moved after the build. Rebuild. |
| `the file ... was not found` for `window.xml` / `notes.xml` | The step reads it through `env!("CARGO_MANIFEST_DIR")`; run from the step's folder or through `cargo run -p ...`. |
| `cannot start Raven (is a Vulkan driver installed?)` | A real window was requested on a machine without a driver. Use `--test`. |
| A widget has a rect of zero | No frame has been drawn yet. Call `Rdraw_frame()` once before `harness::rect`. |
| `already borrowed: BorrowMutError` | A `RefCell` borrow was kept across a callback that borrows again. Copy the values you need out of the borrow first. |
