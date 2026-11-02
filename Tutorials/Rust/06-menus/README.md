# Step 06 — Menus

**Level 3 · Intermediate · ~25 minutes**

Every desktop application has a menu bar. In Raven it is XML like everything
else, and its entries are the actions of step 04.

---

## What you will learn

```xml
<MenuBar>
    <Menu Text="File">
        <MenuItem Text="New" Shortcut="Ctrl+N" OnClicked="menu.new"/>
        <MenuItem Text="Open..." Shortcut="Ctrl+O" OnClicked="menu.open"/>
        <Separator/>
        <Menu Text="Recent">                       <!-- a sub-menu -->
            <MenuItem Text="notes.txt" OnClicked="menu.recent"/>
        </Menu>
        <Separator/>
        <MenuItem Text="Quit" Shortcut="Ctrl+Q" OnClicked="app.quit"/>
    </Menu>
</MenuBar>
```

| Attribute | Meaning |
|---|---|
| `Shortcut="Ctrl+N"` | works with the menu closed, and shows in the menu |
| `Checkable="true"` + `Checked="true"` | an entry with a check mark |
| `OnClicked="name"` | the action the entry fires |
| nested `<Menu>` | a sub-menu |

**Built-in actions need no Rust at all**: `app.quit`, `theme.dark`,
`theme.light`, `theme.toggle`, `show:NAME`, `toggle:NAME`, `page:NAME=+1`...
See *Actions* in `docs/XML.md`. `Raction_bind` takes a built-in name over.

A checkable entry keeps its own state. Raven toggles the check mark **before**
calling you, so ask the item instead of mirroring it:

```rust
action_bind("view.status", move |sender, _action| unsafe {
    let checked = Rmenu_item_is_checked(sender) != 0;
    Rlabel_set_text(status, cstr(if checked { "Status shown" } else { "Status hidden" }).as_ptr());
});
```

---

## The program

[`src/main.rs`](src/main.rs) builds the whole window from one XML string:

```
MenuBar
├── File   New (Ctrl+N) · Open… (Ctrl+O) · ─── · Recent ▸ notes.txt · ─── · Quit (Ctrl+Q)
└── View   Show status (checkable, Ctrl+G) · ─── · Dark · Light
```

Rust binds three actions and reads one check mark. `Quit`, `Dark` and `Light`
could each be a built-in (`app.quit`, `theme.dark`, `theme.light`) — the step
uses `app.quit` and lets the theme entries go through the same mechanism so you
can see both styles side by side.

---

## Build

```bash
cargo build -p step-06-menus
```

## Run

```bash
cargo run -p step-06-menus
```

* Open **File** with the mouse; entries appear below the title.
* Hover **Recent** — the sub-menu opens to the right.
* `Ctrl+O` with no menu open runs the entry anyway.
* `Ctrl+G` toggles **Show status**, and the status line stops updating.
* `Ctrl+Q` quits.

## Test

```bash
cargo run -p step-06-menus -- --test
```

```
06-menus: 7 checks, 0 failures
PASS
```

Menu entries have no stable rectangle until they open, so the test uses the
theme metrics that place them:

```rust
let bar = unsafe { Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT) };
let row = unsafe { Rtheme_get_metric(R_METRIC_ROW_HEIGHT) };
harness::click_at(window, 15.0, bar * 0.5);              // the File title
harness::click_at(window, 30.0, bar + 4.0 + row * 0.5);  // the first entry
```

Using metrics rather than pixels keeps the test valid if the theme changes.

---

## Checkpoint

* How is a sub-menu different from a menu in the bar?
* Who toggles a checkable entry: your code or Raven?
* Do shortcuts need the menu to be open?

---

## Exercises

1. Add **Edit → Select all** with `Ctrl+A`, and a stub action that prints to the
   terminal.
2. Add **File → Save** with `Ctrl+S`, disabled while there is nothing to save
   (`Rwidget_set_enabled` on the item).
3. Replace **Dark**/**Light** with the built-in `theme.toggle` and delete the
   bindings. What happens to the menu entries?
4. Add a context menu on the body label. *(Hint: `Rwidget_set_context_menu`
   needs a menu widget; building it in code is an exercise in step 08's
   territory. XML can do it with a `<Menu>` and `ContextMenu="name"` on the
   widget — check `docs/XML.md` for the exact attribute.)*

---

## Troubleshooting

| Problem | Fix |
|---|---|
| The bar has no height | It must be the first child of a `VBox`; give the rest `Slot.Fill`. |
| Entries open off-window | The menu opens below its title; near the bottom edge, move the bar. |
| A shortcut does nothing | The item must be enabled; a disabled entry ignores its shortcut. |
| Check mark and your variable disagree | You are mirroring. Read `Rmenu_item_is_checked`. |
| Menu entries show but do nothing | No `action_bind` for the name they fire. |

**Previous:** [Step 05 — XML files](../05-xml-files/README.md) ·
**Next:** [Step 07 — A text editor](../07-text-editor/README.md)
