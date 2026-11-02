# Step 07 — A text editor and files

**Level 4 · Advanced · ~35 minutes**

A multi-line editor with line numbers, undo, clipboard and syntax colours is
**one widget**. This step wires it to files and the dialogs of the system.

---

## What you will learn

### The editor

```ts
r.editorText(editor);            // string, valid until the next edit
r.editorLength(editor);          // bytes
r.editorSetText(editor, text);
r.editorSetLanguage(editor, "C");// or r.languageForFile(path)
r.editorCursor(editor);          // [line, column]
r.editorUndo(editor); r.editorRedo(editor); r.editorSelectAll(editor);
r.onEditorChanged(editor, () => { ... });
```

### Files

The dialog of the system comes from Raven; reading and writing is **Deno's own
API**, which is already UTF-8 and permission-checked:

```ts
const path = r.openFileDialog({ parent: window, title: "Open a text" });
if (path) r.editorSetText(editor, Deno.readTextFileSync(path));

Deno.writeTextFileSync(path, r.editorText(editor));
```

`r.openFileDialog` and `r.saveFileDialog` block until the user answers and
return `null` when cancelled. (`Rfile_read` / `Rfile_write` exist too; Deno's
are simpler to teach and to use.)

---

## The program

[`main.ts`](main.ts) is a minimal but complete editor:

```
VBox
├── HBox        toolbar: Open..., Save
├── TextEditor  (Fill 1, line numbers, word wrap)
└── Label       status bar
```

Four functions carry the logic, and they take a **path**, not a dialog:

| Function | Used by |
|---|---|
| `openPath(path)` | the Open dialog, the command line, the test |
| `saveTo(path)` | Save (when the name is known), Save As, the test |
| `askOpen` / `save` | the toolbar, and only in a normal run |

That split is what makes the step testable: the test calls `openPath` and
`saveTo` with a temporary file, so nothing has to click through a system dialog.

The editor reports typing through a callback:

```ts
r.onEditorChanged(editor, () => {
  if (state.modified) return;
  state.modified = true;
  updateTitle();          // adds the * to the title
  r.setText(status, "Modified");
});
```

---

## Run

```bash
deno task 07
```

Type. Press **Open...** and choose a `.ts`, `.c` or `.py` file: the language,
and its colours, come from the file name. Edit, press **Save** — the title loses
its `*`. `Ctrl+Z` undoes; `Ctrl+/` comments the selection.

## Test

```bash
deno task 07 --test
```

```
07-text-editor: 9 checks, 0 failures
PASS
```

The test writes a file with `Deno.writeTextFileSync`, opens it, checks the text
and the length, types with `r.typeText`, verifies the `modified` flag, saves, and
reads the file back to prove the content reached the disk.

---

## Checkpoint

* Why does `openPath` take a path instead of opening a dialog?
* What happens to the string from `editorText` after the next keystroke?
* Why is the callback given to `onEditorChanged` kept by the binding?
* What does `openFileDialog` return when the user cancels?

---

## Exercises

1. Add **Save As** (always a dialog) next to **Save**.
2. Show the cursor position in the status bar with `r.editorCursor` from a
   timer or a frame callback (step 08 does not, for lack of a frame callback in
   the binding — add `Rset_frame_callback` to `raven.ts` and do it).
3. Add a **New** button that clears the editor and the path.
4. Ask before discarding unsaved changes with `r.messageBox`.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `PermissionDenied` on read/write | Run with `--allow-read` / `--allow-write`; `deno task` does. |
| The dialog never returns | It is modal and blocks. Never call it from `--test`. |
| `Cannot read <path>` | The path is wrong, or the file is a directory. |
| Colour never appears | Unknown extension: `languageForFile` returns `null`. |
| The `*` never appears | The changed callback is not set, or the text was set from code. |

**Previous:** [Step 06 — Menus](../06-menus/README.md) ·
**Next:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md)
