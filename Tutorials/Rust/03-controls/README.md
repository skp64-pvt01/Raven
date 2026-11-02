# Step 03 — Controls and state

**Level 2 · Elementary · ~30 minutes**

Four controls drive one preview. The habit this step installs: **the widget is
the source of truth**. You never copy a checkbox's state into a variable — you
ask the checkbox.

---

## What you will learn

| Control (XML) | Read it with |
|---|---|
| `<TextInput Name="name"/>` | `Rtext_input_get_text` → `text(pointer)` |
| `<Checkbox Name="loud"/>` | `Rcheckbox_is_checked` (`0` / `1`) |
| `<Slider Name="size" Min="12" Max="32"/>` | `Rslider_get_value` |
| `<ComboBox Name="theme" Items="Light\|Dark"/>` | `Rcombo_box_get_selected` |

And the XML wiring:

```xml
<Checkbox Name="loud" Text="Shout it" OnChanged="form.update"/>
<Slider Name="size" Min="12" Max="32" Value="20" OnChanged="form.update"/>
<ComboBox Name="theme" Items="Light|Dark" Selected="0" OnChanged="form.update"/>
<TextInput Name="name" Placeholder="..." OnChanged="form.update"/>
```

`OnChanged="name"` fires the action `name` when the value changes. A text input
fires it on **Enter** (for every keystroke, the C API's
`Rtext_input_set_changed_callback` is the tool — see step 07 for the same
technique on the editor).

One action can serve every control, because it reads them all:

```rust
action_bind("form.update", move |_sender, _action| unsafe {
    let typed = text(Rtext_input_get_text(name));
    let mut who = if typed.is_empty() { "world".into() } else { typed };
    if Rcheckbox_is_checked(loud) != 0 { who = who.to_uppercase(); }
    Rlabel_set_text(preview, cstr(&format!("Hello, {who}!")).as_ptr());
    Rlabel_set_text_size(preview, Rslider_get_value(size));
    let chosen = Rcombo_box_get_selected(theme);
    Rtheme_apply(if chosen == 1 { R_THEME_DARK } else { R_THEME_LIGHT });
});
```

No `struct Form { loud: bool, size: f32 }`, no way for the UI and the model to
disagree.

---

## Build

```bash
cargo build -p step-03-controls
```

## Run

```bash
cargo run -p step-03-controls
```

Type a name and press **Enter** → the preview follows. Tick **Shout it** → the
preview becomes uppercase. Move the slider → the preview grows. Choose **Dark**
→ the entire window restyles.

## Test

```bash
cargo run -p step-03-controls -- --test
```

```
03-controls: 6 checks, 0 failures
PASS
```

The test types the way a user would — `harness::click` to focus, then
`Rwindow_inject_text` — and presses `R_KEY_ENTER` to fire the input's action.
For the combo box it uses the theme metric that places the rows:

```rust
let row = unsafe { Rtheme_get_metric(R_METRIC_ROW_HEIGHT) };
harness::click(window, theme);                       // open the list
harness::click_at(window, x + 10.0, y + height + 2.0 + row * 1.5);
```

That keeps the test valid if the theme changes.

---

## Checkpoint

* Which XML attribute turns a control into an action?
* When does a `<TextInput>` fire it, and what do you use for every keystroke?
* Why is reading all four controls in one action simpler than four handlers?

---

## Exercises

1. Add a `<SpinBox Name="steps" Min="1" Max="10" Value="2"/>` that multiplies
   the text size.
2. Show the number of characters typed in a second label.
3. Make the preview follow the typing **live**, using
   `Rtext_input_set_changed_callback` and a `extern "C"` trampoline (step 07
   shows the pattern).
4. Add a `<Checkbox Style="Switch">` that turns word wrap of a label on and off.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Typing does nothing | The input does not have the focus: click it first, or call `Rwidget_focus`. |
| `OnChanged` never fires | Writing a value with `*_set_*` does not fire it; only the user does. |
| The combo box opens off-screen | Its list opens below the box: leave room. |
| `BorrowMutError` | A `RefCell` borrow was held across the action. Copy the values out first. |

**Previous:** [Step 02 — Layout](../02-layout/README.md) ·
**Next:** [Step 04 — Actions and shortcuts](../04-actions/README.md)
