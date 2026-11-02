/**
 * Raven tutorial 08 (TypeScript) - the starting point of an application.
 *
 * Everything the seven steps taught, in the usual shape of an editor:
 *
 *   notes.xml   the window: header bar with the menus, the editor, a status bar
 *   this file   the actions the menu entries name, and the files they touch
 *
 *   deno task 08                a window
 *   deno task 08 -- notes.txt   open a file
 *   deno task 08 --test         no window, no GPU: checks only
 *
 * Not here, and worth adding: dragging a file onto the window. The library
 * reports it through Rwindow_set_drop_callback; the binding does not expose it
 * yet, so it is an exercise in raven.ts.
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML_PATH = new URL("./notes.xml", import.meta.url).pathname;

interface App {
  window: number;
  editor: number;
  status: number;
  position: number;
  numbers: number;
  path: string;
  modified: boolean;
  askDialogs: boolean;
  lastLine: number;
  lastColumn: number;
}

const app: App = {
  window: r.INVALID_ID,
  editor: r.INVALID_ID,
  status: r.INVALID_ID,
  position: r.INVALID_ID,
  numbers: r.INVALID_ID,
  path: "",
  modified: false,
  askDialogs: !t.isTest(),
  lastLine: -1,
  lastColumn: -1,
};

const baseName = (path: string) => path.replaceAll("\\", "/").split("/").at(-1) ?? path;

function updateTitle(): void {
  const name = app.path ? baseName(app.path) : "Untitled";
  r.windowSetTitle(app.window, `${name}${app.modified ? " *" : ""} - Notes`);
}

function setStatus(text: string): void {
  r.setText(app.status, text);
}

function newDocument(): void {
  r.editorSetText(app.editor, "");
  r.editorSetLanguage(app.editor, null);
  app.path = "";
  app.modified = false;
  updateTitle();
  setStatus("New document");
  r.focus(app.editor);
}

function openPath(path: string): boolean {
  let content: string;
  try {
    content = Deno.readTextFileSync(path);
  } catch {
    setStatus(`Cannot read ${path}`);
    return false;
  }
  r.editorSetText(app.editor, content);
  r.editorSetLanguage(app.editor, r.languageForFile(path));
  app.path = path;
  app.modified = false;
  updateTitle();
  setStatus(`Opened ${path}`);
  r.focus(app.editor);
  return true;
}

function saveTo(path: string): boolean {
  try {
    Deno.writeTextFileSync(path, r.editorText(app.editor));
  } catch {
    setStatus(`Cannot write ${path}`);
    return false;
  }
  app.path = path;
  app.modified = false;
  r.editorSetLanguage(app.editor, r.languageForFile(path));
  updateTitle();
  setStatus(`Saved ${path}`);
  return true;
}

/** True when the text can be replaced: nothing to lose, or the user agreed. */
function canDiscard(): boolean {
  if (!app.modified || !app.askDialogs) return true;
  return r.messageBox(
    app.window,
    "Unsaved changes",
    "The text has changes that were not saved. Discard them?",
    r.R_MESSAGE_OK_CANCEL,
  ) === r.R_MESSAGE_RESULT_OK;
}

function askOpen(): void {
  const chosen = r.openFileDialog({ parent: app.window, title: "Open a text" });
  if (chosen) openPath(chosen);
}

function save(ask: boolean): boolean {
  if (ask || !app.path) {
    const chosen = r.saveFileDialog({
      parent: app.window,
      title: "Save the text",
      fileName: app.path ? baseName(app.path) : "untitled.txt",
      defaultExtension: "txt",
    });
    if (!chosen) return false;
    return saveTo(chosen);
  }
  return saveTo(app.path);
}

/* --- the actions notes.xml names ----------------------------------------- */

function actionFile(_sender: number, action: string): void {
  if (action === "file.new") {
    if (canDiscard()) newDocument();
  } else if (action === "file.open") {
    if (canDiscard()) askOpen();
  } else if (action === "file.save") {
    save(false);
  } else if (action === "file.save_as") {
    save(true);
  } else if (action === "file.quit") {
    if (canDiscard()) r.quit(0);
  }
}

function actionEdit(_sender: number, action: string): void {
  r.focus(app.editor);
  if (action === "edit.undo") r.editorUndo(app.editor);
  else if (action === "edit.redo") r.editorRedo(app.editor);
  else if (action === "edit.select_all") r.editorSelectAll(app.editor);
}

function actionView(sender: number, action: string): void {
  if (action === "view.numbers") {
    // The menu item owns its check mark: read it, do not mirror it.
    r.editorSetLineNumbers(app.editor, r.menuItemIsChecked(sender));
  } else if (action === "view.wrap") {
    r.editorSetWordWrap(app.editor, r.menuItemIsChecked(sender));
  } else if (action === "view.dark") {
    r.applyTheme(r.R_THEME_DARK);
  } else if (action === "view.light") {
    r.applyTheme(r.R_THEME_LIGHT);
  }
}

/* --- start ---------------------------------------------------------------- */

function main(): number {
  if (!t.init("Raven Notes (TypeScript)")) return 1;

  for (const name of ["file.new", "file.open", "file.save", "file.save_as", "file.quit"]) {
    r.actionBind(name, actionFile);
  }
  for (const name of ["edit.undo", "edit.redo", "edit.select_all"]) r.actionBind(name, actionEdit);
  for (const name of ["view.numbers", "view.wrap", "view.dark", "view.light"]) {
    r.actionBind(name, actionView);
  }

  app.window = r.windowFromXmlFile(XML_PATH);
  t.require(app.window !== r.INVALID_ID, "notes.xml could not be read");

  app.editor = r.find(app.window, "editor");
  app.status = r.find(app.window, "status");
  app.position = r.find(app.window, "position");
  app.numbers = r.find(app.window, "view.numbers");
  t.require(r.isValid(app.editor), "the editor is missing from notes.xml");

  r.onEditorChanged(app.editor, () => {
    if (app.modified) return;
    app.modified = true;
    updateTitle();
    setStatus("Modified");
  });

  // The close button of the header bar asks the same question as File > Quit.
  r.windowSetCloseCallback(app.window, () => {
    if (!canDiscard()) return false;
    r.quit(0);
    return true;
  });

  if (t.isTest()) {
    r.drawFrame();

    // 1. Open a file written by the test.
    const path = "/tmp/raven_tutorial_08_ts.txt";
    const sample = "Notes\nsecond line\n";
    Deno.writeTextFileSync(path, sample);
    t.check(openPath(path), "the file opens");
    t.check(r.editorText(app.editor) === sample, "the editor holds the file");
    t.check(!app.modified, "opening does not modify");

    // 2. Saving with an explicit path, as File > Save does once it has a name.
    t.check(saveTo(path), "the file saves");
    t.check(!app.modified, "saving clears the mark");

    // 3. Typing marks the document.
    t.click(app.window, app.editor);
    r.typeText(app.window, "abc");
    t.check(app.modified, "typing sets the modified flag");

    // 4. File > New, through the action the menu entry names.
    r.actionFire("file.new");
    t.check(r.editorText(app.editor) === "", "file.new clears the editor");
    t.check(app.path === "", "file.new forgets the path");

    // 5. The checkable View entry drives the editor through its own state.
    r.menuItemSetChecked(app.numbers, false);
    r.actionFire("view.numbers", app.numbers);
    t.check(!r.menuItemIsChecked(app.numbers), "view.numbers reads the item, not a copy");

    // 6. Ctrl+N runs the same action through the shortcut of the XML.
    r.editorSetText(app.editor, "text to lose");
    t.key(app.window, r.R_KEY_N, r.R_MOD_CTRL);
    t.check(r.editorText(app.editor) === "", "Ctrl+N runs file.new");

    // 7. Quit through the action leaves the loop.
    r.actionFire("file.quit");
    t.check(!r.isRunning(), "file.quit ends the loop");

    Deno.removeSync(path);
    return t.finish("08-capstone-notes");
  }

  // A file given on the command line, else the welcome text.
  const argument = Deno.args.find((value) => !value.startsWith("-"));
  if (argument) {
    openPath(argument);
  } else {
    r.editorSetText(app.editor, `Notes - a starter project

File > Open (Ctrl+O) opens a text, File > Save (Ctrl+S) writes it.

TODO: add a toolbar, recent files, or a preview panel.
`);
    updateTitle();
    setStatus("Press Ctrl+O to open a file.");
  }
  r.focus(app.editor);

  while (r.waitFrame(-1.0)) {}
  return t.finish("08-capstone-notes");
}

Deno.exit(main());
