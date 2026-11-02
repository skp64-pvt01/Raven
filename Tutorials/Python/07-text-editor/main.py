"""
Raven tutorial 07 (Python) - a text editor and files.

The editor is one widget:

    Rtext_editor_get_text / Rtext_editor_get_length   what to save
    Rtext_editor_set_text                             what to load
    Rtext_editor_set_changed_callback                 "the user typed"
    Rtext_editor_set_language                         colours from the file name

Files go through the library too: Rfile_read / Rfile_write on every platform,
and Rfile_dialog_open / Rfile_dialog_save for the dialogs of the system.

In Python, Rfile_read returns bytes (it frees the C buffer for you), so decode
before putting it in the editor.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 07 - Editor" Width="720" Height="520">
    <VBox Spacing="0">
        <HBox Spacing="6" Slot.Padding="8">
            <Button Text="Open..." OnClicked="file.open"/>
            <Button Name="save" Text="Save" OnClicked="file.save"/>
        </HBox>
        <TextEditor Name="editor" LineNumbers="true" Wrap="true" Slot.Fill="1"
            Placeholder="Type here, or press Open..."/>
        <Label Name="status" Text="Ready" Slot.Padding="10,6" Opacity="0.8"/>
    </VBox>
</Window>"""

WELCOME = (
    "Raven tutorial 07\n\n"
    "This text lives in a TextEditor: several lines,\n"
    "undo, clipboard, and colours once the file has a name.\n"
)


def main():
    if not tutorial.init("Raven tutorial 07"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    editor = r.Rwidget_find(window, "editor")
    status = r.Rwidget_find(window, "status")

    state = {"path": "", "modified": False}

    def base_name(path):
        return path.replace("\\", "/").rsplit("/", 1)[-1]

    def update_title():
        name = base_name(state["path"]) if state["path"] else "Untitled"
        star = " *" if state["modified"] else ""
        r.Rwindow_set_title(window, "%s%s - Tutorial 07" % (name, star))

    def set_status(text):
        r.Rlabel_set_text(status, text)

    def open_path(path):
        data = r.Rfile_read(path)
        if data is None:
            set_status("Cannot read %s" % path)
            return False
        r.Rtext_editor_set_text(editor, data.decode("utf-8", "replace"))
        r.Rtext_editor_set_language(editor, r.Rlanguage_for_file(path))
        state["path"] = path
        state["modified"] = False
        update_title()
        set_status("Opened %s (%d bytes)" % (path, len(data)))
        return True

    def save_to(path):
        content = r.Rtext_editor_get_text(editor) or ""
        if not r.Rfile_write(path, content.encode("utf-8")):
            set_status("Cannot write %s" % path)
            return False
        state["path"] = path
        state["modified"] = False
        r.Rtext_editor_set_language(editor, r.Rlanguage_for_file(path))
        update_title()
        set_status("Saved %s (%d bytes)" % (path, len(content)))
        return True

    def on_changed(editor_id, user_data):
        if state["modified"]:
            return
        state["modified"] = True
        update_title()
        set_status("Modified")

    r.Rtext_editor_set_changed_callback(editor, on_changed)

    def ask_open(sender, action, user_data):
        desc = r.RFileDialogDesc()
        desc.parent = window
        desc.title = b"Open a text"
        chosen = r.Rfile_dialog_open(desc)
        if chosen:
            open_path(chosen)

    def ask_save(sender, action, user_data):
        if state["path"]:
            save_to(state["path"])
            return
        desc = r.RFileDialogDesc()
        desc.parent = window
        desc.title = b"Save the text"
        desc.file_name = b"untitled.txt"
        desc.default_extension = b"txt"
        chosen = r.Rfile_dialog_save(desc)
        if chosen:
            save_to(chosen)

    r.Raction_bind("file.open", ask_open)
    r.Raction_bind("file.save", ask_save)

    r.Rtext_editor_set_text(editor, WELCOME)
    update_title()

    if tutorial.is_test():
        r.Rdraw_frame()

        # 1. A file written by the program, read back into the editor.
        path = "/tmp/raven_tutorial_07_python.txt"
        sample = "first line\nsecond line\n"
        tutorial.check(bool(r.Rfile_write(path, sample.encode("utf-8"))), "the sample file was written")
        tutorial.check(open_path(path), "the file opens")
        tutorial.check(r.Rtext_editor_get_text(editor) == sample, "the editor holds the file")
        tutorial.check(r.Rtext_editor_get_length(editor) == len(sample), "the length is right")
        tutorial.check(not state["modified"], "opening a file is not a modification")

        # 2. Typing marks the document modified.
        tutorial.click(window, editor)
        r.Rwindow_inject_text(window, "typed")
        tutorial.check(state["modified"], "typing sets the modified flag")
        tutorial.check("typed" in r.Rtext_editor_get_text(editor), "the typed text is in the editor")

        # 3. Saving writes what the editor holds, and clears the mark.
        tutorial.check(save_to(path), "the file saves")
        tutorial.check(not state["modified"], "saving clears the modified flag")
        reread = r.Rfile_read(path)
        tutorial.check(reread is not None and b"typed" in reread, "the typed text reached the disk")

        os.remove(path)
        return tutorial.finish("07-text-editor")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("07-text-editor")


if __name__ == "__main__":
    sys.exit(main())
