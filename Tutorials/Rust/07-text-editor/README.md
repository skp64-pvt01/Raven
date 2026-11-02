# Step 07 — A text editor and files

**Level 4 · Advanced · ~35 minutes**

A multi-line editor with line numbers, undo, clipboard and syntax colours is
**one widget**. This step wires it to files and shows how a C callback becomes a
Rust closure.

---

## What you will learn

### The editor

```rust
Rtext_editor_get_text(editor);          // *const c_char, valid until the next edit
Rtext_editor_get_length(editor);        // bytes
Rtext_editor_set_text(editor, text);
Rtext_editor_set_language(editor, "C"); // or Rlanguage_for_file(path)
Rtext_editor_set_line_numbers(editor, 1);
Rtext_editor_set_word_wrap(editor, 1);
Rtext_editor_undo(editor); Rtext_editor_redo(editor); Rtext_editor_select_all(editor);
```

### Files, the portable way

```rust
let mut size = 0usize;
let data = Rfile_read(cstr(path).as_ptr(), &mut size);   // null on failure
let content = text(data);
Rmemory_free(data as *mut c_void);                       // the library allocated it

Rfile_write(cstr(path).as_ptr(), content.as_ptr() as *const c_void, content.len());
```

### A C callback from Rust

`Rtext_editor_set_changed_callback` takes a C function pointer, not a closure.
A `extern "C"` trampoline carries the closure through `user_data` — exactly what
the binding's own `action_bind` does:

```rust
unsafe extern "C" fn changed_trampoline(_editor: Rid, user_data: *mut c_void) {
    (*(user_data as *mut Box<dyn FnMut()>))();
}

fn on_editor_changed<F: FnMut() + 'static>(editor: Rid, function: F) {
    let boxed: Box<Box<dyn FnMut()>> = Box::new(Box::new(function));
    unsafe {
        Rtext_editor_set_changed_callback(editor, Some(changed_trampoline), Box::into_raw(boxed) as *mut c_void)
    };
}
```

The `Box` is deliberately leaked: the library keeps the pointer for the life of
the program. (A production application would unregister and free it on shutdown;
for a GUI that lives as long as the process, this is the usual trade.)

---

## The program

[`src/main.rs`](src/main.rs) is a minimal but complete editor:

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

That split is deliberate: the test calls `open_path` and `save_to` with a
temporary file, so nothing has to click through a system dialog.

`on_editor_changed` is the only place that sets `modified`, and it also updates
the title with a `*`:

```rust
on_editor_changed(editor, move || {
    if state.borrow().modified { return; }
    state.borrow_mut().modified = true;
    update_title(window, &state);
    set_status(status, "Modified");
});
```

---

## Build

```bash
cargo build -p step-07-text-editor
```

## Run

```bash
cargo run -p step-07-text-editor
```

Type. Press **Open...** and choose a `.c`, `.cpp` or `.py` file: the language,
and its colours, come from the file name. Edit, press **Save** — the title loses
its `*`. `Ctrl+Z` undoes; `Ctrl+/` comments the selection.

## Test

```bash
cargo run -p step-07-text-editor -- --test
```

```
07-text-editor: 11 checks, 0 failures
PASS
```

The test writes a file with `Rfile_write`, opens it, checks text and length,
types into the editor with `Rwindow_inject_text`, verifies the `modified` flag,
saves, and reads the file back to prove the content reached the disk.

---

## Checkpoint

* Why does `open_path` take a path instead of opening a dialog itself?
* What happens to the pointer from `Rtext_editor_get_text` after the next
  keystroke? (It is invalidated — copy it with `text()` if you need it later.)
* Who allocates the buffer from `Rfile_read`, and who frees it?
* Why must the closure given to `on_editor_changed` be `'static`?

---

## Exercises

1. Add **Save As** (always a dialog) next to **Save**.
2. Show the cursor position in the status bar with
   `Rtext_editor_get_cursor(editor, &mut line, &mut column)` from a frame
   callback (step 08 does exactly this).
3. Add a **New** button that clears the editor and the path.
4. Ask before discarding unsaved changes: `Rmessage_box` with
   `R_MESSAGE_OK_CANCEL`.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Rfile_read` returns null | Wrong path, or the file is not readable. |
| Saving writes an empty file | Use `Rtext_editor_get_length`, not `strlen` — the document may contain NUL bytes. |
| The dialog never returns | It is modal and blocks. Never call `Rfile_dialog_*` from `--test`. |
| Colours do not appear | Unknown extension: `Rlanguage_for_file` returns null. |
| Crash in the changed callback | The `Box` was dropped. Leak it (`Box::into_raw`), as the code does. |

**Previous:** [Step 06 — Menus](../06-menus/README.md) ·
**Next:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md)
