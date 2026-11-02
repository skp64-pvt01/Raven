# Step 02 — Layout

**Level 1 · Beginner · ~25 minutes**

A window that looks right at any size. Panels arrange their children; each child
carries a *slot* that says how. In XML a slot is an attribute starting with
`Slot.` — this step needs almost no TypeScript.

---

## What you will learn

| XML | Meaning |
|---|---|
| `<VBox Spacing="10">` / `<HBox Spacing="8">` | stack, or place in a row, with a gap |
| `Slot.Padding="24"` or `"10,6,10,6"` | space around the child (all sides, or L/T/R/B) |
| `Slot.Fill="1"` | share of the leftover space |
| `Slot.HAlign` / `Slot.VAlign` | where a child sits when smaller than its cell |
| `<Spacer Slot.Fill="1"/>` | empty space that takes the slack |
| `<Separator/>` | a line |

The rule: **a panel decides, a slot asks.**

---

## The program

[`main.ts`](main.ts) holds one XML string with four bands:

```
VBox (Spacing 10, Padding 16)
├── Label      "Layout"
├── Label      explanation
├── HBox       three buttons, each Slot.Fill="1"   → equal thirds
├── Separator
├── HBox       growing label (Fill 1) + Fixed button
├── Spacer     Fill 1  ← takes the remaining height
└── HBox       status label (Fill 1) + Clear button
```

Remove the spacer's `Fill` and the footer rises under the buttons — the most
common layout bug, and the test catches it.

The only TypeScript is the Clear action:

```ts
r.actionBind("layout.clear", () => r.setText(status, "Nothing pressed yet."));
```

---

## Run

```bash
deno task 02
```

Resize: the three buttons stay equal, the fixed one keeps its size, the footer
stays at the bottom.

## Test

```bash
deno task 02 --test
```

```
02-layout: 7 checks, 0 failures
PASS
```

The test checks **relations**, not pixels:

```ts
const [x, , width] = r.rect(r.getChild(equal, index));
t.check(Math.abs(width - firstWidth) < 1, "the three buttons are equally wide");
```

Relations survive a theme change; absolute numbers would not. Draw one frame
first (`r.drawFrame`) — before it, every rect is empty.

---

## Checkpoint

* Why do the three buttons have the same width?
* Why does the label grow and the button not?
* What does `Slot.Fill="2"` next to `Slot.Fill="1"` produce?

---

## Exercises

1. Give the middle button `Slot.Fill="2"`; predict, then confirm with `r.rect`.
2. Wrap twenty labels in `<ScrollBox><VBox>...</VBox></ScrollBox>`.
3. Move the `<Spacer/>` above the separator and explain the result.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Everything is zero-sized | The root is not a panel. Start the window with a `VBox`. |
| A widget ignores `Fill` | Its parent is not a box, or a sibling takes the space. |
| The footer floats in the middle | No `Spacer` with `Fill` inside the `VBox`. |
| Rects are all zeros | No frame drawn yet: call `r.drawFrame()`. |

**Previous:** [Step 01 — First window](../01-first-window/README.md) ·
**Next:** [Step 03 — Controls](../03-controls/README.md)
