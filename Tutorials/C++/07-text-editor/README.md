# Step 07 — A text editor and files

**Level 4 · Advanced · ~35 minutes**

A multi-line editor with line numbers, undo, clipboard and syntax colours is
**one widget** in Raven. This step wires it to files, and introduces the file
dialogs of the system.

---

## What you will learn

### The editor

```cpp
Rid editor = Rtext_editor_create("initial text");
Rtext_editor_get_text(editor);          // const char*, valid until the next edit
Rtext_editor_get_length(editor);        // bytes
Rtext_editor_set_changed_callback(editor, on_changed, nullptr);
Rtext_editor_set_line_numbers(editor, R_TRUE);
Rtext_editor_set_word_wrap(editor, R_TRUE);
Rtext_editor_set_language(editor, "C"); // or Rlanguage_for_file(path)
Rtext_editor_undo(editor); Rtext_editor_redo(editor); Rtext_editor_select_all(editor);
```

### Files, the portable way

```cpp
size_t size = 0;
char* text = Rfile_read(path, &size);   // NULL on failure
Rmemory_free(text);                     // the library allocated it

Rfile_write(path, text, length);        // R_TRUE on success
```

Use `Rfile_read` / `Rfile_write` rather than `fopen`: same behaviour on every
platform, and text files opened through them work with drag and drop later.

### Dialogs of the system

```cpp
RFileDialogDesc desc{};
desc.parent = window;                   // the dialog is modal to this window
desc.title = "Open a text";
const char* chosen = Rfile_dialog_open(&desc);   // NULL when cancelled
```

`Rfile_dialog_save` adds `file_name`, `filters`, `filter_count` and
`default_extension`. Both return a path owned by the library — copy it before
the next dialog.

---

## The program

[`main.cpp`](main.cpp) is a minimal but complete editor:

```
VBox
├── HBox     toolbar: Open..., Save
├── TextEditor  (fill 1, line numbers, word wrap)
└── Label    status bar
```

Four functions carry the logic, and they take a **path**, not a dialog:

| Function | Used by |
|---|---|
| `open_path(path)` | the Open dialog, a drag and drop, the command line, the test |
| `save_to(path)` | Save (when the name is known), Save As, the test |
| `ask_open()` / `ask_save()` | the toolbar, and only in a normal run |

That split is deliberate: the test calls `open_path` and `save_to` with a
temporary file, so nothing has to click through a system dialog.

`on_changed` is the only place that sets `modified`, and it also updates the
title with a `*`:

```cpp
static void on_changed(Rid editor, void* user_data) {
    if (app.modified) return;
    app.modified = true;
    update_title();
    set_status("Modified");
}
```

---

## Build

```bash
cd "Tutorials/C++"
cmake --build build --target 07-text-editor
```

## Run

```bash
./build/07-text-editor
```

Type in the editor. Press **Open...** and choose a `.c` or `.cpp` or `.py` file:
the language, and its colours, come from the file name. Edit, press **Save** —
the title loses its `*`. Undo with `Ctrl+Z` (the editor has it built in).

Try `Tab`, `Ctrl+/` (comment the selection), and dropping a file on the editor.

## Test

```bash
./build/07-text-editor --test
```

```
07-text-editor: 11 checks, 0 failures
PASS
```

The test writes a file with `Rfile_write`, opens it, checks the text and the
length, types into the editor with `Rwindow_inject_text`, verifies the
`modified` flag, saves, and reads the file back to prove the content reached
the disk. It deletes the file when done.

---

## Checkpoint

* Why does `open_path` take a path instead of opening a dialog itself?
* What happens to the string returned by `Rtext_editor_get_text` after the next
  keystroke? (It is invalidated — copy it if you need it later.)
* Who allocates the memory returned by `Rfile_read`? Who frees it?

---

## Exercises

1. Add **Save As** (always a dialog) next to **Save**.
2. Show the cursor position in the status bar with
   `Rtext_editor_get_cursor(editor, &line, &column)` from an
   `Rset_frame_callback`.
3. Add a **New** button that clears the editor and the path.
4. Ask before discarding unsaved changes:
   `Rmessage_box(window, "Unsaved changes", "...", R_MESSAGE_OK_CANCEL)` and
   compare with `R_MESSAGE_RESULT_OK`. *(Step 08 does exactly this.)*

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Rfile_read` returns `NULL` | The path is wrong, or the file is not readable. Check `Rerror` output — the library reports it. |
| Saving writes an empty file | `Rtext_editor_get_length` was not used; `strlen` on the text stops at a `NUL` inside the document. |
| The dialog never returns | It is modal: `Rfile_dialog_*` blocks until the user answers. Never call it from a `--test` run. |
| Colours do not appear | The language name is wrong, or the file has no extension: `Rlanguage_for_file` returns `NULL` for unknown types. |
| `Rtext_editor_get_text` points to garbage later | The pointer is only valid until the next edit. Copy it. |

**Previous:** [Step 06 — Menus](../06-menus/README.md) ·
**Next:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md)
