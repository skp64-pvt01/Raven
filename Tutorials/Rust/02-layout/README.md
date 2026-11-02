# Step 02 — Layout

**Level 1 · Beginner · ~25 minutes**

A window that looks right at any size. Panels arrange their children; each child
carries a *slot* that says how it wants to be arranged. In XML, a slot is an
attribute that starts with `Slot.` — no Rust is needed for this step.

---

## What you will learn

| XML | Meaning |
|---|---|
| `<VBox Spacing="10">` / `<HBox Spacing="8">` | stack vertically / place in a row, with a gap |
| `Slot.Padding="24"` or `"10,6,10,6"` | space around the child (all sides, or left, top, right, bottom) |
| `Slot.Fill="1"` | share of the leftover space |
| `Slot.HAlign` / `Slot.VAlign` | where a child sits in its cell when smaller |
| `<Spacer Slot.Fill="1"/>` | an empty widget that takes the slack |
| `<Separator/>` | a line |

The rule: **a panel decides, a slot asks.** Nothing sets an absolute position.

---

## The program

[`src/main.rs`](src/main.rs) holds one XML string with four bands:

```
VBox (root, Spacing 10, Padding 16)
├── Label        "Layout"
├── Label        explanation
├── HBox         three buttons, each Slot.Fill="1"   → equal thirds
├── Separator
├── HBox         growing label (Fill 1) + Fixed button
├── Spacer       Fill 1  ← takes the remaining height
└── HBox         status label (Fill 1) + Clear button
```

Delete the spacer's `Fill` and the footer rises under the buttons: the most
common layout bug, and the test below catches it.

The only Rust in the step is the `Clear` action:

```rust
action_bind("layout.clear", move |_sender, _action| {
    unsafe { Rlabel_set_text(status, cstr("Nothing pressed yet.").as_ptr()) };
});
```

Everything else — the boxes, the shares, the alignment — is in the XML.

---

## Build

```bash
cargo build -p step-02-layout
```

## Run

```bash
cargo run -p step-02-layout
```

Resize the window. The three buttons stay equal and share the width; the
**Fixed** button keeps its size; the footer stays at the bottom.

## Test

```bash
cargo run -p step-02-layout -- --test
```

```
02-layout: 7 checks, 0 failures
PASS
```

The test asks the widgets for their rectangles (`harness::rect`, which wraps
`Rwidget_get_rect`) and checks **relations**, not pixels: equal widths, left to
right, footer below the row. Relations survive a theme change; absolute numbers
would not.

One detail every layout test needs: **draw one frame first**. Before
`Rdraw_frame()` the widgets have no rectangles.

```rust
let (_, _, first_width, _) = harness::rect(first);
for index in 1..3 {
    let (x, _, width, _) = harness::rect(unsafe { Rwidget_get_child(equal, index) });
    check((width - first_width).abs() < 1.0, "equal widths");
    ...
}
```

---

## Checkpoint

* Why do the three buttons have the same width?
* Why does the label in the second row grow and the button not?
* What happens if two children have `Fill="2"` and `Fill="1"`?

---

## Exercises

1. Give the middle button `Slot.Fill="2"`. Predict the widths, then confirm with
   `harness::rect`.
2. Put twenty labels inside a `<ScrollBox><VBox>...</VBox></ScrollBox>` and
   scroll them.
3. Move the `<Spacer/>` above the separator and explain the result.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Everything collapses to zero size | The root is not a panel, or nothing asks for space. Start the window with a `VBox`. |
| A widget ignores `Fill` | Its parent is not a box, or a sibling already takes the space. |
| The footer floats in the middle | No `Spacer` with `Fill`, or it is outside the `VBox`. |
| `harness::rect` returns zeros | No `Rdraw_frame()` ran after the change. |

**Previous:** [Step 01 — First window](../01-first-window/README.md) ·
**Next:** [Step 03 — Controls](../03-controls/README.md)
