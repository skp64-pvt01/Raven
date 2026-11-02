//! Raven tutorial 05 (Rust) - an interface written in XML.
//!
//! The window exists only as `window.xml`. Rust does three things:
//!
//!   `Rwindow_create_from_xml_file`  builds it,
//!   `widget_find`                   finds a widget by its `Name`,
//!   `action_bind`                   answers `OnClicked="app.greet"`.
//!
//! `env!("CARGO_MANIFEST_DIR")` is the folder of `Cargo.toml`: the file is found
//! wherever the binary is started from.

mod harness;

use raven::*;

fn main() {
    if !harness::init("Raven tutorial 05") {
        std::process::exit(1);
    }

    let xml = concat!(env!("CARGO_MANIFEST_DIR"), "/window.xml");
    let window = unsafe { Rwindow_create_from_xml_file(cstr(xml).as_ptr()) };
    harness::require(window != R_INVALID_ID, "window.xml could not be read");

    let name = widget_find(window, "name");
    let greeting = widget_find(window, "greeting");
    harness::require(name != R_INVALID_ID && greeting != R_INVALID_ID, "a Name is missing from window.xml");

    action_bind("app.greet", move |_sender, _action| unsafe {
        let typed = text(Rtext_input_get_text(name));
        let who = if typed.is_empty() { "world" } else { typed.as_str() };
        Rlabel_set_text(greeting, cstr(&format!("Hello, {who}!")).as_ptr());
    });

    // Uncomment to read the file again every time it is saved:
    // unsafe { Rxml_set_live_reload(1) };

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        harness::click(window, name);
        let typed = cstr("Grace");
        unsafe { Rwindow_inject_text(window, typed.as_ptr()) };
        harness::click(window, widget_find(window, "greet"));
        harness::check(
            unsafe { text(Rlabel_get_text(greeting)) } == "Hello, Grace!",
            "the button fires the action the XML names",
        );

        // The XML names an action, not a function: it can be fired without the button.
        let empty = cstr("");
        unsafe {
            Rtext_input_set_text(name, empty.as_ptr());
            Raction_fire(cstr("app.greet").as_ptr(), R_INVALID_ID);
        }
        harness::check(
            unsafe { text(Rlabel_get_text(greeting)) } == "Hello, world!",
            "the action works with no widget behind it",
        );

        std::process::exit(harness::finish("05-xml-files"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("05-xml-files"));
}
