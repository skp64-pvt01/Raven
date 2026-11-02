# Step 08 — Starter project: Notes (Python)

**Level 4 · Advanced · ~45 minutes**

This is the step the course was built for: a small text editor with a menu bar,
keyboard shortcuts, system file dialogs, an unsaved-changes guard and drag and
drop. It is deliberately plain — it is the folder you copy when you start a
desktop tool in Python.

```
File  New · Open… · Save · Save as… · Quit
Edit  Undo · Redo · Select all
View  Line numbers · Word wrap · Dark · Light
```

---

## Files

| File | Contains |
|---|---|
| [`main.py`](main.py) | the application: actions, files, events, state, and its own test helpers |
| [`notes.xml`](notes.xml) | the window: header bar, menus, editor, status bar |

Read them in this order: `notes.xml` (what the window **is**), then `main.py`
top to bottom (what it **does**).

The helpers are copied into `main.py` rather than imported from `../tutorial.py`,
so the folder stands alone.

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
                (no dialog)              (path in)                (path in)
                    │                        │                        │
                    └────── dialogs only in ask_open() / save() ──────┘
```

Three decisions make this a starter rather than a demo:

**1. The XML owns the layout, Python owns the behaviour.** Adding a menu entry is
a one-line XML change plus a branch — no widget is created in code.

**2. Every file operation takes a path.** `open_path` and `save_to` never open a
dialog; `ask_open` and `save` do. That is why the whole thing is testable
without a user, and why drag and drop reuses the same code.

**3. The state is one small object.**

```python
class App:
    window, editor, status, position, numbers
    path: str
    modified: bool
    ask_dialogs: bool      # False in --test: no user to answer
    last_line, last_column
```

`modified` is set in exactly one place (`on_changed`) and cleared in exactly two
(`new_document`, `save_to`).

### The unsaved-changes guard

```python
def can_discard():
    if not app.modified or not app.ask_dialogs:
        return True
    return r.Rmessage_box(app.window, "Unsaved changes",
                          "The text has changes that were not saved. Discard them?",
                          r.R_MESSAGE_OK_CANCEL) == r.R_MESSAGE_RESULT_OK
```

It is called by **File > New**, **File > Open**, **File > Quit**, the window close
button, `Alt+F4`, and a dropped file. One function, six doors.

### Drag and drop

```python
def on_drop(target, event, user_data):
    if event and event.phase == r.R_DROP_DROPPED and event.path_count > 0:
        path = event.paths[0].decode("utf-8", "replace")
        if can_discard():
            open_path(path)
    return 1          # "I take it"
```

---

## Run

```bash
python3 08-capstone-notes/main.py                 # welcome text
python3 08-capstone-notes/main.py notes.txt       # open a file
```

Check by hand:

- [ ] `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Q` all work.
- [ ] After typing, the title shows a `*`.
- [ ] `Ctrl+Q` with unsaved text asks before quitting.
- [ ] Closing the window asks the same question.
- [ ] **View > Line numbers** and **Word wrap** toggle and keep their check marks.
- [ ] **View > Dark** and **Light** restyle the window.
- [ ] Dropping a text file from the file manager opens it.
- [ ] Opening a `.py` or `.c` file colours it.

## Test

```bash
python3 08-capstone-notes/main.py --test
```

```
08-capstone-notes: 12 checks, 0 failures
PASS
```

The test covers the model, not the pixels: open a written file, save it, type
into it, run **File > New** through the real action, drive a checkable menu
entry, run `Ctrl+N` through the XML shortcut, and quit through `file.quit`.

---

## Using this as a starting point

1. **Copy the folder.** It needs only `raven` and the two files.
2. **Rename the application:** the `<Window Title>` in `notes.xml` and the string
   in `init(...)`.
3. **Add a menu entry** — three places, in this order:
   1. `notes.xml`: `<MenuItem Text="Export…" Shortcut="Ctrl+E" OnClicked="file.export"/>`
   2. `main.py`: `r.Raction_bind("file.export", action_file)`
   3. `action_file`: an `elif action == "file.export":` branch.
4. **Add a panel** (a preview, a file list): put it in the `VBox` of the XML with
   `Slot.Fill`, and find it with `Rwidget_find`.
5. **Ship it**: zip the folder, or make a `.desktop` entry that runs
   `python3 /path/to/main.py`. Nothing to compile.

---

## Roadmap ideas

| Idea | Where to look |
|---|---|
| Recent files sub-menu | `Rmenu_add_menu` / `Rmenu_add_item` at run time |
| Toolbar with buttons | an `HBox` in the `HeaderBar`, same `OnClicked` names |
| Language picker | `<ComboBox Name="language" OnChanged="view.language"/>` |
| Preview pane | `<Splitter>` with two children |
| Autosave | `Rtimer_create` with a handler |
| More languages | `Rlanguage_register` |

`examples/notepad` in the Raven repository is the same shape with completion,
markers and a custom language — the next file to read.

---

## Checkpoint

* How many places must change to add a menu entry?
* Why do `open_path` and `save_to` not open dialogs themselves?
* Which single function decides whether text may be discarded?
* Why does the drop handler return `1` even when it does nothing?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `notes.xml could not be read` | It is read next to `main.py`; keep the two together. |
| A menu entry does nothing | No `Raction_bind` for the name in the XML. |
| The `*` never appears | The changed callback is not set, or the text was set from code. |
| `TypeError` on save | `Rfile_write` takes `bytes`: encode the text. |
| Quitting loses the text | A new exit route was added without `can_discard`. |

**Previous:** [Step 07 — A text editor](../07-text-editor/README.md) ·
**Back to the course index:** [README](../README.md)
