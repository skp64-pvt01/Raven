# Raven tutorials for Python

A graded course that takes you from an empty `main()` to a text editor with
menus, keyboard shortcuts, file dialogs and drag and drop — the shape most
desktop tools start from.

Each step is a folder with one `main.py` and a `README.md`. Every step builds,
runs and checks itself without a screen, so the course works on a machine with
no display and no GPU.

---

## Before you start

| Need | Why |
|---|---|
| **Python 3.8 or newer** | Nothing to compile on your side. |
| **The `raven` module** | The binding: `bindings/python/raven.py` in the Raven repository, or the `raven-gui` package. |
| **The Raven shared library** | `libraven.so` / `raven.dll` / `libraven.dylib`. |
| **A Vulkan driver** | Only to open a real window. `--test` needs neither a driver nor a display. |

You do **not** need to know C, CMake, or any other GUI toolkit.

### Getting Raven

**Linux and macOS — from the repository.** The PyPI package currently ships a
Windows wheel only, so on Linux the binding comes from the checkout:

```bash
git clone https://github.com/oscar-soirey/Raven.git
cmake -S Raven -B Raven/build -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=ON
cmake --build Raven/build -j"$(nproc)"
```

Then point two environment variables at it:

```bash
export PYTHONPATH="/path/to/Raven/bindings/python:$PYTHONPATH"
export RAVEN_LIBRARY="/path/to/Raven/build/libraven.so"
```

**Windows.** `pip install raven-gui` installs the module and the library
together; nothing else to set.

**Any platform, once a wheel for it exists.** `pip install raven-gui` — the
package bundles the library, so the two variables above are unnecessary.

Check it works:

```bash
python3 -c "import raven; print(raven.__file__)"
python3 -c "
import raven as r
d = r.RInitDesc(); d.backend = r.R_BACKEND_NULL; d.headless = 1
r.Rinit_ex(d); r.Rshutdown(); print('ok')"
```

---

## Run and test the whole course

```bash
cd "Tutorials/Python"

python3 01-first-window/main.py            # opens a window
python3 01-first-window/main.py --test     # no window, no GPU: checks only
python3 check_all.py                       # every step's --test, in order
```

`check_all.py` runs the steps that have a `main.py`; step 09 is packaging only
(a CMake project that installs and packs step 08), so it has its own commands in
[09-packaging/README.md](09-packaging/README.md).

Expected at the end:

```
All steps passed.
```

---

## The course

| Level | Step | You build | You learn |
|---|---|---|---|
| **1 — Beginner** | [01](01-first-window/README.md) | A window with a label and two buttons | `Rinit_ex`, XML, `Rwidget_find`, `Raction_bind`, the frame loop |
| | [02](02-layout/README.md) | A page that arranges itself | Boxes, `Slot.Fill`, padding, alignment, spacers |
| **2 — Elementary** | [03](03-controls/README.md) | A form that updates a preview | Text input, checkbox, slider, combo box, reading state |
| | [04](04-actions/README.md) | A counter with shortcuts | Actions, `Raction_fire`, `Rwindow_register_shortcut`, enabling |
| **3 — Intermediate** | [05](05-xml-files/README.md) | The same interface, in a file | `window.xml`, `Rwidget_find`, live reload |
| | [06](06-menus/README.md) | A menu bar with sub-menus | Menus in XML, shortcuts, checkable entries, built-in actions |
| **4 — Advanced** | [07](07-text-editor/README.md) | A minimal editor that opens and saves | `TextEditor`, `Rfile_read`/`Rfile_write`, dialogs |
| | [08](08-capstone-notes/README.md) | **A starter project: Notes** | Header bar, menus, status bar, unsaved-changes guard, drag and drop |
| | [09](09-packaging/README.md) | **A DEB package and an archive of Notes** | `install(PROGRAMS/FILES)`, a launcher, CPack, DEB metadata |

Estimated time: half a day for levels 1–2, half a day for 3, an afternoon for
level 4 with the exercises.

---

## The two ways to run every step

```bash
python3 05-xml-files/main.py            # a window
python3 05-xml-files/main.py --test     # null backend, null platform
```

The `--test` run starts Raven with `R_BACKEND_NULL` and `headless`, injects
clicks and keys with `Rwindow_inject_*`, and checks what the widgets hold. Steps
01–07 get those helpers from [`tutorial.py`](tutorial.py); step 08 keeps its own
copy so the starter project is one folder.

Output of a good run:

```
06-menus: 7 checks, 0 failures
PASS
```

---

## Conventions used in the code

* **`import raven as r`.** Every function of the C API is there under its own
  name.
* **Texts are `str`.** `Rtext_input_get_text` returns a `str`; `Rlabel_set_text`
  takes one. The wrapper encodes and decodes UTF-8 for you.
* **`Rfile_read` returns `bytes`** (and frees the C buffer itself) —
  `data.decode("utf-8", "replace")` before putting it in an editor.
* **Values written through pointers are returned**: `Rwidget_get_rect(widget)`
  returns `(x, y, width, height)`, `Rtext_editor_get_cursor` returns
  `(line, column)`.
* **UI in XML, behaviour in Python.** `OnClicked="name"` fires the action
  `name`; `Raction_bind("name", function)` answers it. A widget the code touches
  gets a `Name` and is found with `Rwidget_find`.
* **`Rid` is a handle**: an opaque integer, checked on every call.
  `R_INVALID_ID` is the null handle.

---

## When something does not work

| Symptom | Cause and fix |
|---|---|
| `ModuleNotFoundError: No module named 'raven'` | `PYTHONPATH` does not include `Raven/bindings/python`, and the package is not installed. |
| `the Raven library was not found; set RAVEN_LIBRARY` | `RAVEN_LIBRARY` is unset or points to a path that does not exist. |
| `cannot start Raven (is a Vulkan driver installed?)` | A real window was requested on a machine without a driver. Use `--test`. |
| `window.xml could not be read` | The script reads it next to itself; do not move the `.py` without the `.xml`. |
| A widget has a rect of zero | No frame was drawn yet: call `Rdraw_frame()` before reading rectangles. |
| A dialog never returns | `Rfile_dialog_*` and `Rmessage_box` block until the user answers. Never call them from `--test`. |
