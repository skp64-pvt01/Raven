# Step 05 — An interface written in XML

**Level 3 · Intermediate · ~25 minutes**

Steps 01–04 kept the XML in a string inside `main.rs`. Once an interface has
more than a dozen widgets, that string belongs in a file of its own: editable
without recompiling, and openable in the designer.

---

## What you will learn

```rust
let xml = concat!(env!("CARGO_MANIFEST_DIR"), "/window.xml");
let window = unsafe { Rwindow_create_from_xml_file(cstr(xml).as_ptr()) };
let name = widget_find(window, "name");            // a Name="..." in the file
unsafe { Rxml_set_live_reload(1) };                // reload when the file is saved
```

* `env!("CARGO_MANIFEST_DIR")` is the folder that holds `Cargo.toml`. Baking it
  in means the binary finds the file from any working directory — the right
  habit for development, and one line to change when you ship.
* `Rwidget_find` returns `R_INVALID_ID` when the name does not exist. Check it
  once at start-up with `harness::require`.
* `Rxml_set_live_reload(1)` re-reads the file whenever it changes. Widgets keep
  their state where the file did not change.

The file itself — [`window.xml`](window.xml) — has the same content as the
string of step 01, with more widgets. Two rules cover the format:

* **a tag is a widget** — `<Button>`, `<TextInput>`, `<VBox>`;
* **an attribute is a property** — `Text`, `TextSize`, `Name`, and the `Slot.*`
  ones of step 02.

The complete reference is `docs/XML.md` in the Raven repository.

---

## The program

Rust is down to three things:

```rust
action_bind("app.greet", move |_sender, _action| unsafe {
    let typed = text(Rtext_input_get_text(name));
    let who = if typed.is_empty() { "world" } else { typed.as_str() };
    Rlabel_set_text(greeting, cstr(&format!("Hello, {who}!")).as_ptr());
});
```

`OnClicked="app.greet"` in the XML fires that action. The XML knows nothing
about Rust: the same file would work with the Python binding, or with the C API.

Nothing in the file is loaded twice, and nothing has to be rebuilt to change a
label, a spacing or a widget.

---

## Build

```bash
cargo build -p step-05-xml-files
```

## Run

```bash
cargo run -p step-05-xml-files
```

Type a name, press **Greet**. Then edit `window.xml` — change the title, add a
label — save, and (with live reload on) see it applied without restarting.

## Test

```bash
cargo run -p step-05-xml-files -- --test
```

```
05-xml-files: 2 checks, 0 failures
PASS
```

The test drives the input and the button, then fires `app.greet` directly to
prove the XML is wired to an action and not to the button:

```rust
Raction_fire(cstr("app.greet").as_ptr(), R_INVALID_ID);
```

Most of this step's safety comes from `harness::require` at start-up: if a
`Name` disappears from the file, the program says so immediately instead of
failing three calls later.

---

## Checkpoint

* Why is `env!("CARGO_MANIFEST_DIR")` better than `"./window.xml"` here?
* What does `widget_find` return for a missing name?
* What does the XML **not** contain, compared with step 03?

---

## Exercises

1. Move the `<Label>` order around in the file and reload. What survives?
2. Add `<Checkbox Name="loud" Text="Shout it"/>` and make the greeting uppercase
   — which file changes, and which function?
3. Replace the button with `<Button Text="Greet" OnClicked="app.greet"/>` and
   delete `Name="greet"`. Does the Rust still work? Why?
4. Turn on live reload, then rename a `Name` while the program runs. What does
   the program do with the old handle?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `window.xml could not be read` | The step was run from a copied binary without the XML, or `CARGO_MANIFEST_DIR` was baked at build time on another machine. Keep the file next to the source. |
| A `Name` is missing | `Name` is case-sensitive and must be on the widget, not the parent. |
| `OnClicked` does nothing | The action is not bound, or the names differ. |
| Live reload does nothing | `Rxml_set_live_reload(1)` is commented out in the source — the call is there to uncomment. |

**Previous:** [Step 04 — Actions and shortcuts](../04-actions/README.md) ·
**Next:** [Step 06 — Menus](../06-menus/README.md)
