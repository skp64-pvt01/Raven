# Raven tutorials for TypeScript

A graded course that takes you from an empty `main.ts` to a text editor with
menus, keyboard shortcuts, file dialogs and an unsaved-changes guard — the shape
most desktop tools start from.

Each step is a folder with one `main.ts` and a `README.md`. Every step runs and
checks itself without a screen, so the course works on a machine with no display
and no GPU.

---

## Before you start

| Need | Why |
|---|---|
| **Deno 2** | Runs TypeScript directly, with the FFI this course uses. No `npm install`. |
| **The Raven shared library** | `libraven.so` / `raven.dll` / `libraven.dylib`: the binding calls it through FFI. |
| **A Vulkan driver** | Only to open a real window. `--test` needs neither a driver nor a display. |

You do **not** need `npm`, a bundler, CMake (except for step 09), or any other
GUI toolkit.

### Getting Raven

```bash
git clone https://github.com/oscar-soirey/Raven.git
cmake -S Raven -B Raven/build -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=ON
cmake --build Raven/build -j"$(nproc)"
export RAVEN_LIBRARY="$PWD/Raven/build/libraven.so"
```

The binding looks for the library in `RAVEN_LIBRARY`, then beside an installed
program (`/usr/bin/notes` → `/usr/lib/libraven.so`), then in the system search
path.

Check everything:

```bash
cd "Tutorials/TypeScript"
deno task 01 --test
```

---

## Why this course ships a binding

Raven has bindings for C++, Rust, Python, Go, C#, Java, Zig, LuaJIT and Odin —
but none for JavaScript or TypeScript. [`raven.ts`](raven.ts) fills that gap for
this course with Deno's FFI:

```ts
const lib = Deno.dlopen(libraryPath(), { /* the C functions */ });
```

It is deliberately small: the functions the steps use, under names close to the
C API. Three details are worth knowing, because they are what any FFI needs:

* **structures are bytes.** `Rinit_ex` takes an `RInitDesc *`, so the binding
  builds a 40-byte buffer and writes the fields at their C offsets.
* **texts are NUL-terminated.** A `string` is encoded to UTF-8 with a `\0`;
  a `const char *` coming back is read with `Deno.UnsafePointerView`.
* **callbacks must outlive the call.** `actionBind` keeps its
  `Deno.UnsafeCallback` in a module-level array. Drop it and the C side calls a
  freed pointer.

Read `raven.ts` once, at the level of the comments: it is the "how" behind every
step.

---

## Run and test the whole course

```bash
cd "Tutorials/TypeScript"

deno task 01                      # a window
deno task 01 --test               # no window, no GPU
deno task test                    # every step, in order
deno task check                   # type-check every file
```

Expected at the end:

```
All steps passed.
```

Deno needs permissions to use FFI and to read the XML files; the tasks in
`deno.json` pass them:

```
--allow-ffi --allow-env --allow-read --allow-write
```

---

## The course

| Level | Step | You build | You learn |
|---|---|---|---|
| **1 — Beginner** | [01](01-first-window/README.md) | A window with a label and two buttons | `init`, XML, `find`, `actionBind`, the frame loop |
| | [02](02-layout/README.md) | A page that arranges itself | Boxes, `Slot.Fill`, padding, alignment, spacers |
| **2 — Elementary** | [03](03-controls/README.md) | A form that updates a preview | Text input, checkbox, slider, combo box, reading state |
| | [04](04-actions/README.md) | A counter with shortcuts | Actions, `actionFire`, `registerShortcut`, enabling |
| **3 — Intermediate** | [05](05-xml-files/README.md) | The same interface, in a file | `window.xml`, `import.meta.url`, live reload |
| | [06](06-menus/README.md) | A menu bar with sub-menus | Menus in XML, shortcuts, checkable entries, built-ins |
| **4 — Advanced** | [07](07-text-editor/README.md) | A minimal editor that opens and saves | `TextEditor`, Deno file I/O, dialogs |
| | [08](08-capstone-notes/README.md) | **A starter project: Notes** | Header bar, menus, status bar, unsaved-changes guard |
| | [09](09-packaging/README.md) | **A DEB package of Notes** | `install()`, CPack, shipping the FFI library, a launcher |

Estimated time: half a day for levels 1–2, half a day for 3, an afternoon for
level 4 with the exercises.

---

## The two ways to run every step

```bash
deno task 05              # opens a window
deno task 05 --test       # null backend and null platform
```

The `--test` run starts Raven with `R_BACKEND_NULL` and `headless`, injects
clicks and keys with `Rwindow_inject_*`, and checks what the widgets hold:

```
06-menus: 7 checks, 0 failures
PASS
```

---

## Running under Node

Node has no built-in FFI, so the same code needs a library such as
[`koffi`](https://koffi.dev). The steps are written for Deno, but the library
calls and the XML are identical. This complete program works under Node 22+ and
shows the three pieces you need:

```bash
npm install koffi
RAVEN_LIBRARY=/path/to/libraven.so node node-example.mjs
```

```js
import koffi from "koffi";

const lib = koffi.load(process.env.RAVEN_LIBRARY ?? "libraven.so");
const Rinit_ex = lib.func("int Rinit_ex(void *desc)");
const Rwindow_create_from_xml = lib.func("unsigned int Rwindow_create_from_xml(const char *xml, size_t size)");
const Rwidget_find = lib.func("unsigned int Rwidget_find(unsigned int scope, const char *name)");
const Rlabel_get_text = lib.func("const char *Rlabel_get_text(unsigned int label)");
const Rdraw_frame = lib.func("int Rdraw_frame()");
const Raction_bind = lib.func("void Raction_bind(const char *name, void *callback, void *user_data)");
const Rshutdown = lib.func("void Rshutdown()");

// RInitDesc: backend (2 = null) at 0, headless at 32.
const desc = Buffer.alloc(40);
desc.writeInt32LE(2, 0);
desc.writeInt32LE(1, 32);
if (!Rinit_ex(desc)) throw new Error("cannot start Raven");

const xml = '<Window Title="Node" Width="300" Height="200"><Label Name="hi" Text="hello"/></Window>';
const window = Rwindow_create_from_xml(xml, xml.length); // byte length: the XML is ASCII
if (!window) throw new Error("the window could not be built");
```

`koffi` needs a **prototype** for a callback, and the pointer must be kept:

```js
const Action = koffi.proto("void Action(unsigned int sender, const char *action)");
const onGreet = koffi.register((sender, action) => { /* ... */ }, koffi.pointer(Action));
Raction_bind("app.greet", onGreet, null);
```

If you port the course to Node, replace `raven.ts` with a koffi version of the
same functions and copy the step logic as it is. Step 09 packages a Deno
application; a Node one ships the script, `node_modules` and a launcher instead.

---

## Conventions used in the code

* **`import * as r from "../raven.ts"`.** One module, names close to the C API.
* **`Rid` is a handle**: an opaque number, checked on every call.
  `r.INVALID_ID` is the null handle.
* **Texts are `string`.** The binding encodes and decodes UTF-8.
* **UI in XML, behaviour in TypeScript.** `OnClicked="name"` fires the action
  `name`; `actionBind("name", fn)` answers it. A widget the code touches gets a
  `Name` and is found with `find`.
* **The loop is yours**: `while (r.waitFrame(-1.0)) {}`.

---

## When something does not work

| Symptom | Cause and fix |
|---|---|
| `Requires ffi access` | Run with `--allow-ffi`; `deno task` does it for you. |
| `the Raven library was not found` | `RAVEN_LIBRARY` is unset or wrong. Build Raven with `-DRAVEN_BUILD_SHARED=ON`. |
| `cannot start Raven (is a Vulkan driver installed?)` | A real window was requested on a machine without a driver. Use `--test`. |
| `window.xml could not be read` | The step reads the file with `import.meta.url`; keep the `.xml` next to `main.ts`. |
| The process crashes after an action | The callback was garbage-collected. `actionBind`/`onEditorChanged` keep theirs; a hand-written `Deno.UnsafeCallback` must be kept too. |
| A widget has a rect of zero | No frame drawn yet: call `r.drawFrame()` before `r.rect`. |
