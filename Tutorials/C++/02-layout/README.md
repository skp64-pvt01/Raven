# Step 02 — Layout

**Level 1 · Beginner · ~25 minutes**

A window that looks right at any size. Panels arrange their children; each
child carries a *slot* that says how it wants to be arranged.

---

## What you will learn

| Function | Meaning |
|---|---|
| `Rwidget_create(R_WIDGET_VBOX / R_WIDGET_HBOX)` | a panel that stacks children vertically / places them in a row |
| `Rpanel_set_spacing` | gap between the children, set once on the panel |
| `Rslot_set_padding(child, l, t, r, b)` | space around the child, inside the panel |
| `Rslot_set_fill(child, 1.0f)` | share of the leftover space |
| `Rslot_set_halign` / `Rslot_set_valign` | where a child sits when smaller than its cell |
| `Rwidget_set_min_size` | a floor for a widget's size |
| `R_WIDGET_SPACER` | an empty widget that takes the slack |

The rule to remember: **a panel decides, a slot asks.** The panel looks at the
slots of its children and divides the space; nothing sets an absolute position.

---

## The program

[`main.cpp`](main.cpp) builds one page with four bands:

1. **Title and explanation** — plain labels in the root `VBox`.
2. **A row of equal buttons** — three buttons, each with `Rslot_set_fill(button, 1.0f)`.
   Three equal requests divide the row into three equal widths, whatever the
   window size.
3. **A row with one growing child** — the label fills, the button keeps its
   width because it has a minimum size and asks for no share.
4. **A spacer and a footer** — the spacer has `fill = 1`, so it takes *all* the
   height left over and pushes the footer to the bottom.

The nesting is the whole trick:

```
VBox (root, padding 16, spacing 10)
├── Label        "Layout"
├── Label        explanation
├── HBox         three buttons (fill 1 each)
├── Separator
├── HBox         growing label (fill 1) + Fixed button (min width 120)
├── Spacer       fill 1  ← takes the remaining height
└── HBox         status label (fill 1) + Clear button
```

If you delete the spacer's `fill`, the footer rises to sit right under the row
and the page looks broken. Try it: this is the most common layout bug.

---

## Build

```bash
cd "Tutorials/C++"
cmake -S . -B build            # once
cmake --build build --target 02-layout
```

## Run

```bash
./build/02-layout
```

Resize the window. Expected:

* the three buttons stay equal and share the width;
* the **Fixed** button never goes below 120 logical units;
* the footer stays at the bottom;
* pressing a button writes its name into the footer.

## Test

```bash
./build/02-layout --test
```

```
02-layout: 6 checks, 0 failures
PASS
```

The test asks the widgets for their rectangles (`Rwidget_get_rect`) and checks
*relations*, not pixels: the three widths are equal, the buttons are ordered
left to right, the footer is below the row. Relations survive a change of theme
metrics; absolute numbers would not.

One detail every layout test needs: **draw one frame first**. Before
`Rdraw_frame()` the widgets have no rectangles yet.

---

## Checkpoint

* Why do the three buttons of the first row have the same width?
* Why does the label in the second row grow and the button not?
* What happens if two children have `fill = 2` and `1`? (Try it.)

---

## Exercises

1. Give the middle button `Rslot_set_fill(button, 2.0f)`. Predict the widths,
   then check in the test with `Rwidget_get_rect`.
2. Replace the root `VBOX` with `R_WIDGET_SCROLL_BOX` and add twenty labels.
   The scroll box needs a child panel: `Rwidget_add_child(scroll, column)`.
3. Set `Rslot_set_padding(column, 40, 8, 40, 8)` and see how the padding and
   the alignment interact.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Everything collapses to zero width | The root is a panel with no fill and no alignment. Give the window content a `VBox` and let children use `fill`. |
| A widget ignores `fill` | Its parent is not a box or grid, or another sibling takes all the space. |
| The footer floats in the middle | No spacer above it, or the spacer has no `fill`. |
| `Rwidget_get_rect` returns zeros | No `Rdraw_frame()` was called after the last change. |

**Previous:** [Step 01 — First window](../01-first-window/README.md) ·
**Next:** [Step 03 — Controls](../03-controls/README.md)
