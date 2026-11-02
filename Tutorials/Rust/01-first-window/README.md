# Step 01 — A first window

**Level 1 · Beginner · ~20 minutes**

A window with a label and two buttons; one button counts, the other quits. This
is the smallest complete Raven program in Rust; everything later is more of it.

---

## What you will learn

1. `Rinit_ex` starts the library, `Rshutdown` ends it.
2. A window is built from XML; `widget_find` gets a widget back by name.
3. `action_bind` answers `OnClicked="..."` with a Rust closure.
4. The frame loop belongs to `main`.

---

## The program

Open [`src/main.rs`](src/main.rs). In order:

### Starting the library

```rust
let mut desc: RInitDesc = std::mem::zeroed();
desc.backend = R_BACKEND_VULKAN;
if Rinit_ex(&desc) == 0 { /* cannot start */ }
```

`RInitDesc` is a C structure: `zeroed()` selects the default for every field.
`harness::init` fills it and switches to the null backend when the program is
run with `--test`.

`Rshutdown()` releases every window, widget and resource still alive;
`harness::finish` calls it and prints the result of the checks.

### A window, described in XML

```rust
let window = window_from_xml(XML);
let label = widget_find(window, "counter");
```

`window_from_xml` wraps `Rwindow_create_from_xml` and takes a `&str`. Every
`Name="..."` in the XML can then be found with `widget_find`.

Nothing in the XML knows Rust: `OnClicked="counter.add"` fires the **action**
`counter.add`, and the program binds that name to code.

### Actions as closures

```rust
let clicks = Rc::new(RefCell::new(0u32));
let counted = clicks.clone();
action_bind("counter.add", move |_sender, _action| {
    let mut count = counted.borrow_mut();
    *count += 1;
    let text = format!("Clicked {count} time{}", ...);
    unsafe { Rlabel_set_text(label, cstr(&text).as_ptr()) };
});
```

Two things worth noticing:

* `cstr("...")` makes a `CString` for the C function. It lives until the end of
  the statement, which is long enough: the library copies the text.
* The closure is `FnMut` and lives until the program ends, so it cannot borrow
  a local. `Rc<RefCell<_>>` is how the counter is shared between the closure and
  the rest of `main`.

`app.quit` needs no binding at all — it is one of Raven's built-in actions.

### The loop

```rust
while unsafe { Rwait_frame(-1.0) } != 0 {}
```

`Rwait_frame` draws one frame, then sleeps until there is input or an animation
(`-1.0` = no timeout). It returns `0` after `Rquit`.

---

## Build

From `Tutorials/Rust`:

```bash
cargo build -p step-01-first-window
```

Build Raven's shared library once, from the repository root:

```bash
cmake -S . -B build -DRAVEN_BUILD_SHARED=ON
cmake --build build
```

Nothing to export: `.cargo/config.toml` in the course folder points the linker
at `build/` and records it as an rpath (see the [course README](../README.md)).

## Run

```bash
cargo run -p step-01-first-window
```

Expected: a 440×240 window. **Click me** twice → *Clicked 2 times*. **Quit**
closes it and the process exits with 0.

No display or no Vulkan driver? Use the null backend:

```bash
cargo run -p step-01-first-window -- --test
```

```
01-first-window: 3 checks, 0 failures
PASS
```

The test drives the real widgets with `Rwindow_inject_mouse_move` and
`Rwindow_inject_mouse_button` — real clicks, no window.

---

## Checkpoint

* What does `action_bind` receive as its two arguments?
* Why can the closure not capture a plain `let mut clicks = 0`?
* Where does the program stop drawing frames?

---

## Exercises

1. Add a third button that resets the counter to zero.
2. Change the label's text size to `32` and give it a colour with
   `Rlabel_set_color(label, 1.0, 0.4, 0.4, 1.0)`.
3. Replace the `Rc<RefCell<_>>` counter with a `static ATOMIC: AtomicU32` and
   see how the closure changes. Which do you prefer, and why?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cannot find -lraven` | Raven's shared library is not built: `cmake -S . -B build -DRAVEN_BUILD_SHARED=ON && cmake --build build` from the repository root. |
| `error while loading shared libraries` | The binary has no rpath to `build/`: rebuild it here (the course config adds it). |
| The window appears empty | `window_from_xml` returned `R_INVALID_ID` — the XML is malformed. Print errors with `Rerror_set_callback`. |
| Clicking does nothing | The name in `OnClicked` and in `action_bind` differ. They are case-sensitive. |

**Next:** [Step 02 — Layout](../02-layout/README.md)
