# Step 04 — Actions and shortcuts

**Level 2 · Elementary · ~30 minutes**

A button, a menu entry and `Ctrl+I` should run the same code. Raven's answer is
an **action**: a name, and one closure bound to it.

---

## What you will learn

```rust
action_bind("counter.add", move |sender, action| { ... });   // register
Raction_fire(cstr("counter.add").as_ptr(), sender);          // from code
Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL as u32, name.as_ptr());
Raction_unbind(cstr("counter.add").as_ptr());                // remove
```

The closure receives the **sender** and the **action name**. One `match` on the
name is the dispatcher of a small application — and the same name can arrive
from a button, a shortcut or a menu entry.

---

## The program

[`src/main.rs`](src/main.rs) is a counter:

| Action | Button | Shortcut | Effect |
|---|---|---|---|
| `counter.add` | **+1** | `Ctrl+I` | `+1` |
| `counter.add_ten` | **+10** | — | `+10` |
| `counter.reset` | **Reset** | `Ctrl+0` | back to `0` |
| `counter.enabled` | the checkbox | — | enables/disables the two add buttons |

The buttons only name the action in XML (`OnClicked="counter.add"`); the
shortcut is registered once:

```rust
let add_name = cstr("counter.add");
unsafe { Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL as u32, add_name.as_ptr()) };
```

### State shared by closures

```rust
let count = Rc::new(RefCell::new(0i32));
{
    let count = count.clone();
    action_bind("counter.reset", move |_sender, action| {
        *count.borrow_mut() = 0;
        show(&count, label, status, action);
    });
}
```

`action_bind` needs `'static + FnMut`, so each closure owns a clone of the `Rc`.
Keep `borrow()` scopes short — never across a call into Raven that could fire
another action.

### Disabling, not just ignoring

```rust
action_bind("counter.enabled", move |_sender, _action| unsafe {
    let on = Rcheckbox_is_checked(enabled);
    Rwidget_set_enabled(add, on);
    Rwidget_set_enabled(add_ten, on);
});
```

A disabled widget takes no clicks, looks disabled, and is skipped by Tab. Notice
that the **shortcut still fires**: it belongs to the window, not to a widget.

---

## Build

```bash
cargo build -p step-04-actions
```

## Run

```bash
cargo run -p step-04-actions
```

**+1** three times → 3. `Ctrl+I` → 4. `Ctrl+0` → 0. Untick *Add buttons
enabled*, click **+1**: nothing. Press `Ctrl+I`: it still counts.

## Test

```bash
cargo run -p step-04-actions -- --test
```

```
04-actions: 8 checks, 0 failures
PASS
```

The test fires the same action three ways — a click, a key, and a direct
`Raction_fire` — and checks one counter. That is the property worth testing:
*one action, several triggers, one implementation*.

---

## Checkpoint

* What are the two arguments of an action closure?
* Where is a shortcut registered, and does it depend on the focus?
* Why does the action work while its button is disabled?

---

## Exercises

1. Add `counter.subtract` on `Ctrl+Shift+I`
   (`R_MOD_CTRL as u32 | R_MOD_SHIFT as u32`) and a button for it.
2. Make **Reset** ask for confirmation with
   `Rmessage_box(window, title, text, R_MESSAGE_OK_CANCEL)` and compare with
   `R_MESSAGE_RESULT_OK`.
3. Disable **+10** above 100, and re-enable it below.
4. **Design question.** How many places would you change to rename
   `counter.add`? What does that suggest about where action names should live?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Raction_fire` does nothing | No closure is bound to that exact name. Names are case-sensitive. |
| The shortcut types into a text input | The window shortcut was not registered, or the focused widget consumed the key. |
| `BorrowMutError` | Two closures borrow the same `RefCell` at once — usually one borrow kept across a call that fires another action. |
| The action fires twice | It is bound twice, or a button and a shortcut both fire it. |

**Previous:** [Step 03 — Controls](../03-controls/README.md) ·
**Next:** [Step 05 — XML files](../05-xml-files/README.md)
