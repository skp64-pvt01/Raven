# Step 05 — An interface written in XML

**Level 3 · Intermediate · ~25 minutes**

Steps 01–04 kept the XML in a string inside `main.ts`. Past a dozen widgets that
string belongs in a file: editable without touching the code, and openable in the
designer.

---

## What you will learn

```ts
const XML_PATH = new URL("./window.xml", import.meta.url).pathname;
const window = r.windowFromXmlFile(XML_PATH);
const name = r.find(window, "name");        // a Name="..." in the file
r.setLiveReload(true);                      // reload when the file is saved
```

* `import.meta.url` is the URL of the module being run, so the XML is found next
  to `main.ts` whatever the working directory is.
* `r.find` returns `0` for a missing name; check once at start-up with
  `t.require`.
* `r.setLiveReload(true)` re-reads the file whenever it changes. Widgets keep
  their state where the file did not change.

[`window.xml`](window.xml) has the same content as the string in step 01, with
more widgets. Two rules cover the format:

* **a tag is a widget** — `<Button>`, `<TextInput>`, `<VBox>`;
* **an attribute is a property** — `Text`, `TextSize`, `Name`, and the `Slot.*`
  ones of step 02.

The full reference is `docs/XML.md` in the Raven repository.

---

## The program

TypeScript is down to the action:

```ts
r.actionBind("app.greet", () => {
  const who = r.inputText(name) || "world";
  r.setText(greeting, `Hello, ${who}!`);
});
```

`OnClicked="app.greet"` in the XML fires that action. The XML knows nothing about
TypeScript — the same file works with the C++, Rust or Python binding.

---

## Run

```bash
deno task 05
```

Type a name, press **Greet**. Then edit `window.xml` — change a title, add a
label — save, and see it applied without restarting (`setLiveReload` is on).

## Test

```bash
deno task 05 --test
```

```
05-xml-files: 2 checks, 0 failures
PASS
```

The test drives the input and the button, then fires `app.greet` directly to
prove the XML is wired to an action and not to the button:

```ts
r.actionFire("app.greet");
```

---

## Checkpoint

* Why is `new URL("./window.xml", import.meta.url)` better than `"./window.xml"`?
* What does `r.find` return for a missing name?
* What does the XML **not** contain, compared with step 03?

---

## Exercises

1. Reorder two labels in the file and reload. What survives?
2. Add `<Checkbox Name="loud" Text="Shout it"/>` and uppercase the greeting.
3. Delete `Name="greet"` from the button. Does the TypeScript still work? Why?
4. With live reload on, rename a `Name` while the program runs. What happens to
   the old handle?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `window.xml could not be read` | The file was moved away from `main.ts`; `import.meta.url` points at the source module. |
| A `Name` is not found | `Name` is case-sensitive and must be on the widget itself. |
| `OnClicked` does nothing | No `actionBind` for that name, or a typo. |
| Live reload does nothing | `r.setLiveReload(true)` is commented out — uncomment it. |

**Previous:** [Step 04 — Actions and shortcuts](../04-actions/README.md) ·
**Next:** [Step 06 — Menus](../06-menus/README.md)
