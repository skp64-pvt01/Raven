/**
 * Raven tutorial 07 (TypeScript) - a text editor and files.
 *
 * The editor is one widget:
 *
 *   editorSetText / editorText / editorLength   what to load and save
 *   onEditorChanged                            "the user typed"
 *   editorSetLanguage / languageForFile        colours from the file name
 *   editorUndo / editorRedo / editorSelectAll
 *
 * Files: the dialog of the system comes from Raven (openFileDialog,
 * saveFileDialog); reading and writing is Deno's own API, which is already
 * asynchronous, UTF-8 and permission-checked.
 *
 * Run:  deno task 07        Test: deno task 07 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
<Window Title="Tutorial 07 - Editor" Width="720" Height="520">
    <VBox Spacing="0">
        <HBox Spacing="6" Slot.Padding="8">
            <Button Text="Open..." OnClicked="file.open"/>
            <Button Text="Save" OnClicked="file.save"/>
        </HBox>
        <TextEditor Name="editor" LineNumbers="true" Wrap="true" Slot.Fill="1"
            Placeholder="Type here, or press Open..."/>
        <Label Name="status" Text="Ready" Slot.Padding="10,6" Opacity="0.8"/>
    </VBox>
</Window>`;

const WELCOME = `Raven tutorial 07

This text lives in a TextEditor: several lines,
undo, clipboard, and colours once the file has a name.
`;

function main(): number {
  if (!t.init("Raven tutorial 07")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const editor = r.find(window, "editor");
  const status = r.find(window, "status");

  const state = { path: "", modified: false };

  const baseName = (path: string) => path.replaceAll("\\", "/").split("/").at(-1) ?? path;

  const updateTitle = () => {
    const name = state.path ? baseName(state.path) : "Untitled";
    const star = state.modified ? " *" : "";
    r.windowSetTitle(window, `${name}${star} - Tutorial 07`);
  };

  const openPath = (path: string): boolean => {
    let content: string;
    try {
      content = Deno.readTextFileSync(path);
    } catch {
      r.setText(status, `Cannot read ${path}`);
      return false;
    }
    r.editorSetText(editor, content);
    r.editorSetLanguage(editor, r.languageForFile(path));
    state.path = path;
    state.modified = false;
    updateTitle();
    r.setText(status, `Opened ${path} (${content.length} bytes)`);
    return true;
  };

  const saveTo = (path: string): boolean => {
    const content = r.editorText(editor);
    try {
      Deno.writeTextFileSync(path, content);
    } catch {
      r.setText(status, `Cannot write ${path}`);
      return false;
    }
    state.path = path;
    state.modified = false;
    r.editorSetLanguage(editor, r.languageForFile(path));
    updateTitle();
    r.setText(status, `Saved ${path} (${content.length} bytes)`);
    return true;
  };

  r.onEditorChanged(editor, () => {
    if (state.modified) return;
    state.modified = true;
    updateTitle();
    r.setText(status, "Modified");
  });

  // Only in a normal run: a dialog needs a window and a user.
  r.actionBind("file.open", () => {
    const chosen = r.openFileDialog({ parent: window, title: "Open a text" });
    if (chosen) openPath(chosen);
  });
  r.actionBind("file.save", () => {
    if (state.path) {
      saveTo(state.path);
      return;
    }
    const chosen = r.saveFileDialog({
      parent: window,
      title: "Save the text",
      fileName: "untitled.txt",
      defaultExtension: "txt",
    });
    if (chosen) saveTo(chosen);
  });

  r.editorSetText(editor, WELCOME);
  updateTitle();

  if (t.isTest()) {
    r.drawFrame();

    // 1. A file written by the program, read back into the editor.
    const path = "/tmp/raven_tutorial_07_ts.txt";
    const sample = "first line\nsecond line\n";
    Deno.writeTextFileSync(path, sample);
    t.check(openPath(path), "the file opens");
    t.check(r.editorText(editor) === sample, "the editor holds the file");
    t.check(r.editorLength(editor) === sample.length, "the length is right");
    t.check(!state.modified, "opening a file is not a modification");

    // 2. Typing marks the document modified.
    t.click(window, editor);
    r.typeText(window, "typed");
    t.check(state.modified, "typing sets the modified flag");
    t.check(r.editorText(editor).includes("typed"), "the typed text is in the editor");

    // 3. Saving writes what the editor holds, and clears the mark.
    t.check(saveTo(path), "the file saves");
    t.check(!state.modified, "saving clears the modified flag");
    t.check(Deno.readTextFileSync(path).includes("typed"), "the typed text reached the disk");

    Deno.removeSync(path);
    return t.finish("07-text-editor");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("07-text-editor");
}

Deno.exit(main());
