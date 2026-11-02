# Step 05 — An interface written in XML

**Level 3 · Intermediate · ~25 minutes**

Steps 01–04 kept the XML in a string inside `main.py`. Past a dozen widgets that
string belongs in a file: editable without touching the code, and openable in the
designer.

---

## What you will learn

```python
window = r.Rwindow_create_from_xml_file(os.path.join(HERE, "window.xml"))
name = r.Rwidget_find(window, "name")          # a Name="..." in the file
r.Rxml_set_live_reload(1)                      # reload when the file is saved
```

* `HERE` is the folder of the script (`os.path.dirname(os.path.abspath(__file__))`),
  so the program finds the file from any working directory.
* `Rwidget_find` returns `R_INVALID_ID` for a missing name; check once at
  start-up with `tutorial.require`.
* `Rxml_set_live_reload(1)` re-reads the file whenever it changes. Widgets keep
  their state where the file did not change.

[`window.xml`](window.xml) has the same content as the string in step 01, with
more widgets. Two rules cover the format:

* **a tag is a widget** — `<Button>`, `<TextInput>`, `<VBox>`;
* **an attribute is a property** — `Text`, `TextSize`, `Name`, and the `Slot.*`
  ones of step 02.

The full reference is `docs/XML.md` in the Raven repository.

---

## The program

Python is down to the action:

```python
def greet(sender, action, user_data):
    who = r.Rtext_input_get_text(name) or "world"
    r.Rlabel_set_text(greeting, "Hello, %s!" % who)

r.Raction_bind("app.greet", greet)
```

`OnClicked="app.greet"` in the XML fires that action. The XML knows nothing about
Python — the same file works with the Rust binding or the C API.

---

## Run

```bash
python3 05-xml-files/main.py
```

Type a name, press **Greet**. Then edit `window.xml` — change a title, add a
label — save, and (with live reload on) see it applied without restarting.

## Test

```bash
python3 05-xml-files/main.py --test
```

```
05-xml-files: 2 checks, 0 failures
PASS
```

The test drives the input and the button, then fires `app.greet` directly, to
prove the XML is wired to an action and not to the button:

```python
r.Raction_fire("app.greet", r.R_INVALID_ID)
```

---

## Checkpoint

* Why is `os.path.abspath(__file__)` better than `"window.xml"` here?
* What does `Rwidget_find` return for a missing name?
* What does the XML **not** contain, compared with step 03?

---

## Exercises

1. Reorder two labels in the file and reload. What survives?
2. Add `<Checkbox Name="loud" Text="Shout it"/>` and uppercase the greeting.
3. Delete `Name="greet"` from the button. Does the Python still work? Why?
4. With live reload on, rename a `Name` while the program runs. What happens to
   the old handle?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `window.xml could not be read` | The file was moved away from the script. Keep them together. |
| A `Name` is not found | `Name` is case-sensitive and must be on the widget itself. |
| `OnClicked` does nothing | No `Raction_bind` for that name, or a typo. |
| Live reload does nothing | `Rxml_set_live_reload(1)` is commented out — uncomment it. |

**Previous:** [Step 04 — Actions and shortcuts](../04-actions/README.md) ·
**Next:** [Step 06 — Menus](../06-menus/README.md)
