//! Raven tutorial 08 (Rust) - the starting point of an application.
//!
//! Everything the seven steps taught, in the usual shape of an editor:
//!
//!   notes.xml     the window: header bar with the menus, the editor, a status bar
//!   this file     the actions the menu entries name, and the files they touch
//!
//!   cargo run                     a window
//!   cargo run -- notes.txt        open a file
//!   cargo run -- --test           no window, no GPU: checks only
//!
//! Where to go from here: a toolbar firing the same actions, recent files built
//! at run time, a language picker, a splitter with a preview.

mod harness;

use raven::*;
use std::cell::RefCell;
use std::ffi::c_void;
use std::os::raw::{c_char, c_int};
use std::rc::Rc;

/// Where notes.xml lives: `NOTES_XML`, then the data folder compiled in by the
/// packaging step, then next to the crate (a plain `cargo run`).
fn xml_path() -> std::path::PathBuf {
    if let Some(path) = std::env::var_os("NOTES_XML") {
        return path.into();
    }
    let folder = option_env!("NOTES_DATA_DIR").unwrap_or(env!("CARGO_MANIFEST_DIR"));
    std::path::PathBuf::from(folder).join("notes.xml")
}

struct App {
    window: Rid,
    editor: Rid,
    status: Rid,
    position: Rid,
    numbers: Rid,
    path: String,
    modified: bool,
    ask_dialogs: bool,
    last_line: i32,
    last_column: i32,
}

type Shared = Rc<RefCell<App>>;

/* ---------------------------------------------------------------------------
 *  Title, status
 * ------------------------------------------------------------------------- */

fn base_name(path: &str) -> &str {
    path.rsplit(['/', '\\']).next().unwrap_or(path)
}

fn update_title(app: &App) {
    let name = if app.path.is_empty() { "Untitled" } else { base_name(&app.path) };
    let star = if app.modified { " *" } else { "" };
    unsafe { Rwindow_set_title(app.window, cstr(&format!("{name}{star} - Notes")).as_ptr()) };
}

fn set_status(app: &App, text_value: &str) {
    unsafe { Rlabel_set_text(app.status, cstr(text_value).as_ptr()) };
}

/* ---------------------------------------------------------------------------
 *  Files: plain functions, so a test can call them with no dialog
 * ------------------------------------------------------------------------- */

fn new_document(shared: &Shared) {
    let mut app = shared.borrow_mut();
    unsafe {
        let empty = cstr("");
        Rtext_editor_set_text(app.editor, empty.as_ptr());
        Rtext_editor_set_language(app.editor, std::ptr::null());
        Rwidget_focus(app.editor);
    }
    app.path.clear();
    app.modified = false;
    update_title(&app);
    set_status(&app, "New document");
}

fn open_path(shared: &Shared, path: &str) -> bool {
    let editor = shared.borrow().editor;

    let mut size = 0usize;
    let data = unsafe { Rfile_read(cstr(path).as_ptr(), &mut size) };
    if data.is_null() {
        let app = shared.borrow();
        set_status(&app, &format!("Cannot read {path}"));
        return false;
    }
    let content = unsafe { text(data) };
    unsafe { Rmemory_free(data as *mut c_void) };

    let mut app = shared.borrow_mut();
    unsafe {
        Rtext_editor_set_text(editor, cstr(&content).as_ptr());
        Rtext_editor_set_language(editor, Rlanguage_for_file(cstr(path).as_ptr()));
        Rwidget_focus(editor);
    }
    app.path = path.to_string();
    app.modified = false;
    update_title(&app);
    set_status(&app, &format!("Opened {path}"));
    true
}

fn save_to(shared: &Shared, path: &str) -> bool {
    let editor = shared.borrow().editor;
    let content = unsafe { text(Rtext_editor_get_text(editor)) };
    let written = unsafe {
        Rfile_write(cstr(path).as_ptr(), content.as_ptr() as *const c_void, content.len())
    };

    let mut app = shared.borrow_mut();
    if written == 0 {
        set_status(&app, &format!("Cannot write {path}"));
        return false;
    }
    app.path = path.to_string();
    app.modified = false;
    update_title(&app);
    set_status(&app, &format!("Saved {path}"));
    true
}

/// R_TRUE when the text can be replaced: nothing to lose, or the user agreed.
fn can_discard(shared: &Shared) -> bool {
    // Copy what the dialog needs: never hold a borrow across a modal dialog.
    let (window, modified, ask) = {
        let app = shared.borrow();
        (app.window, app.modified, app.ask_dialogs)
    };
    if !modified || !ask {
        return true;
    }
    unsafe {
        Rmessage_box(
            window,
            cstr("Unsaved changes").as_ptr(),
            cstr("The text has changes that were not saved. Discard them?").as_ptr(),
            R_MESSAGE_OK_CANCEL,
        ) == R_MESSAGE_RESULT_OK
    }
}

/// Only in a normal run: a dialog needs a window and a user.
fn ask_open(shared: &Shared) {
    let window = shared.borrow().window;
    unsafe {
        let mut desc: RFileDialogDesc = std::mem::zeroed();
        desc.parent = window;
        let title = cstr("Open a text");
        desc.title = title.as_ptr() as *const c_char;
        let chosen = Rfile_dialog_open(&desc);
        if !chosen.is_null() {
            let path = text(chosen);
            open_path(shared, &path);
        }
    }
}

fn save(shared: &Shared, ask: bool) {
    let (window, known) = {
        let app = shared.borrow();
        (app.window, app.path.clone())
    };
    if !ask && !known.is_empty() {
        save_to(shared, &known);
        return;
    }
    unsafe {
        let mut desc: RFileDialogDesc = std::mem::zeroed();
        desc.parent = window;
        let title = cstr("Save the text");
        let file_name = cstr(if known.is_empty() { "untitled.txt" } else { base_name(&known) });
        let extension = cstr("txt");
        desc.title = title.as_ptr() as *const c_char;
        desc.file_name = file_name.as_ptr() as *const c_char;
        desc.default_extension = extension.as_ptr() as *const c_char;
        let chosen = Rfile_dialog_save(&desc);
        if !chosen.is_null() {
            let path = text(chosen);
            save_to(shared, &path);
        }
    }
}

/* ---------------------------------------------------------------------------
 *  C callbacks: a trampoline per signature, carrying a Rust closure
 * ------------------------------------------------------------------------- */

unsafe extern "C" fn changed_trampoline(_editor: Rid, user_data: *mut c_void) {
    (*(user_data as *mut Box<dyn FnMut()>))();
}

unsafe extern "C" fn frame_trampoline(_delta: f64, user_data: *mut c_void) {
    (*(user_data as *mut Box<dyn FnMut()>))();
}

unsafe extern "C" fn close_trampoline(_window: Rid, user_data: *mut c_void) -> c_int {
    if (*(user_data as *mut Box<dyn FnMut() -> bool>))() { 1 } else { 0 }
}

unsafe extern "C" fn drop_trampoline(_target: Rid, event: *const RDropEvent, user_data: *mut c_void) -> c_int {
    if (*(user_data as *mut Box<dyn FnMut(&RDropEvent) -> bool>))(&*event) { 1 } else { 0 }
}

fn keep(function: Box<dyn FnMut()>) -> *mut c_void {
    Box::into_raw(Box::new(function)) as *mut c_void
}

fn keep_bool(function: Box<dyn FnMut() -> bool>) -> *mut c_void {
    Box::into_raw(Box::new(function)) as *mut c_void
}

fn keep_drop(function: Box<dyn FnMut(&RDropEvent) -> bool>) -> *mut c_void {
    Box::into_raw(Box::new(function)) as *mut c_void
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

fn main() {
    let test = harness::is_test();
    if !harness::init("Raven Notes (Rust)") {
        std::process::exit(1);
    }

    let xml = xml_path();
    let window = unsafe { Rwindow_create_from_xml_file(cstr(&xml.to_string_lossy()).as_ptr()) };
    harness::require(window != R_INVALID_ID, "notes.xml could not be read");

    let editor = widget_find(window, "editor");
    let status = widget_find(window, "status");
    let position = widget_find(window, "position");
    let numbers = widget_find(window, "view.numbers");
    harness::require(editor != R_INVALID_ID, "the editor is missing from notes.xml");

    let state: Shared = Rc::new(RefCell::new(App {
        window,
        editor,
        status,
        position,
        numbers,
        path: String::new(),
        modified: false,
        ask_dialogs: !test,
        last_line: -1,
        last_column: -1,
    }));

    // --- File -------------------------------------------------------------
    for (name, ask) in [("file.save", false), ("file.save_as", true)] {
        let state = state.clone();
        action_bind(name, move |_sender, _action| save(&state, ask));
    }
    {
        let state = state.clone();
        action_bind("file.new", move |_sender, _action| {
            if can_discard(&state) {
                new_document(&state);
            }
        });
    }
    {
        let state = state.clone();
        action_bind("file.open", move |_sender, _action| {
            if can_discard(&state) {
                ask_open(&state);
            }
        });
    }
    {
        let state = state.clone();
        action_bind("file.quit", move |_sender, _action| {
            if can_discard(&state) {
                unsafe { Rquit(0) };
            }
        });
    }

    // --- Edit -------------------------------------------------------------
    {
        let state = state.clone();
        action_bind("edit.undo", move |_sender, _action| unsafe {
            let editor = state.borrow().editor;
            Rtext_editor_undo(editor);
        });
    }
    {
        let state = state.clone();
        action_bind("edit.redo", move |_sender, _action| unsafe {
            let editor = state.borrow().editor;
            Rtext_editor_redo(editor);
        });
    }
    {
        let state = state.clone();
        action_bind("edit.select_all", move |_sender, _action| unsafe {
            let editor = state.borrow().editor;
            Rtext_editor_select_all(editor);
        });
    }

    // --- View -------------------------------------------------------------
    {
        let state = state.clone();
        action_bind("view.numbers", move |sender, _action| unsafe {
            let editor = state.borrow().editor;
            Rtext_editor_set_line_numbers(editor, Rmenu_item_is_checked(sender));
        });
    }
    {
        let state = state.clone();
        action_bind("view.wrap", move |sender, _action| unsafe {
            let editor = state.borrow().editor;
            Rtext_editor_set_word_wrap(editor, Rmenu_item_is_checked(sender));
        });
    }
    action_bind("view.dark", |_sender, _action| unsafe { Rtheme_apply(R_THEME_DARK) });
    action_bind("view.light", |_sender, _action| unsafe { Rtheme_apply(R_THEME_LIGHT) });

    // --- Events -----------------------------------------------------------
    {
        let state = state.clone();
        let pointer = keep(Box::new(move || {
            let mut app = state.borrow_mut();
            if app.modified {
                return;
            }
            app.modified = true;
            update_title(&app);
            set_status(&app, "Modified");
        }));
        unsafe { Rtext_editor_set_changed_callback(editor, Some(changed_trampoline), pointer) };
    }
    {
        let state = state.clone();
        let pointer = keep(Box::new(move || {
            let mut app = state.borrow_mut();
            if unsafe { Rwidget_is_valid(app.editor) } == 0 {
                return;
            }
            let (mut line, mut column) = (0i32, 0i32);
            unsafe { Rtext_editor_get_cursor(app.editor, &mut line, &mut column) };
            if line == app.last_line && column == app.last_column {
                return;
            }
            app.last_line = line;
            app.last_column = column;
            let text_value = format!("Line {}, column {}", line + 1, column + 1);
            unsafe { Rlabel_set_text(app.position, cstr(&text_value).as_ptr()) };
        }));
        unsafe { Rset_frame_callback(Some(frame_trampoline), pointer) };
    }
    {
        let state = state.clone();
        let pointer = keep_bool(Box::new(move || {
            if can_discard(&state) {
                unsafe { Rquit(0) };
                return true;
            }
            false
        }));
        unsafe { Rwindow_set_close_callback(window, Some(close_trampoline), pointer) };
    }
    {
        let state = state.clone();
        let pointer = keep_drop(Box::new(move |event: &RDropEvent| {
            if event.phase == R_DROP_DROPPED && event.path_count > 0 {
                let path = unsafe { text(*event.paths) };
                if can_discard(&state) {
                    open_path(&state, &path);
                }
            }
            true
        }));
        unsafe { Rwindow_set_drop_callback(window, Some(drop_trampoline), pointer) };
    }

    state.borrow_mut().modified = false;
    {
        let app = state.borrow();
        update_title(&app);
    }

    if test {
        unsafe { Rdraw_frame() };

        // 1. Open a file written by the test.
        let path = "/tmp/raven_tutorial_08_rust.txt";
        let sample = "Notes\nsecond line\n";
        let written = unsafe {
            Rfile_write(cstr(path).as_ptr(), sample.as_ptr() as *const c_void, sample.len())
        };
        harness::check(written != 0, "the sample file was written");
        harness::check(open_path(&state, path), "the file opens");
        harness::check(
            unsafe { text(Rtext_editor_get_text(editor)) } == sample,
            "the editor holds the file",
        );
        harness::check(!state.borrow().modified, "opening does not modify");

        // 2. Saving with an explicit path, as File > Save does once it has a name.
        harness::check(save_to(&state, path), "the file saves");
        harness::check(!state.borrow().modified, "saving clears the mark");

        // 3. Typing marks the document.
        harness::click(window, editor);
        unsafe { Rwindow_inject_text(window, cstr("abc").as_ptr()) };
        harness::check(state.borrow().modified, "typing sets the modified flag");

        // 4. File > New, through the action the menu entry names.
        unsafe { Raction_fire(cstr("file.new").as_ptr(), R_INVALID_ID) };
        harness::check(
            unsafe { text(Rtext_editor_get_text(editor)) }.is_empty(),
            "file.new clears the editor",
        );
        harness::check(state.borrow().path.is_empty(), "file.new forgets the path");

        // 5. The checkable View entry drives the editor through its own state.
        let numbers = state.borrow().numbers;
        unsafe {
            Rmenu_item_set_checked(numbers, 0);
            Raction_fire(cstr("view.numbers").as_ptr(), numbers);
        }
        harness::check(unsafe { Rmenu_item_is_checked(numbers) } == 0, "view.numbers reads the item, not a copy");

        // 6. Ctrl+N runs the same action through the shortcut of the XML.
        unsafe { Rtext_editor_set_text(editor, cstr("text to lose").as_ptr()) };
        harness::key(window, R_KEY_N, R_MOD_CTRL);
        harness::check(
            unsafe { text(Rtext_editor_get_text(editor)) }.is_empty(),
            "Ctrl+N runs file.new",
        );

        // 7. Quit through the action leaves the loop.
        unsafe { Raction_fire(cstr("file.quit").as_ptr(), R_INVALID_ID) };
        harness::check(unsafe { Ris_running() } == 0, "file.quit ends the loop");

        std::fs::remove_file(path).ok();
        std::process::exit(harness::finish("08-capstone-notes"));
    }

    // A file given on the command line, else the welcome text.
    let argument = std::env::args().nth(1);
    match argument {
        Some(path) if !path.starts_with('-') => {
            open_path(&state, &path);
        }
        _ => unsafe {
            Rtext_editor_set_text(editor, cstr(
                "Notes - a starter project\n\n\
                 File > Open (Ctrl+O) opens a text, File > Save (Ctrl+S) writes it.\n\
                 Drop a file on the window to open it too.\n\n\
                 TODO: add a toolbar, recent files, or a preview panel.\n").as_ptr());
            set_status(&state.borrow(), "Drop a file on the window, or press Ctrl+O.");
            Rwidget_focus(editor);
        },
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("08-capstone-notes"));
}
