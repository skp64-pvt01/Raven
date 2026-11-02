# Step 03 — Controls and state

**Level 2 · Elementary · ~30 minutes**

Four controls, one preview label. The point of this step is a habit: **the
widget is the source of truth**. You never mirror a checkbox's state in your own
variable — you ask the checkbox.

---

## What you will learn

| Control | Created with | Read | Written |
|---|---|---|---|
| single-line text | `Rwidget_create(R_WIDGET_TEXT_INPUT)` | `Rtext_input_get_text` | `Rtext_input_set_text` |
| checkbox | `Rcheckbox_create("...")` | `Rcheckbox_is_checked` | `Rcheckbox_set_checked` |
| slider | `Rwidget_create(R_WIDGET_SLIDER)` | `Rslider_get_value` | `Rslider_set_value` |
| combo box | `Rwidget_create(R_WIDGET_COMBO_BOX)` | `Rcombo_box_get_selected` | `Rcombo_box_set_selected` |

Each one has a *changed* callback that says **when** something happened; the
callback reads the new value from the widget. That is why the program below has
a single `refresh()` called by all four callbacks.

---

## The program

[`main.cpp`](main.cpp) is a small profile form:

```
VBox
├── Label      "Controls"
├── Label      "Your name"
├── TextInput  Name                      → changed callback
├── Checkbox   "Shout it"                → changed callback
├── Label      "Text size"
├── Slider     12 … 32                   → changed callback
├── Label      "Theme"
├── ComboBox   Light / Dark              → applies the theme
├── Separator
└── Label      preview: "Hello, Ada!"
```

`refresh()` does the actual work:

```cpp
const char* typed = Rtext_input_get_text(g_name);
std::string name = typed && typed[0] ? typed : "world";
if (Rcheckbox_is_checked(g_loud)) { /* uppercase */ }
Rlabel_set_text(g_preview, ("Hello, " + name + "!").c_str());
Rlabel_set_text_size(g_preview, Rslider_get_value(g_size));
```

Note there is no `bool loud;` global and no `float size;` global. Two fewer
variables, and no way for the interface and the model to disagree.

### Placeholders and callbacks worth knowing

```cpp
Rtext_input_set_placeholder(g_name, "Type a name");
Rtext_input_set_changed_callback(g_name, on_changed, nullptr);      // every keystroke
Rtext_input_set_committed_callback(g_name, on_changed, nullptr);    // Enter pressed
```

A combo box selection is an **index**, not a string:
`Rcombo_box_get_selected` returns `-1` when nothing is selected.

---

## Build

```bash
cd "Tutorials/C++"
cmake --build build --target 03-controls
```

## Run

```bash
./build/03-controls
```

Type a name — the preview follows each keystroke. Tick **Shout it** — the
preview becomes uppercase. Move the slider — the preview grows. Choose **Dark**
— the whole window changes theme, including the controls you are touching.

## Test

```bash
./build/03-controls --test
```

```
03-controls: 8 checks, 0 failures
PASS
```

The test types the way a user would:

```cpp
tutorial::click(window, g_name);          // give the input the focus
Rwindow_inject_text(window, "Ada");       // real text input events
TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_preview), "Hello, Ada!") == 0);
```

and uses geometry only where the control has no keyboard equivalent — the combo
box is opened by a click and the second row is clicked by position, using
`Rtheme_get_metric(R_METRIC_ROW_HEIGHT)` so it works with any theme.

---

## Checkpoint

* Which function tells you a checkbox changed? Which tells you *what* it holds?
* What does `Rcombo_box_get_selected` return with an empty selection?
* Why does `refresh()` read all four controls even when only one changed?
  (Answer: because it is cheap, and because it is impossible to get out of sync.)

---

## Exercises

1. Add a spin box (`R_WIDGET_SPIN_BOX`, `Rspin_box_set_range`, `Rspin_box_get_value`)
   that sets the preview's text size in steps of one.
2. Make the preview show the number of characters typed.
3. Swap the combo box for two checkboxes: *Dark* and *Light*.
4. **GitHub-flavoured challenge:** keep the two theme checkboxes mutually
   exclusive using `Rcheckbox_set_checked` from the callback of the other.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Typing does nothing | The input does not have the focus: click it first, or call `Rwidget_focus`. |
| The changed callback never fires | It was set after the value was written; writing with `*_set_*` does not fire it. |
| The combo box opens off-screen | Its list opens below the box: leave room, or put the box higher. |
| Widget contents vanish after a theme change | The tree was rebuilt instead of restyled. Apply a theme with `Rtheme_apply`; do not recreate widgets. |

**Previous:** [Step 02 — Layout](../02-layout/README.md) ·
**Next:** [Step 04 — Actions and shortcuts](../04-actions-and-shortcuts/README.md)
