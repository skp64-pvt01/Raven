# Step 03 — Controls and state

**Level 2 · Elementary · ~30 minutes**

Four controls drive one preview. The habit this step installs: **the widget is
the source of truth**. You never copy a checkbox's state into a variable — you
ask the checkbox.

---

## What you will learn

| Control (XML) | Read it with |
|---|---|
| `<TextInput Name="name"/>` | `r.Rtext_input_get_text(name)` → `str` |
| `<Checkbox Name="loud"/>` | `r.Rcheckbox_is_checked(loud)` → `bool` |
| `<Slider Name="size" Min="12" Max="32"/>` | `r.Rslider_get_value(size)` |
| `<ComboBox Name="theme" Items="Light\|Dark"/>` | `r.Rcombo_box_get_selected(theme)` |

And the XML wiring:

```xml
<Checkbox Name="loud" Text="Shout it" OnChanged="form.update"/>
<Slider Name="size" Min="12" Max="32" Value="20" OnChanged="form.update"/>
<ComboBox Name="theme" Items="Light|Dark" Selected="0" OnChanged="form.update"/>
<TextInput Name="name" Placeholder="..." OnChanged="form.update"/>
```

`OnChanged="name"` fires the action `name` when the value changes. A text input
fires it on **Enter** (for every keystroke, use
`Rtext_input_set_changed_callback`).

One action can serve every control because it reads them all:

```python
def update(sender, action, user_data):
    who = r.Rtext_input_get_text(name) or "world"
    if r.Rcheckbox_is_checked(loud):
        who = who.upper()
    r.Rlabel_set_text(preview, "Hello, %s!" % who)
    r.Rlabel_set_text_size(preview, r.Rslider_get_value(size))
    chosen = r.Rcombo_box_get_selected(theme)
    r.Rtheme_apply(r.R_THEME_DARK if chosen == 1 else r.R_THEME_LIGHT)
```

No `loud = False`, no `size = 20`, and no way for the UI and the model to
disagree.

---

## Run

```bash
python3 03-controls/main.py
```

Type and press **Enter** → the preview follows. Tick **Shout it** → uppercase.
Move the slider → the preview grows. **Dark** → the window restyles.

## Test

```bash
python3 03-controls/main.py --test
```

```
03-controls: 6 checks, 0 failures
PASS
```

The test types like a user — click to focus, `Rwindow_inject_text`, then
`R_KEY_ENTER` — and places the combo rows with the theme metric:

```python
row = r.Rtheme_get_metric(r.R_METRIC_ROW_HEIGHT)
tutorial.click(window, theme)
tutorial.click_at(window, x + 10, y + height + 2 + row * 1.5)
```

---

## Checkpoint

* Which XML attribute turns a control into an action?
* When does a `<TextInput>` fire it, and what do you use for every keystroke?
* Why is one handler simpler than four?

---

## Exercises

1. Add a `<SpinBox Min="1" Max="10" Value="2"/>` that multiplies the size.
2. Show the number of characters typed.
3. Make the preview follow typing live with
   `Rtext_input_set_changed_callback(editor, handler)`; the handler receives
   `(editor, user_data)`.
4. Add a `<Checkbox Style="Switch">` that toggles word wrap of a label.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Typing does nothing | The input needs the focus: click it, or call `Rwidget_focus`. |
| `OnChanged` never fires | Setting a value from code does not fire it. |
| The list opens off-screen | The combo list opens below the box: leave room. |
| A stale value is shown | You cached it. Read the widget again in the handler. |

**Previous:** [Step 02 — Layout](../02-layout/README.md) ·
**Next:** [Step 04 — Actions and shortcuts](../04-actions/README.md)
