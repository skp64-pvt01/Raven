# Step 01 — A first window

**Level 1 · Beginner · ~20 minutes**

A window with a label and two buttons; one counts, one quits. This is the
smallest complete Raven program in Python.

---

## What you will learn

1. `Rinit_ex` starts the library, `Rshutdown` ends it.
2. A window is built from XML; `Rwidget_find` gets a widget back by name.
3. `Raction_bind` answers `OnClicked="..."` with a Python function.
4. The frame loop belongs to `main`.

---

## The program

Open [`main.py`](main.py).

### Starting the library

```python
desc = r.RInitDesc()
desc.backend = r.R_BACKEND_VULKAN
if not r.Rinit_ex(desc):
    return 1
```

`RInitDesc()` is zeroed by Python, so every unset field keeps its default.
`tutorial.init` fills it and switches to the null backend when the program is run
with `--test`.

### A window, described in XML

```python
window = r.Rwindow_create_from_xml(XML)
label = r.Rwidget_find(window, "counter")
```

Every `Name="..."` in the XML can be found with `Rwidget_find`. Nothing in the
XML knows Python: `OnClicked="counter.add"` fires the **action**
`counter.add`, and the program binds that name to a function.

### Actions as functions

```python
clicks = [0]

def on_click(sender, action, user_data):
    clicks[0] += 1
    r.Rlabel_set_text(label, "Clicked %d time%s" % (clicks[0], ...))

r.Raction_bind("counter.add", on_click)
```

The function receives the sender and the action name. `clicks` is a one-element
list because Python closures cannot rebind an outer integer without `nonlocal` —
either tool is fine; the course uses the list.

`app.quit` needs no binding: it is one of Raven's built-in actions.

### The loop

```python
while r.Rwait_frame(-1.0):
    pass
```

`Rwait_frame` draws one frame, then sleeps until there is input or an animation
(`-1.0` = no timeout). It returns a false value after `Rquit`.

---

## Run

```bash
python3 01-first-window/main.py
```

Expected: a 440×240 window. **Click me** twice → *Clicked 2 times*. **Quit**
closes it, exit code 0.

Without a display or a driver:

```bash
python3 01-first-window/main.py --test
```

```
01-first-window: 3 checks, 0 failures
PASS
```

The test drives the real widgets with `Rwindow_inject_mouse_*` — real clicks, no
window.

---

## Checkpoint

* What are the three arguments of an action callback?
* Why is `clicks` a list rather than an integer?
* Where does the program stop drawing frames?

---

## Exercises

1. Add a third button that resets the counter.
2. Change the label's text size to `32` and its colour with
   `r.Rlabel_set_color(label, 1, 0.4, 0.4, 1)`.
3. Rename `counter.add` everywhere. How many places were there?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `ModuleNotFoundError: No module named 'raven'` | Set `PYTHONPATH` to `Raven/bindings/python`. |
| `the Raven library was not found` | Set `RAVEN_LIBRARY` to `libraven.so`. |
| The window is empty | `Rwindow_create_from_xml` returned `R_INVALID_ID`: the XML is malformed. |
| Clicking does nothing | The names in `OnClicked` and `Raction_bind` differ (case-sensitive). |

**Next:** [Step 02 — Layout](../02-layout/README.md)
