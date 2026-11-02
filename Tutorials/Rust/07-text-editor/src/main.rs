//! Raven tutorial 07 (Rust) - a text editor and files.
//!
//! The editor is one widget:
//!
//!   `Rtext_editor_get_text` / `Rtext_editor_get_length`  what to save
//!   `Rtext_editor_set_text`                              what to load
//!   `Rtext_editor_set_changed_callback`                  "the user typed"
//!   `Rtext_editor_set_language`                          colours from the file name
//!
//! Files go through the library too: `Rfile_read` / `Rfile_write` on every
//! platform, and `Rfile_dialog_open` / `Rfile_dialog_save` for the dialogs of
//! the system.
//!
//! The changed callback is a C function pointer, so a small `extern "C"`
//! trampoline carries a Rust closure to it - the same idea as the binding's
//! own `action_bind`.

mod harness;

use raven::*;
use std::cell::RefCell;
use std::ffi::c_void;
use std::os::raw::c_char;
use std::rc::Rc;

const XML: &str = r#"
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
</Window>"#;

struct State {
    path: String,
    modified: bool,
}

fn base_name(path: &str) -> &str {
    path.rsplit(['/', '\\']).next().unwrap_or(path)
}

fn update_title(window: Rid, state: &Rc<RefCell<State>>) {
    let state = state.borrow();
    let name = if state.path.is_empty() { "Untitled" } else { base_name(&state.path) };
    let star = if state.modified { " *" } else { "" };
    unsafe { Rwindow_set_title(window, cstr(&format!("{name}{star} - Tutorial 07")).as_ptr()) };
}

fn set_status(status: Rid, text_value: &str) {
    unsafe { Rlabel_set_text(status, cstr(text_value).as_ptr()) };
}

fn open_path(window: Rid, editor: Rid, status: Rid, state: &Rc<RefCell<State>>, path: &str) -> bool {
    unsafe {
        let mut size = 0usize;
        let data = Rfile_read(cstr(path).as_ptr(), &mut size);
        if data.is_null() {
            set_status(status, &format!("Cannot read {path}"));
            return false;
        }
        let content = text(data);
        Rmemory_free(data as *mut c_void);

        Rtext_editor_set_text(editor, cstr(&content).as_ptr());
        Rtext_editor_set_language(editor, Rlanguage_for_file(cstr(path).as_ptr()));

        {
            let mut state = state.borrow_mut();
            state.path = path.to_string();
            state.modified = false;
        }
        update_title(window, state);
        set_status(status, &format!("Opened {path} ({size} bytes)"));
        true
    }
}

fn save_to(window: Rid, editor: Rid, status: Rid, state: &Rc<RefCell<State>>, path: &str) -> bool {
    let content = unsafe { text(Rtext_editor_get_text(editor)) };
    let ok = unsafe {
        Rfile_write(cstr(path).as_ptr(), content.as_ptr() as *const c_void, content.len())
    };
    if ok == 0 {
        set_status(status, &format!("Cannot write {path}"));
        return false;
    }

    {
        let mut state = state.borrow_mut();
        state.path = path.to_string();
        state.modified = false;
    }
    update_title(window, state);
    set_status(status, &format!("Saved {path} ({} bytes)", content.len()));
    true
}

// The closure the library calls when the user types.
unsafe extern "C" fn changed_trampoline(_editor: Rid, user_data: *mut c_void) {
    let function = &mut *(user_data as *mut Box<dyn FnMut()>);
    function();
}

fn on_editor_changed<F: FnMut() + 'static>(editor: Rid, function: F) {
    let boxed: Box<Box<dyn FnMut()>> = Box::new(Box::new(function));
    unsafe {
        Rtext_editor_set_changed_callback(editor, Some(changed_trampoline), Box::into_raw(boxed) as *mut c_void)
    };
}

/// Only in a normal run: a dialog needs a window and a user.
fn ask_open(window: Rid, editor: Rid, status: Rid, state: &Rc<RefCell<State>>) {
    unsafe {
        let mut desc: RFileDialogDesc = std::mem::zeroed();
        desc.parent = window;
        let title = cstr("Open a text");
        desc.title = title.as_ptr() as *const c_char;
        let chosen = Rfile_dialog_open(&desc);
        if !chosen.is_null() {
            let path = text(chosen);
            open_path(window, editor, status, state, &path);
        }
    }
}

fn ask_save(window: Rid, editor: Rid, status: Rid, state: &Rc<RefCell<State>>) {
    let known = state.borrow().path.clone();
    if !known.is_empty() {
        save_to(window, editor, status, state, &known);
        return;
    }
    unsafe {
        let mut desc: RFileDialogDesc = std::mem::zeroed();
        desc.parent = window;
        let title = cstr("Save the text");
        let default_name = cstr("untitled.txt");
        let extension = cstr("txt");
        desc.title = title.as_ptr() as *const c_char;
        desc.file_name = default_name.as_ptr() as *const c_char;
        desc.default_extension = extension.as_ptr() as *const c_char;
        let chosen = Rfile_dialog_save(&desc);
        if !chosen.is_null() {
            let path = text(chosen);
            save_to(window, editor, status, state, &path);
        }
    }
}

fn main() {
    if !harness::init("Raven tutorial 07") {
        std::process::exit(1);
    }

    let welcome = "Raven tutorial 07\n\nThis text lives in a TextEditor: several lines,\n\
                   undo, clipboard, and colours once the file has a name.\n";
    let window = window_from_xml(XML);
    let editor = widget_find(window, "editor");
    let status = widget_find(window, "status");
    harness::require(editor != R_INVALID_ID, "the editor is missing from the XML");
    unsafe { Rtext_editor_set_text(editor, cstr(welcome).as_ptr()) };

    let state = Rc::new(RefCell::new(State { path: String::new(), modified: false }));

    {
        let state = state.clone();
        on_editor_changed(editor, move || {
            if state.borrow().modified {
                return;
            }
            state.borrow_mut().modified = true;
            update_title(window, &state);
            set_status(status, "Modified");
        });
    }

    {
        let state = state.clone();
        action_bind("file.open", move |_sender, _action| {
            ask_open(window, editor, status, &state);
        });
    }
    {
        let state = state.clone();
        action_bind("file.save", move |_sender, _action| {
            ask_save(window, editor, status, &state);
        });
    }

    update_title(window, &state);

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        // 1. A file written by the program, read back into the editor.
        let path = "/tmp/raven_tutorial_07_rust.txt";
        let sample = "first line\nsecond line\n";
        let written = unsafe {
            Rfile_write(cstr(path).as_ptr(), sample.as_ptr() as *const c_void, sample.len())
        };
        harness::check(written != 0, "the sample file was written");
        harness::check(open_path(window, editor, status, &state, path), "the file opens");
        harness::check(
            unsafe { text(Rtext_editor_get_text(editor)) } == sample,
            "the editor holds the file",
        );
        harness::check(unsafe { Rtext_editor_get_length(editor) } as usize == sample.len(), "the length is right");
        harness::check(!state.borrow().modified, "opening a file is not a modification");

        // 2. Typing marks the document modified.
        harness::click(window, editor);
        unsafe { Rwindow_inject_text(window, cstr("typed").as_ptr()) };
        harness::check(state.borrow().modified, "typing sets the modified flag");
        harness::check(
            unsafe { text(Rtext_editor_get_text(editor)) }.contains("typed"),
            "the typed text is in the editor",
        );

        // 3. Saving writes what the editor holds, and clears the mark.
        harness::check(save_to(window, editor, status, &state, path), "the file saves");
        harness::check(!state.borrow().modified, "saving clears the modified flag");
        let mut size = 0usize;
        let reread = unsafe { Rfile_read(cstr(path).as_ptr(), &mut size) };
        harness::check(!reread.is_null(), "the file can be read again");
        if !reread.is_null() {
            harness::check(unsafe { text(reread) }.contains("typed"), "the typed text reached the disk");
            unsafe { Rmemory_free(reread as *mut c_void) };
        }
        std::fs::remove_file(path).ok();

        std::process::exit(harness::finish("07-text-editor"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("07-text-editor"));
}
