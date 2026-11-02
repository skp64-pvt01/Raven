# Step 08 — Starter project: Notes (TypeScript)

**Level 4 · Advanced · ~45 minutes**

This is the step the course was built for: a small text editor with a menu bar,
keyboard shortcuts, system file dialogs and an unsaved-changes guard. It is
deliberately plain — it is the folder you copy when you start a desktop tool.

```
File  New · Open… · Save · Save as… · Quit
Edit  Undo · Redo · Select all
View  Line numbers · Word wrap · Dark · Light
```

---

## Files

| File | Contains |
|---|---|
| [`main.ts`](main.ts) | the application: actions, files, state |
| [`notes.xml`](notes.xml) | the window: header bar, menus, editor, status bar |

Read them in this order: `notes.xml` (what the window **is**), then `main.ts`
top to bottom (what it **does**).

`main.ts` imports `../raven.ts` and `../tutorial.ts`, which is why the packaged
version (step 09) installs those two files one folder above the application.

---

## Architecture

```
notes.xml ──windowFromXmlFile──▶ window tree
                                  │
            OnClicked="file.save" ┤
                                  ▼
                     actionBind("file.save", actionFile)
                                  │
             ┌────────────────────┼────────────────────┐
             ▼                    ▼                    ▼
        newDocument()         openPath()           saveTo()
        (no dialog)           (path in)            (path in)
             │                    │                    │
             └──── dialogs only in askOpen() / save() ─┘
```

Three decisions make this a starter rather than a demo:

**1. The XML owns the layout, TypeScript owns the behaviour.** Adding a menu
entry is a one-line XML change plus a branch — no widget is created in code.

**2. Every file operation takes a path.** `openPath` and `saveTo` never open a
dialog; `askOpen` and `save` do. That is why the whole thing is testable without
a user.

**3. The state is one object.**

```ts
interface App {
  window: number; editor: number; status: number; position: number; numbers: number;
  path: string;
  modified: boolean;
  askDialogs: boolean;      // false in --test: no user to answer
  lastLine: number; lastColumn: number;
}
```

`modified` is set in exactly one place (`onEditorChanged`) and cleared in exactly
two (`newDocument`, `saveTo`).

### The unsaved-changes guard

```ts
function canDiscard(): boolean {
  if (!app.modified || !app.askDialogs) return true;
  return r.messageBox(app.window, "Unsaved changes",
    "The text has changes that were not saved. Discard them?",
    r.R_MESSAGE_OK_CANCEL) === r.R_MESSAGE_RESULT_OK;
}
```

It is called by **File > New**, **File > Open**, **File > Quit** and the window's
close button. One function, four doors.

---

## Run

```bash
deno task 08                 # welcome text
deno task 08 -- notes.txt    # open a file
```

Check by hand:

- [ ] `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Q` all work.
- [ ] After typing, the title shows a `*`.
- [ ] `Ctrl+Q` with unsaved text asks before quitting.
- [ ] Closing the window asks the same question.
- [ ] **View > Line numbers** and **Word wrap** toggle and keep their check marks.
- [ ] **View > Dark** and **Light** restyle the window.
- [ ] Opening a `.ts` or `.c` file colours it.

## Test

```bash
deno task 08 --test
```

```
08-capstone-notes: 11 checks, 0 failures
PASS
```

The test covers the model, not the pixels: open a written file, save it, type
into it, run **File > New** through the real action, drive a checkable menu entry,
run `Ctrl+N` through the XML shortcut, and quit through `file.quit`.

---

## Using this as a starting point

1. **Copy** `main.ts`, `notes.xml`, `../raven.ts` and `../tutorial.ts`.
2. **Rename the application:** the `<Window Title>` in `notes.xml` and the string
   in `t.init(...)`.
3. **Add a menu entry** — three places, in this order:
   1. `notes.xml`: `<MenuItem Text="Export…" Shortcut="Ctrl+E" OnClicked="file.export"/>`
   2. `main.ts`: `r.actionBind("file.export", actionFile)`
   3. `actionFile`: an `else if (action === "file.export")` branch.
4. **Add a panel** (a preview, a file list): put it in the `VBox` of the XML with
   `Slot.Fill`, and find it with `r.find`.
5. **Ship it:** step 09 packages exactly this application.

---

## Roadmap ideas

| Idea | Where to look |
|---|---|
| Drag and drop | `Rwindow_set_drop_callback` — add it to `raven.ts` |
| Recent files sub-menu | `Rmenu_add_menu` / `Rmenu_add_item` at run time |
| Toolbar with buttons | an `HBox` in the `HeaderBar`, same `OnClicked` names |
| Language picker | `<ComboBox Name="language" OnChanged="view.language"/>` |
| Preview pane | `<Splitter>` with two children |
| Cursor in the status bar | `Rset_frame_callback` + `r.editorCursor` |

`examples/notepad` in the Raven repository is the same shape with completion,
markers and a custom language — the next file to read.

---

## Checkpoint

* How many places must change to add a menu entry?
* Why do `openPath` and `saveTo` not open dialogs themselves?
* Which single function decides whether text may be discarded?
* Why must the closure given to `actionBind` be independent of `main()`'s locals?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `notes.xml could not be read` | It is read next to `main.ts`; keep the two together. |
| A menu entry does nothing | No `actionBind` for the name in the XML. |
| The `*` never appears | The changed callback is not set, or the text was set from code. |
| Quitting loses the text | A new exit route was added without `canDiscard`. |
| The process exits when an action runs | A callback was collected. `actionBind` keeps its own; do not build `Deno.UnsafeCallback` by hand without keeping it. |

**Previous:** [Step 07 — A text editor](../07-text-editor/README.md) ·
**Next:** [Step 09 — Packaging and installation](../09-packaging/README.md)
