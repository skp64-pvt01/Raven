# Step 01 — A first window

**Level 1 · Beginner · ~20 minutes**

A window with a label and two buttons; one counts, one quits. This is the
smallest complete Raven program in TypeScript.

---

## What you will learn

1. `init()` starts the library, `shutdown()` ends it.
2. A window is built from XML; `find()` gets a widget back by name.
3. `actionBind()` answers `OnClicked="..."` with a TypeScript function.
4. The frame loop belongs to your `main`.

---

## The program

Open [`main.ts`](main.ts).

### Starting the library

```ts
import * as r from "../raven.ts";

if (!t.init("Raven tutorial 01")) return 1;
```

`t.init` wraps `r.init`: a real Vulkan window normally, the **null backend**
with `--test`. The binding encodes `RInitDesc` into a 40-byte buffer — that is
what an FFI must do for a C structure.

### A window, described in XML

```ts
const window = r.windowFromXml(XML);
const label = r.find(window, "counter");
```

Every `Name="..."` in the XML can be found with `r.find`. Nothing in the XML
knows TypeScript: `OnClicked="counter.add"` fires the **action**
`counter.add`, and the program binds that name to code.

### Actions as functions

```ts
let clicks = 0;
r.actionBind("counter.add", () => {
  clicks += 1;
  r.setText(label, `Clicked ${clicks} time${clicks === 1 ? "" : "s"}`);
});
```

The closure is kept by the binding for the life of the program: it must not
depend on anything that goes out of scope. `app.quit` needs no binding — it is
one of Raven's built-in actions.

### The loop

```ts
while (r.waitFrame(-1.0)) {}
```

`waitFrame` draws one frame, then sleeps until there is input or an animation
(`-1.0` = no timeout). It returns `false` after `r.quit()`.

---

## Run

```bash
deno task 01
```

Expected: a 440×240 window. **Click me** twice → *Clicked 2 times*. **Quit**
closes it, exit code 0.

Without a display or a driver:

```bash
deno task 01 --test
```

```
01-first-window: 3 checks, 0 failures
PASS
```

The test drives the real widgets with `r.mouseMove`/`r.mouseButton` — real
clicks, no window.

---

## Checkpoint

* What does `actionBind` receive as its two arguments?
* Why can the closure not capture a variable that goes out of scope?
* Where does the program stop drawing frames?

---

## Exercises

1. Add a third button that resets the counter.
2. Change the label size to `32` with `r.setTextSize(label, 32)`.
3. Read `raven.ts` and find what `windowFromXml` does. Where does the string
   become bytes, and why does the length have to be a `bigint`?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Requires ffi access` | Run through `deno task`, or pass `--allow-ffi`. |
| `Could not open library` | `RAVEN_LIBRARY` is unset or points at a missing file. |
| The window is empty | `windowFromXml` returned `0`: the XML is malformed. |
| Clicking does nothing | The names in `OnClicked` and `actionBind` differ (case-sensitive). |

**Next:** [Step 02 — Layout](../02-layout/README.md)
