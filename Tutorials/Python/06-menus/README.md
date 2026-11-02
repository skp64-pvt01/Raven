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
        <Menu Text="Recent">                        <!-- a sub-menu -->
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
| a nested `<Menu>` | a sub-menu |

**Built-in actions need no Python**: `app.quit`, `theme.dark`, `theme.light`,
`theme.toggle`, `show:NAME`, `toggle:NAME`, `page:NAME=+1`... A bound action
takes over a built-in name.

A checkable entry keeps its own state. Raven toggles it **before** calling you,
so ask the item instead of mirroring it:

```python
def on_show_status(sender, action, user_data):
    checked = r.Rmenu_item_is_checked(sender)
    r.Rlabel_set_text(status, "Status shown" if checked else "Status hidden")
```

---

## The program

[`main.py`](main.py) builds the window from one XML string:

```
MenuBar
├── File   New (Ctrl+N) · Open… (Ctrl+O) · ─── · Recent ▸ notes.txt · ─── · Quit (Ctrl+Q)
└── View   Show status (checkable, Ctrl+G) · ─── · Dark · Light
```

Python binds three actions and reads one check mark. **Quit** uses the built-in
`app.quit`; **Dark** and **Light** use the built-in `theme.dark` / `theme.light`
— no handler at all.

---

## Run

```bash
python3 06-menus/main.py
```

* Open **File**; entries appear below the title.
* Hover **Recent** — the sub-menu opens to the right.
* `Ctrl+O` with no menu open runs the entry anyway.
* `Ctrl+G` toggles **Show status**; the status line stops updating.
* `Ctrl+Q` quits.

## Test

```bash
python3 06-menus/main.py --test
```

```
06-menus: 7 checks, 0 failures
PASS
```

Menu entries have no stable rectangle until they open, so the test uses the
theme metrics that place them:

```python
bar = r.Rtheme_get_metric(r.R_METRIC_CONTROL_HEIGHT)
row = r.Rtheme_get_metric(r.R_METRIC_ROW_HEIGHT)
tutorial.click_at(window, 15, bar * 0.5)              # the File title
tutorial.click_at(window, 30, bar + 4 + row * 0.5)    # the first entry
```

Metrics rather than pixels keep the test valid if the theme changes.

---

## Checkpoint

* How is a sub-menu different from a menu in the bar?
* Who toggles a checkable entry: your code or Raven?
* Do shortcuts need the menu to be open?

---

## Exercises

1. Add **Edit → Select all** with `Ctrl+A` and a handler that calls
   `Rtext_editor_select_all` (step 07 has an editor).
2. Add **File → Save** with `Ctrl+S`, disabled while there is nothing to save.
3. Replace **Dark**/**Light** with a single `theme.toggle` entry and delete the
   handlers.
4. Add a right-click menu on the body label with `Rwidget_set_context_menu`.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| The bar has no height | It must be the first child of a `VBox`; give the rest `Slot.Fill`. |
| Entries open off-window | The menu opens below its title; move the bar away from the bottom edge. |
| A shortcut does nothing | The item must be enabled. |
| Check mark and your variable disagree | You are mirroring. Read `Rmenu_item_is_checked`. |
| Entries do nothing | No `Raction_bind` for the name they fire. |

**Previous:** [Step 05 — XML files](../05-xml-files/README.md) ·
**Next:** [Step 07 — A text editor](../07-text-editor/README.md)
