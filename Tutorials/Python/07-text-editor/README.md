# Step 07 — A text editor and files

**Level 4 · Advanced · ~35 minutes**

A multi-line editor with line numbers, undo, clipboard and syntax colours is
**one widget**. This step wires it to files and the dialogs of the system.

---

## What you will learn

### The editor

```python
r.Rtext_editor_get_text(editor)            # str, valid until the next edit
r.Rtext_editor_get_length(editor)          # bytes
r.Rtext_editor_set_text(editor, text)
r.Rtext_editor_set_language(editor, "C")   # or r.Rlanguage_for_file(path)
r.Rtext_editor_get_cursor(editor)          # (line, column)
r.Rtext_editor_undo(editor); r.Rtext_editor_redo(editor); r.Rtext_editor_select_all(editor)
```

### Files, the portable way

```python
data = r.Rfile_read(path)                  # bytes, or None
text = data.decode("utf-8", "replace")     # the C buffer is freed for you

r.Rfile_write(path, content.encode("utf-8"))   # True on success
```

### Dialogs of the system

```python
desc = r.RFileDialogDesc()
desc.parent = window          # modal to this window
desc.title = b"Open a text"   # ctypes: bytes, not str
desc.file_name = b"untitled.txt"
desc.default_extension = b"txt"
chosen = r.Rfile_dialog_open(desc)     # None when cancelled
```

The path returned is owned by the library — copy it (a `str`) before the next
dialog.

---

## The program

[`main.py`](main.py) is a minimal but complete editor:

```
VBox
├── HBox        toolbar: Open..., Save
├── TextEditor  (Fill 1, line numbers, word wrap)
└── Label       status bar
```

Four functions carry the logic, and they take a **path**, not a dialog:

| Function | Used by |
|---|---|
| `open_path(path)` | the Open dialog, the command line, the test |
| `save_to(path)` | Save (when the name is known), Save As, the test |
| `ask_open()` / `ask_save()` | the toolbar, and only in a normal run |

That split is what makes the step testable: the test calls `open_path` and
`save_to` with a temporary file, so nothing has to click through a system dialog.

The editor reports typing through a callback:

```python
def on_changed(editor_id, user_data):
    if state["modified"]:
        return
    state["modified"] = True
    update_title()          # adds the * to the title
    set_status("Modified")

r.Rtext_editor_set_changed_callback(editor, on_changed)
```

---

## Run

```bash
python3 07-text-editor/main.py
```

Type. Press **Open...** and choose a `.c`, `.cpp` or `.py` file: the language,
and its colours, come from the file name. Edit, press **Save** — the title loses
its `*`. `Ctrl+Z` undoes; `Ctrl+/` comments the selection.

## Test

```bash
python3 07-text-editor/main.py --test
```

```
07-text-editor: 10 checks, 0 failures
PASS
```

The test writes a file with `Rfile_write`, opens it, checks the text and the
length, types with `Rwindow_inject_text`, verifies the `modified` flag, saves,
and reads the file back to prove the content reached the disk.

---

## Checkpoint

* Why does `open_path` take a path instead of opening a dialog?
* What happens to the text from `Rtext_editor_get_text` after the next keystroke?
* Why must `Rfile_read`'s result be decoded, while `Rtext_editor_get_text`
  already gives a `str`?
* What does `Rfile_dialog_open` return when the user cancels?

---

## Exercises

1. Add **Save As** (always a dialog) next to **Save**.
2. Show the cursor position with `Rtext_editor_get_cursor` from
   `Rset_frame_callback(handler)` — the handler receives `(delta, user_data)`.
3. Add a **New** button that clears the editor and the path.
4. Ask before discarding unsaved changes with `Rmessage_box`.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Rfile_read` returns `None` | Wrong path, or the file is not readable. |
| `TypeError` when writing | `Rfile_write` takes `bytes`: call `.encode("utf-8")`. |
| The dialog never returns | It is modal and blocks. Never call it from `--test`. |
| Colour never appears | Unknown extension: `Rlanguage_for_file` returns `None`. |
| The `*` never appears | The changed callback is not set, or the text was set from code (that does not fire it). |

**Previous:** [Step 06 — Menus](../06-menus/README.md) ·
**Next:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md)
