"""
Raven tutorial 08 (Python) - the starting point of an application.

Everything the seven steps taught, in the usual shape of an editor:

    notes.xml     the window: header bar with the menus, the editor, a status bar
    this file     the actions the menu entries name, and the files they touch

    python3 main.py                a window
    python3 main.py notes.txt      open a file
    python3 main.py --test         no window, no GPU: checks only

This file holds its own helpers, so the folder can be copied out on its own.

Where to go from here: a toolbar firing the same actions, recent files built at
run time, a language picker, a splitter with a preview.
"""

import os
import sys

import raven as r

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
#  Helpers (the same ones the earlier steps import from ../tutorial.py)
# ---------------------------------------------------------------------------

CHECKS = 0
FAILURES = 0


def is_test():
    return "--test" in sys.argv


def check(condition, message):
    global CHECKS, FAILURES
    CHECKS += 1
    if not condition:
        FAILURES += 1
        print("  FAIL:", message)


def require(condition, message):
    if not condition:
        print("  FAIL:", message)
        r.Rshutdown()
        sys.exit(1)


def init(app_name):
    desc = r.RInitDesc()
    if is_test():
        desc.backend = r.R_BACKEND_NULL
        desc.headless = 1
    else:
        desc.backend = r.R_BACKEND_VULKAN
    if not r.Rinit_ex(desc):
        print("%s: cannot start Raven (is a Vulkan driver installed?)" % app_name)
        return False
    if is_test():
        r.Rset_animations_enabled(0)
    return True


def click(window, widget):
    x, y, width, height = r.Rwidget_get_rect(widget)
    click_at(window, x + width / 2, y + height / 2)


def click_at(window, x, y):
    r.Rwindow_inject_mouse_move(window, x, y)
    r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 1)
    r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 0)


def key(window, name, modifiers=0):
    r.Rwindow_inject_key(window, name, 1, modifiers)
    r.Rwindow_inject_key(window, name, 0, modifiers)


def finish(name):
    r.Rshutdown()
    if CHECKS:
        print("%s: %d checks, %d failures" % (name, CHECKS, FAILURES))
        print("PASS" if FAILURES == 0 else "FAIL")
    return 0 if FAILURES == 0 else 1


# ---------------------------------------------------------------------------
#  The application
# ---------------------------------------------------------------------------


class App:
    def __init__(self):
        self.window = r.R_INVALID_ID
        self.editor = r.R_INVALID_ID
        self.status = r.R_INVALID_ID
        self.position = r.R_INVALID_ID
        self.numbers = r.R_INVALID_ID
        self.path = ""
        self.modified = False
        self.ask_dialogs = not is_test()
        self.last_line = -1
        self.last_column = -1


app = App()


def base_name(path):
    return path.replace("\\", "/").rsplit("/", 1)[-1]


def update_title():
    name = base_name(app.path) if app.path else "Untitled"
    star = " *" if app.modified else ""
    r.Rwindow_set_title(app.window, "%s%s - Notes" % (name, star))


def set_status(text):
    r.Rlabel_set_text(app.status, text)


def new_document():
    r.Rtext_editor_set_text(app.editor, "")
    r.Rtext_editor_set_language(app.editor, None)
    app.path = ""
    app.modified = False
    update_title()
    set_status("New document")
    r.Rwidget_focus(app.editor)


def open_path(path):
    data = r.Rfile_read(path)
    if data is None:
        set_status("Cannot read %s" % path)
        return False
    r.Rtext_editor_set_text(app.editor, data.decode("utf-8", "replace"))
    r.Rtext_editor_set_language(app.editor, r.Rlanguage_for_file(path))
    app.path = path
    app.modified = False
    update_title()
    set_status("Opened %s" % path)
    r.Rwidget_focus(app.editor)
    return True


def save_to(path):
    content = r.Rtext_editor_get_text(app.editor) or ""
    if not r.Rfile_write(path, content.encode("utf-8")):
        set_status("Cannot write %s" % path)
        return False
    app.path = path
    app.modified = False
    r.Rtext_editor_set_language(app.editor, r.Rlanguage_for_file(path))
    update_title()
    set_status("Saved %s" % path)
    return True


def can_discard():
    if not app.modified or not app.ask_dialogs:
        return True
    return r.Rmessage_box(app.window, "Unsaved changes",
                          "The text has changes that were not saved. Discard them?",
                          r.R_MESSAGE_OK_CANCEL) == r.R_MESSAGE_RESULT_OK


def ask_open():
    desc = r.RFileDialogDesc()
    desc.parent = app.window
    desc.title = b"Open a text"
    chosen = r.Rfile_dialog_open(desc)
    if chosen:
        open_path(chosen)


def save(ask):
    if ask or not app.path:
        desc = r.RFileDialogDesc()
        desc.parent = app.window
        desc.title = b"Save the text"
        desc.file_name = base_name(app.path).encode("utf-8") if app.path else b"untitled.txt"
        desc.default_extension = b"txt"
        chosen = r.Rfile_dialog_save(desc)
        if not chosen:
            return False
        return save_to(chosen)
    return save_to(app.path)


# ---------------------------------------------------------------------------
#  Actions
# ---------------------------------------------------------------------------


def action_file(sender, action, user_data):
    if action == "file.new":
        if can_discard():
            new_document()
    elif action == "file.open":
        if can_discard():
            ask_open()
    elif action == "file.save":
        save(False)
    elif action == "file.save_as":
        save(True)
    elif action == "file.quit":
        if can_discard():
            r.Rquit(0)


def action_edit(sender, action, user_data):
    r.Rwidget_focus(app.editor)
    if action == "edit.undo":
        r.Rtext_editor_undo(app.editor)
    elif action == "edit.redo":
        r.Rtext_editor_redo(app.editor)
    elif action == "edit.select_all":
        r.Rtext_editor_select_all(app.editor)


def action_view(sender, action, user_data):
    if action == "view.numbers":
        # The menu item owns its check mark: read it, do not mirror it.
        r.Rtext_editor_set_line_numbers(app.editor, r.Rmenu_item_is_checked(sender))
    elif action == "view.wrap":
        r.Rtext_editor_set_word_wrap(app.editor, r.Rmenu_item_is_checked(sender))
    elif action == "view.dark":
        r.Rtheme_apply(r.R_THEME_DARK)
    elif action == "view.light":
        r.Rtheme_apply(r.R_THEME_LIGHT)


def on_changed(editor, user_data):
    if app.modified:
        return
    app.modified = True
    update_title()
    set_status("Modified")


def on_frame(delta, user_data):
    if not r.Rwidget_is_valid(app.editor):
        return
    line, column = r.Rtext_editor_get_cursor(app.editor)
    if line == app.last_line and column == app.last_column:
        return
    app.last_line, app.last_column = line, column
    r.Rlabel_set_text(app.position, "Line %d, column %d" % (line + 1, column + 1))


def on_close(window, user_data):
    if not can_discard():
        return 0
    r.Rquit(0)
    return 1


def on_drop(target, event, user_data):
    if event and event.phase == r.R_DROP_DROPPED and event.path_count > 0:
        path = event.paths[0].decode("utf-8", "replace")
        if can_discard():
            open_path(path)
    return 1


# ---------------------------------------------------------------------------
#  Start
# ---------------------------------------------------------------------------

WELCOME = (
    "Notes - a starter project\n\n"
    "File > Open (Ctrl+O) opens a text, File > Save (Ctrl+S) writes it.\n"
    "Drop a file on the window to open it too.\n\n"
    "TODO: add a toolbar, recent files, or a preview panel.\n"
)


def main():
    if not init("Raven Notes"):
        return 1

    for name in ("file.new", "file.open", "file.save", "file.save_as", "file.quit"):
        r.Raction_bind(name, action_file)
    for name in ("edit.undo", "edit.redo", "edit.select_all"):
        r.Raction_bind(name, action_edit)
    for name in ("view.numbers", "view.wrap", "view.dark", "view.light"):
        r.Raction_bind(name, action_view)

    # NOTES_XML overrides the file: the packaging step uses it for a build tree.
    xml_path = os.environ.get("NOTES_XML") or os.path.join(HERE, "notes.xml")
    app.window = r.Rwindow_create_from_xml_file(xml_path)
    require(app.window != r.R_INVALID_ID, "notes.xml could not be read")

    app.editor = r.Rwidget_find(app.window, "editor")
    app.status = r.Rwidget_find(app.window, "status")
    app.position = r.Rwidget_find(app.window, "position")
    app.numbers = r.Rwidget_find(app.window, "view.numbers")
    require(r.Rwidget_is_valid(app.editor), "the editor is missing from notes.xml")

    r.Rtext_editor_set_changed_callback(app.editor, on_changed)
    r.Rwindow_set_close_callback(app.window, on_close)
    r.Rwindow_set_drop_callback(app.window, on_drop)
    r.Rset_frame_callback(on_frame)

    if is_test():
        r.Rdraw_frame()

        # 1. Open a file written by the test.
        path = "/tmp/raven_tutorial_08_python.txt"
        sample = "Notes\nsecond line\n"
        check(bool(r.Rfile_write(path, sample.encode("utf-8"))), "the sample file was written")
        check(open_path(path), "the file opens")
        check(r.Rtext_editor_get_text(app.editor) == sample, "the editor holds the file")
        check(not app.modified, "opening does not modify")

        # 2. Saving with an explicit path, as File > Save does once it has a name.
        check(save_to(path), "the file saves")
        check(not app.modified, "saving clears the mark")

        # 3. Typing marks the document.
        click(app.window, app.editor)
        r.Rwindow_inject_text(app.window, "abc")
        check(app.modified, "typing sets the modified flag")

        # 4. File > New, through the action the menu entry names.
        r.Raction_fire("file.new", r.R_INVALID_ID)
        check(r.Rtext_editor_get_text(app.editor) == "", "file.new clears the editor")
        check(app.path == "", "file.new forgets the path")

        # 5. The checkable View entry drives the editor through its own state.
        r.Rmenu_item_set_checked(app.numbers, 0)
        r.Raction_fire("view.numbers", app.numbers)
        check(not r.Rmenu_item_is_checked(app.numbers), "view.numbers reads the item, not a copy")

        # 6. Ctrl+N runs the same action through the shortcut of the XML.
        r.Rtext_editor_set_text(app.editor, "text to lose")
        key(app.window, r.R_KEY_N, r.R_MOD_CTRL)
        check(r.Rtext_editor_get_text(app.editor) == "", "Ctrl+N runs file.new")

        # 7. Quit through the action leaves the loop.
        r.Raction_fire("file.quit", r.R_INVALID_ID)
        check(r.Ris_running() == 0, "file.quit ends the loop")

        os.remove(path)
        return finish("08-capstone-notes")

    # A file given on the command line, else the welcome text.
    arguments = [a for a in sys.argv[1:] if not a.startswith("-")]
    if arguments:
        open_path(arguments[0])
    else:
        r.Rtext_editor_set_text(app.editor, WELCOME)
        update_title()
        set_status("Drop a file on the window, or press Ctrl+O.")
    r.Rwidget_focus(app.editor)

    while r.Rwait_frame(-1.0):
        pass
    return finish("08-capstone-notes")


if __name__ == "__main__":
    sys.exit(main())
