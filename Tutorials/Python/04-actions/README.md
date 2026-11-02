# Step 04 — Actions and shortcuts

**Level 2 · Elementary · ~30 minutes**

A button, a menu entry and `Ctrl+I` should run the same code. Raven's answer is
an **action**: a name, and one function bound to it.

---

## What you will learn

```python
r.Raction_bind("counter.add", on_add)                       # register
r.Raction_fire("counter.add", sender)                       # from code
r.Rwindow_register_shortcut(window, r.R_KEY_I, r.R_MOD_CTRL, "counter.add")
r.Raction_unbind("counter.add")                             # remove
```

The function receives the **sender** and the **action name**:

```python
def on_add(sender, action, user_data):
    count[0] += 1
    show(action)
```

One function can serve several names, or one name can have one function — both
are normal. The same name can arrive from a button, a shortcut or a menu entry.

---

## The program

[`main.py`](main.py) is a counter:

| Action | Button | Shortcut | Effect |
|---|---|---|---|
| `counter.add` | **+1** | `Ctrl+I` | `+1` |
| `counter.add_ten` | **+10** | — | `+10` |
| `counter.reset` | **Reset** | `Ctrl+0` | back to `0` |
| `counter.enabled` | the checkbox | — | enables/disables the add buttons |

The buttons only name the action in XML (`OnClicked="counter.add"`); shortcuts
are registered once:

```python
r.Rwindow_register_shortcut(window, r.R_KEY_I, r.R_MOD_CTRL, "counter.add")
r.Rwindow_register_shortcut(window, r.R_KEY_0, r.R_MOD_CTRL, "counter.reset")
```

### Disabling, not just ignoring

```python
def on_enabled(sender, action, user_data):
    on = r.Rcheckbox_is_checked(enabled)
    r.Rwidget_set_enabled(add, on)
    r.Rwidget_set_enabled(add_ten, on)
```

A disabled widget takes no clicks, looks disabled and is skipped by Tab. Note
that the **shortcut still fires**: it belongs to the window, not to the button.

---

## Run

```bash
python3 04-actions/main.py
```

**+1** three times → 3. `Ctrl+I` → 4. `Ctrl+0` → 0. Untick *Add buttons
enabled*, click **+1**: nothing. `Ctrl+I`: it still counts.

## Test

```bash
python3 04-actions/main.py --test
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

* What are the three arguments of an action function?
* Where is a shortcut registered, and does it need the focus?
* Why does the action work while its button is disabled?

---

## Exercises

1. Add `counter.subtract` on `Ctrl+Shift+I`
   (`r.R_MOD_CTRL | r.R_MOD_SHIFT`) and a button for it.
2. Make **Reset** ask for confirmation with
   `r.Rmessage_box(window, title, text, r.R_MESSAGE_OK_CANCEL)` and compare with
   `r.R_MESSAGE_RESULT_OK`.
3. Disable **+10** above 100 and re-enable it below.
4. **Design question:** how many places would you change to rename
   `counter.add`? Where should action names live in a bigger program?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Raction_fire` does nothing | No function is bound to that exact name (case-sensitive). |
| A shortcut types into a text input | The window shortcut was not registered, or the widget consumed the key. |
| The action fires twice | It is bound twice, or a button and a shortcut both fire it. |
| A disabled button still runs | Something other than the button fires the action: that is intended. |

**Previous:** [Step 03 — Controls](../03-controls/README.md) ·
**Next:** [Step 05 — XML files](../05-xml-files/README.md)
