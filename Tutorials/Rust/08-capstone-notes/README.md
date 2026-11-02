# Step 08 — Starter project: Notes (Rust)

**Level 4 · Advanced · ~45 minutes**

This is the step the course was built for: a small text editor with a menu bar,
keyboard shortcuts, system file dialogs, an unsaved-changes guard and drag and
drop. It is deliberately plain — it is the crate you copy when you start a
desktop tool in Rust.

```
File  New · Open… · Save · Save as… · Quit
Edit  Undo · Redo · Select all
View  Line numbers · Word wrap · Dark · Light
```

---

## Files

| File | Contains |
|---|---|
| [`src/main.rs`](src/main.rs) | the application: actions, files, events, state |
| [`notes.xml`](notes.xml) | the window: header bar, menus, editor, status bar |
| [`src/harness.rs`](src/harness.rs) | start/test helpers (copied in, so the crate stands alone) |
| [`Cargo.toml`](Cargo.toml) | one path to the binding |

Read them in this order: `notes.xml` (what the window **is**), then `main.rs`
top to bottom (what it **does**).

---

## Architecture

```
notes.xml ──Rwindow_create_from_xml_file──▶ window tree
                                             │
               OnClicked="file.save" ────────┤
                                             ▼
                       action_bind("file.save", closure)
                                             │
                    ┌────────────────────────┼────────────────────────┐
                    ▼                        ▼                        ▼
                new_document()           open_path()              save_to()
                (no dialog)              (path in)                (path in)
                    │                        │                        │
                    └────── dialogs only in ask_open() / save() ──────┘
```

Three decisions make this a starter rather than a demo:

**1. The XML owns the layout, Rust owns the behaviour.** Adding a menu entry is
a one-line XML change plus a closure branch — no widget is built in code.

**2. Every file operation takes a path.** `open_path` and `save_to` never open a
dialog; `ask_open` and `save` do. That is why the whole thing is testable
without a user, and why drag and drop reuses the same code.

**3. The state is one struct behind an `Rc<RefCell<_>>`.**

```rust
struct App {
    window: Rid, editor: Rid, status: Rid, position: Rid, numbers: Rid,
    path: String,
    modified: bool,
    ask_dialogs: bool,      // false in --test: no user to answer
    last_line: i32, last_column: i32,
}
type Shared = Rc<RefCell<App>>;
```

Closures registered with `action_bind` are `'static` and `FnMut`, so each one
receives a clone of `Shared`. Borrows are kept short: never across a modal
dialog or across a call that could fire another action.

### The C callbacks

Raven asks for C function pointers. One trampoline per signature carries a Rust
closure, and the `Box` is leaked for the life of the program:

```rust
unsafe extern "C" fn changed_trampoline(_editor: Rid, user_data: *mut c_void) {
    (*(user_data as *mut Box<dyn FnMut()>))();
}
```

That is how `Rtext_editor_set_changed_callback`, `Rset_frame_callback`,
`Rwindow_set_close_callback` and `Rwindow_set_drop_callback` are wired.

### The unsaved-changes guard

```rust
fn can_discard(shared: &Shared) -> bool {
    let (window, modified, ask) = { let app = shared.borrow(); (app.window, app.modified, app.ask_dialogs) };
    if !modified || !ask { return true; }
    unsafe { Rmessage_box(window, cstr("Unsaved changes").as_ptr(), ..., R_MESSAGE_OK_CANCEL) == R_MESSAGE_RESULT_OK }
}
```

It is called by **File > New**, **File > Open**, **File > Quit**, the window's
close button, `Alt+F4`, and a dropped file. One function, six doors.

### Drag and drop

```rust
move |event: &RDropEvent| {
    if event.phase == R_DROP_DROPPED && event.path_count > 0 {
        let path = unsafe { text(*event.paths) };
        if can_discard(&state) { open_path(&state, &path); }
    }
    true
}
```

Returning `true` says "I take it" and highlights the window while the pointer
moves.

---

## Build

**In the course**, from `Tutorials/Rust`:

```bash
cargo build -p step-08-capstone-notes
```

**Standalone** — the crate depends on Raven by an explicit path:

```bash
cd "Tutorials/Rust/08-capstone-notes"
cargo run
```

If the binding is not at `../../../bindings/rust`, edit the single line in
`Cargo.toml`:

```toml
raven-gui = { path = "/home/you/code/Raven/bindings/rust" }
```

## Run

```bash
cargo run -p step-08-capstone-notes                 # welcome text
cargo run -p step-08-capstone-notes -- notes.txt    # open a file
```

Check by hand:

- [ ] `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Q` all work.
- [ ] After typing, the title shows a `*`.
- [ ] `Ctrl+Q` with unsaved text asks before quitting.
- [ ] Closing the window asks the same question.
- [ ] **View > Line numbers** and **Word wrap** toggle and keep their check marks.
- [ ] **View > Dark** and **Light** restyle the window.
- [ ] Dropping a text file from the file manager opens it.
- [ ] Opening a `.rs` or `.cpp` file colours it.

## Test

```bash
cargo run -p step-08-capstone-notes -- --test
cargo test -p step-08-capstone-notes        # the same, through cargo test
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

1. **Copy the folder** and rename the package and binary in `Cargo.toml`.
2. **Point the dependency** at your Raven checkout (one line).
3. **Rename the application:** the `<Window Title>` in `notes.xml`, and the
   string in `harness::init`.
4. **Add a menu entry** — three places, in this order:
   1. `notes.xml`: `<MenuItem Text="Export…" Shortcut="Ctrl+E" OnClicked="file.export"/>`
   2. `main.rs`: `action_bind("file.export", move |_, _| export(&state));`
   3. the function it calls.
5. **Add a panel** (a preview, a list of files): put it in the `VBox` of the XML
   with `Slot.Fill`, and find it with `widget_find`.
6. **Ship it**: `cargo build --release` and keep `notes.xml` next to the binary
   (or embed it with `include_str!` — the whole file is a `&'static str`).

---

## Roadmap ideas

| Idea | Where to look |
|---|---|
| Recent files sub-menu | built at run time with `Rmenu_add_item` |
| Toolbar with buttons | an `HBox` in the `HeaderBar`, same `OnClicked` names |
| Language picker | `<ComboBox Name="language" OnChanged="view.language"/>` |
| Preview pane | `<Splitter>` with two children |
| Autosave | `Rtimer_create` with a closure trampoline |
| More languages | `Rlanguage_register` |

`examples/notepad` in the Raven repository is the same shape with completion,
markers and a custom language — the next file to read.

---

## Checkpoint

* How many places must change to add a menu entry?
* Why do `open_path` and `save_to` not open dialogs themselves?
* Which single function decides whether text may be discarded?
* Why is the `Box` given to a C callback leaked?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `notes.xml could not be read` | It is read through `CARGO_MANIFEST_DIR`; run through `cargo run -p ...`. |
| A menu entry does nothing | No `action_bind` for the name in the XML. |
| The `*` never appears | The changed callback was not set, or setting the text does not fire it (it should not). |
| `BorrowMutError` | A `borrow()`/`borrow_mut()` held across a dialog or another action. Copy the values out first. |
| Quitting loses the text | A new exit route was added without `can_discard`. |
| Drag and drop does nothing | On some desktops the drop needs a moment over the window before release; the first phase must return `true`. |

**Previous:** [Step 07 — A text editor](../07-text-editor/README.md) ·
**Back to the course index:** [README](../README.md)
