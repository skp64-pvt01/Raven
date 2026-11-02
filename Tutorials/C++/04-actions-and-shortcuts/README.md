# Step 04 — Actions and shortcuts

**Level 2 · Elementary · ~30 minutes**

A menu entry, a button and `Ctrl+I` should run the same code. Raven's answer is
an **action**: a name, and one callback bound to it.

---

## What you will learn

```cpp
Raction_bind("counter.add", on_action, nullptr);                          // register
Raction_fire("counter.add", sender);                                      // from code or a button
Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL, "counter.add");    // from the keyboard
Raction_unbind("counter.add");                                            // remove
```

The callback receives the action's **name** and the widget that sent it:

```cpp
static void on_action(Rid sender, const char* action, void* user_data) {
    if (strcmp(action, "counter.add") == 0) { ... }
}
```

For a small application this `strcmp` chain *is* the dispatcher, and it is the
same chain a menu-driven program of any size starts with.

---

## The program

[`main.cpp`](main.cpp) is a counter with three actions:

| Action | Button | Shortcut | Effect |
|---|---|---|---|
| `counter.add` | **+1** | `Ctrl+I` | `+1` |
| `counter.add_ten` | **+10** | — | `+10` |
| `counter.reset` | **Reset** | `Ctrl+0` | back to `0` |

The important part is that `run()` holds the logic **once**:

```cpp
static void run(const char* action) {
    if (strcmp(action, "counter.add") == 0) g_value += 1;
    else if (strcmp(action, "counter.add_ten") == 0) g_value += 10;
    else if (strcmp(action, "counter.reset") == 0) g_value = 0;
    ...
}
```

The buttons do not increment; they *fire the action*:

```cpp
Rbutton_set_clicked_callback(g_add, [](Rid sender, void*) {
    Raction_fire("counter.add", sender);
}, nullptr);
```

That indirection is what lets the same name also arrive from a menu item (step
06) or from an XML `OnClicked="counter.add"` (step 05).

### Disabling, not just ignoring

```cpp
Rwidget_set_enabled(g_add, checked);
```

A disabled widget takes no clicks, looks disabled, and is skipped by Tab — and
its state is readable with `Rwidget_is_enabled`. Do this rather than guarding
inside the callback. Note that the **shortcut still fires the action**: a
shortcut belongs to the window, not to a widget.

---

## Build

```bash
cd "Tutorials/C++"
cmake --build build --target 04-actions-and-shortcuts
```

## Run

```bash
./build/04-actions-and-shortcuts
```

Press **+1** three times → 3. Press `Ctrl+I` → 4. Press `Ctrl+0` → 0. Untick
**Add buttons enabled**, then click **+1**: nothing. Press `Ctrl+I` anyway: the
counter goes up, because the shortcut is not the button.

## Test

```bash
./build/04-actions-and-shortcuts --test
```

```
04-actions-and-shortcuts: 9 checks, 0 failures
PASS
```

The test fires each action by a different route — a click, a key, and a direct
`Raction_fire` — and checks the same `g_value` each time. That is the property
worth testing: *one action, several triggers, one implementation*.

---

## Checkpoint

* What are the two arguments of an action callback?
* Where is a shortcut registered, and does it depend on the focus?
* Why does the action still work while its button is disabled?

---

## Exercises

1. Add `counter.subtract` on `Ctrl+Shift+I` and a button for it.
   (`R_MOD_CTRL | R_MOD_SHIFT`.)
2. Make `Reset` ask for confirmation with
   `Rmessage_box(window, "Reset", "Set the counter to zero?", R_MESSAGE_OK_CANCEL)`
   and compare the result with `R_MESSAGE_RESULT_OK`.
3. Disable the **+10** button when the value is above 100, and re-enable it
   when it drops. Which function do you call, and from where?
4. **Design question.** You now have three routes to `counter.add`. How many
   places would you have to change to rename it? What does that suggest about
   where the name should live in a larger program?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Raction_fire` does nothing | No handler is bound to that exact name. Names are case-sensitive and not trimmed. |
| A shortcut types into a text input instead | The window shortcut was not registered, or the key is already handled by the focused widget. |
| `Ctrl+Shift+I` fires plain `Ctrl+I` | The callback compares modifiers exactly: register both combinations if both must work. |
| The action fires twice | The button callback fires it *and* a shortcut is bound to the same key, or the handler was bound twice. |

**Previous:** [Step 03 — Controls](../03-controls/README.md) ·
**Next:** [Step 05 — XML interfaces](../05-xml-interface/README.md)
