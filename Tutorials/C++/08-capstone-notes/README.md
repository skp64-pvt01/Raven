# Step 08 — Starter project: Notes

**Level 4 · Advanced · ~45 minutes**

This is the step the course was built for: a small text editor with a menu bar,
keyboard shortcuts, system file dialogs, an unsaved-changes guard and drag and
drop. It is deliberately plain — it is meant to be the folder you copy when you
start a desktop tool.

```
File  New · Open… · Save · Save as… · Quit
Edit  Undo · Redo · Select all
View  Line numbers · Word wrap · Dark · Light
```

---

## Files

| File | Contains | Lines |
|---|---|---|
| [`main.cpp`](main.cpp) | the application: actions, files, the window, and the headless test helpers | ~400 |
| [`notes.xml`](notes.xml) | the window: header bar, menus, editor, status bar | ~50 |
| [`CMakeLists.txt`](CMakeLists.txt) | builds this folder **on its own** | ~25 |

Read them in this order: `notes.xml` first (what the window **is**), then
`main.cpp` top to bottom (what it **does**).

Unlike the earlier steps, this folder depends on nothing but Raven: the test
helpers are in `main.cpp`, not in the course's `../common/`, so the folder can be
copied out and built on its own.

---

## Architecture

```
notes.xml ──Rwindow_create_from_xml_file──▶ window tree
                                              │
                OnClicked="file.save" ────────┤
                                              ▼
                        Raction_bind("file.save", action_file)
                                              │
                     ┌────────────────────────┼────────────────────────┐
                     ▼                        ▼                        ▼
                 new_document()           open_path()              save_to()
                 (pure, no dialog)        (path in)                (path in)
                     │                        │                        │
                     └──────── dialogs only in ask_open() / save() ────┘
```

Three decisions make this a starter rather than a demo:

**1. The XML owns the layout, the code owns the behaviour.** Adding a menu entry
is a one-line XML change plus a handler branch — no widget is created in C++.

**2. Every file operation takes a path.** `open_path` and `save_to` never open a
dialog; `ask_open` and `save` do. That is why the whole thing is testable
without a user, and why drag and drop reuses the same code.

**3. The state is one small struct.**

```cpp
struct App {
    Rid window, editor, status, position, numbers_item;
    char path[1024];
    bool modified;
    bool ask_dialogs;      // false in --test: no user to answer
    int last_line, last_column;
};
```

`modified` is set in exactly one place (`on_changed`) and cleared in exactly
two (`new_document`, `save_to`). The title, the close button and *File > Quit*
all ask `can_discard()`, so no route loses text without asking.

### The unsaved-changes guard

```cpp
static bool can_discard() {
    if (!app.modified) return true;
    if (!app.ask_dialogs) return true;
    return Rmessage_box(app.window, "Unsaved changes",
        "The text has changes that were not saved. Discard them?",
        R_MESSAGE_OK_CANCEL) == R_MESSAGE_RESULT_OK;
}
```

It is called by **File > New**, **File > Open**, **File > Quit**, the window's
close button, `Alt+F4`, and a dropped file. One function, six doors.

### Drag and drop

```cpp
static int on_drop(Rid, const RDropEvent* event, void*) {
    if (event->phase != R_DROP_DROPPED) return R_TRUE;   // R_TRUE = "I take it"
    if (event->path_count > 0 && can_discard()) open_path(event->paths[0]);
    return R_TRUE;
}
```

---

## Build

**As part of the course**, from `Tutorials/C++`:

```bash
cmake --build build --target 08-capstone-notes
```

**As a standalone project** — this folder has its own `CMakeLists.txt`:

```bash
cd "Tutorials/C++/08-capstone-notes"
cmake -S . -B build
cmake --build build
```

(Use a separate build directory; the course's `build/` belongs to the parent
project.)

## Run

```bash
./build/notes                             # welcome text
./build/notes notes.txt                   # open a file
```

From the course build, the executable is called `08-capstone-notes` instead.

Check by hand:

- [ ] `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Q` all work.
- [ ] After typing, the title shows a `*`.
- [ ] `Ctrl+Q` with unsaved text asks before quitting.
- [ ] Closing the window with its title-bar button asks the same question.
- [ ] **View > Line numbers** and **Word wrap** toggle and keep their check marks.
- [ ] **View > Dark** and **Light** restyle the whole window.
- [ ] Dropping a text file from the file manager opens it.
- [ ] Opening a `.cpp` file colours it.

## Test

```bash
./build/08-capstone-notes --test
```

```
08-capstone-notes: 14 checks, 0 failures
PASS
```

The test covers the parts a test should cover — the model, not the pixels:
open a written file, save it, type into it, run **File > New** through the real
action, drive a checkable menu entry, run `Ctrl+N` through the XML shortcut, and
quit through `file.quit`.

```bash
ctest --test-dir build --output-on-failure   # all eight steps
```

---

## Using this as a starting point

1. **Copy the folder** somewhere else and rename the project and executable in
   `CMakeLists.txt`. Nothing else is needed: it builds on its own.
2. **Rename the application:** the `<Window Title>` in `notes.xml`, and
   `desc.app_name` in `tutorial::init`.
3. **Add a menu entry** — three places, in this order:
   1. `notes.xml`: `<MenuItem Text="Export…" Shortcut="Ctrl+E" OnClicked="file.export"/>`
   2. `main.cpp`: `Raction_bind("file.export", action_file, nullptr);`
   3. `action_file`: a branch on `strcmp(action, "file.export")`.
4. **Add a panel** (a preview, a list of files): put it in the `VBox` of the
   XML with `Slot.Fill`, and find it with `Rwidget_find`.
5. **Give it a build.** `cmake --install` ships the binary, but a real
   application usually also wants:

```cmake
install(TARGETS notes RUNTIME DESTINATION bin)
install(FILES notes.xml DESTINATION share/notes)
```

and then reads the XML from `share/notes` instead of `RAVEN_TUTORIAL_DIR`.

---

## Roadmap ideas

| Idea | Where to look |
|---|---|
| Recent files sub-menu | `Rmenu_add_menu` + `Rmenu_add_item` at run time (step 06) |
| Toolbar with icons | `Rwidget_create(R_WIDGET_HBOX)` in the header bar, `Rbutton_create` |
| Language picker | `Rcombo_box_add_item` + `Rtext_editor_set_language` (step 03) |
| Find and replace | no built-in API: a second `TextEditor` pane, or scan `Rtext_editor_get_text` and `Rtext_editor_select_all` |
| Preview pane | `Rwidget_create(R_WIDGET_SPLITTER)` |
| Live language detection | `Rlanguage_for_file`, already used in `open_path` |
| Autosave | `Rtimer_create` (see `examples/notepad`) |
| More languages | `Rlanguage_register` — a small `RLanguageDesc` |

The repository's `examples/notepad` takes this same shape and adds completion,
markers, a language combo box and a custom language. When this step feels
comfortable, that is the next file to read.

---

## Checkpoint

* How many places must change to add a menu entry? Which one is the odd one?
* Why do `open_path` and `save_to` not open dialogs themselves?
* Which single function decides whether text may be discarded?
* Why is `ask_dialogs` a field of `App` and not a global?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cannot read notes.xml` | Run from `build/`, or set `RAVEN_TUTORIAL_DIR`; the compile definition does this for you. |
| A menu entry does nothing | The action is not bound, or the name in the XML and in `Raction_bind` differ. |
| The `*` never appears | `Rtext_editor_set_changed_callback` was not set, or it was set after the text was loaded (loading does not fire it). |
| Save writes to the wrong file | `app.path` was not updated after *Save as*; `save_to` does it, `save` does not. |
| Quitting loses the text | A new exit route was added without `can_discard()`. |
| The window has no title bar buttons | `CustomHeader="true"` puts them in the `HeaderBar`; the system bar is gone by design. |

**Previous:** [Step 07 — A text editor](../07-text-editor/README.md) ·
**Back to the course index:** [README](../README.md)
