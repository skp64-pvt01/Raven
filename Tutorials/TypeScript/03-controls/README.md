# Step 03 — Controls and state

**Level 2 · Elementary · ~30 minutes**

Four controls drive one preview. The habit this step installs: **the widget is
the source of truth**. You never copy a checkbox's state into a variable — you
ask the checkbox.

---

## What you will learn

| Control (XML) | Read it with |
|---|---|
| `<TextInput Name="name"/>` | `r.inputText(name)` |
| `<Checkbox Name="loud"/>` | `r.isChecked(loud)` |
| `<Slider Name="size" Min="12" Max="32"/>` | `r.sliderValue(size)` |
| `<ComboBox Name="theme" Items="Light\|Dark"/>` | `r.selectedIndex(theme)` |

And the XML wiring:

```xml
<Checkbox Name="loud" Text="Shout it" OnChanged="form.update"/>
<Slider Name="size" Min="12" Max="32" Value="20" OnChanged="form.update"/>
<ComboBox Name="theme" Items="Light|Dark" Selected="0" OnChanged="form.update"/>
<TextInput Name="name" Placeholder="..." OnChanged="form.update"/>
```

`OnChanged="name"` fires the action `name` when the value changes. A text input
fires it on **Enter** (for every keystroke, use `r.onInputChanged`).

One action can serve every control because it reads them all:

```ts
r.actionBind("form.update", () => {
  const typed = r.inputText(name);
  let who = typed.length ? typed : "world";
  if (r.isChecked(loud)) who = who.toUpperCase();
  r.setText(preview, `Hello, ${who}!`);
  r.setTextSize(preview, r.sliderValue(size));

  const chosen = r.selectedIndex(theme);
  r.applyTheme(chosen === 1 ? r.R_THEME_DARK : r.R_THEME_LIGHT);
});
```

No `loud = false`, no `size = 20`, and no way for the UI and the model to
disagree.

---

## Run

```bash
deno task 03
```

Type and press **Enter** → the preview follows. Tick **Shout it** → uppercase.
Move the slider → the preview grows. **Dark** → the window restyles.

## Test

```bash
deno task 03 --test
```

```
03-controls: 6 checks, 0 failures
PASS
```

The test types like a user — click to focus, `r.typeText`, then `R_KEY_ENTER` —
and places the combo rows with the theme metric:

```ts
const row = r.metric(r.R_METRIC_ROW_HEIGHT);
t.click(window, theme);
t.clickAt(window, x + 10, y + height + 2 + row * 1.5);
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
3. Make the preview follow typing live with `r.onInputChanged(input, fn)` — the
   handler receives `(input, text)`.
4. Add a `<Checkbox Style="Switch">` that toggles word wrap of a label.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Typing does nothing | The input needs the focus: click it, or call `r.focus`. |
| `OnChanged` never fires | Setting a value from code does not fire it. |
| The list opens off-screen | The combo list opens below the box: leave room. |
| A stale value is shown | You cached it. Read the widget again in the handler. |

**Previous:** [Step 02 — Layout](../02-layout/README.md) ·
**Next:** [Step 04 — Actions and shortcuts](../04-actions/README.md)
