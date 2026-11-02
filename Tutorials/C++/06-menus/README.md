# Step 06 — Menus

**Level 3 · Intermediate · ~30 minutes**

Every desktop application has a menu bar. In Raven it is a widget like any
other, built from four calls, and its entries are the actions of step 04.

---

## What you will learn

| Function | Meaning |
|---|---|
| `Rwidget_create(R_WIDGET_MENU_BAR)` | the bar itself |
| `Rmenu_add_menu(bar, "File")` | a title in the bar — or a sub-menu, when added to a menu |
| `Rmenu_add_item(menu, "Open...")` | an entry |
| `Rmenu_add_separator(menu)` | a line between groups |
| `Rmenu_item_set_shortcut(item, R_KEY_O, R_MOD_CTRL)` | `Ctrl+O`, works with the menu closed |
| `Rmenu_item_set_checkable` / `set_checked` / `is_checked` | an entry with a check mark |
| `Rmenu_item_set_clicked_callback` | what the entry runs |
| `Rwidget_set_context_menu(widget, menu)` | a right-click menu |

A menu is not modal: it is an in-window popup, so it draws over the interface
and closes on `Escape`, on a click elsewhere, or on an entry.

---

## The program

[`main.cpp`](main.cpp) puts a menu bar at the top of the window:

```
MenuBar
├── File   → New (Ctrl+N), Open... (Ctrl+O), ─── , Recent ▸ notes.txt, ─── , Quit (Ctrl+Q)
└── View   → Show status (checkable, Ctrl+G), ─── , Dark, Light
```

Two things are worth reading twice.

**1. The bar is in the layout.** It is the first child of the window's `VBox`,
so it takes one control height and the rest of the interface starts below it.
Nothing special happens to it.

**2. A checkable entry keeps its own state.**

```cpp
Rmenu_item_set_checkable(show, R_TRUE);
Rmenu_item_set_checked(show, R_TRUE);
...
static void on_item(Rid item, void* user_data) {
    if (strcmp(name, "Show status") == 0) {
        Rlabel_set_text(g_status, Rmenu_item_is_checked(item) ? "Status shown" : "Status hidden");
    }
}
```

Raven toggles the check mark *before* calling you. Mirroring it into a `bool`
would create a second copy that can drift; asking the item cannot.

The right-click menu at the bottom of the file is the same code path:

```cpp
Rid context = Rwidget_create(R_WIDGET_MENU);
Rmenu_add_item(context, "Rename");
Rwidget_set_context_menu(body, context);
```

---

## Build

```bash
cd "Tutorials/C++"
cmake --build build --target 06-menus
```

## Run

```bash
./build/06-menus
```

* Open **File** with the mouse; the entries appear below the title.
* Hover **Recent** — the sub-menu opens to the right.
* Press `Ctrl+O` with no menu open: the entry runs anyway.
* Press `Ctrl+G`: **Show status** toggles, and the status line stops updating.
* Right-click the body: a context menu at the pointer.
* `Ctrl+Q` quits.

## Test

```bash
./build/06-menus --test
```

```
06-menus: 10 checks, 0 failures
PASS
```

Menu entries have no stable widget rectangle until they open, so the test uses
the theme metrics that place them:

```cpp
const float bar_h = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT);
const float row   = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
tutorial::click_at(g_window, 15.0f, bar_h * 0.5f);            // the File title
tutorial::click_at(g_window, 30.0f, bar_h + 4.0f + row * 0.5f); // first entry
```

Using metrics rather than pixels keeps the test valid if the theme changes.

---

## Checkpoint

* How is a sub-menu different from a menu in the bar? (Only by its parent.)
* Who toggles a checkable entry: your code or Raven?
* Do shortcuts need the menu to be open?

---

## Exercises

1. Add **Edit → Select all** calling `Raction_fire("edit.select_all", sender)`
   and bind it in step 07's editor — or leave it as a stub here.
2. Add **File → Save** with `Ctrl+S`, and disable it (with
   `Rwidget_set_enabled`) while there is nothing to save.
3. Build the **Recent** sub-menu at run time from a `std::vector<std::string>`.
   Which function must you call before adding entries?
4. **Accessibility check.** Open the File menu, then use `Down` and `Enter` only.
   Note in the README of your own project that this works.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| The bar has no height | It is not the first child of a `VBox`, or the parent gives it `fill = 0` and no padding. |
| Entries open off-window | The menu opens below its title; near the bottom edge, move the bar or use a popup with `Rmenu_open`. |
| A shortcut does nothing | The item must be enabled; a shortcut of a disabled entry is ignored. |
| The check mark and your `bool` disagree | You are mirroring the state. Read `Rmenu_item_is_checked` instead. |
| A click on another widget goes through the open menu | It should not; if it does, the menu is not a child of the window tree (context menus must be created as `R_WIDGET_MENU` and attached with `Rwidget_set_context_menu`). |

**Previous:** [Step 05 — XML interfaces](../05-xml-interface/README.md) ·
**Next:** [Step 07 — A text editor](../07-text-editor/README.md)
