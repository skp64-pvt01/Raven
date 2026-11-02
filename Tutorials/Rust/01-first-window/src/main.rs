//! Raven tutorial 01 (Rust) - a first window.
//!
//! Three ideas only:
//!   1. `Rinit_ex` starts the library, `Rshutdown` ends it.
//!   2. A window is built from XML; `widget_find` gets a widget back by name.
//!   3. `action_bind` answers `OnClicked="..."`, and the loop is yours.

mod harness;

use raven::*;
use std::cell::RefCell;
use std::rc::Rc;

const XML: &str = r#"
<Window Title="Tutorial 01 - First window" Width="440" Height="240">
    <VBox Spacing="12" Slot.Padding="24" Slot.HAlign="Center" Slot.VAlign="Center">
        <Label Text="Hello from Rust" TextSize="24" Slot.HAlign="Center"/>
        <Label Name="counter" Text="Press the button" Slot.HAlign="Center"/>
        <HBox Spacing="8" Slot.HAlign="Center">
            <Button Name="click" Text="Click me" Style="Primary" OnClicked="counter.add"
                Tooltip="Counts the clicks"/>
            <Button Name="quit" Text="Quit" OnClicked="app.quit"/>
        </HBox>
    </VBox>
</Window>"#;

fn main() {
    if !harness::init("Raven tutorial 01") {
        std::process::exit(1);
    }

    // The window, its layout and its texts are in XML.
    let window = window_from_xml(XML);
    let label = widget_find(window, "counter");

    // The counter lives in this closure. `Rc<RefCell<_>>` is what lets the
    // closure own it and still be read from `main` below.
    let clicks = Rc::new(RefCell::new(0u32));
    let counted = clicks.clone();
    action_bind("counter.add", move |_sender, _action| {
        let mut count = counted.borrow_mut();
        *count += 1;
        let text = format!("Clicked {count} time{}", if *count == 1 { "" } else { "s" });
        unsafe { Rlabel_set_text(label, cstr(&text).as_ptr()) };
    });

    // `app.quit` is a built-in action of Raven: no binding needed.

    if harness::is_test() {
        // One frame lays the widgets out: before it, every rect is still empty.
        unsafe { Rdraw_frame() };

        let button = widget_find(window, "click");
        harness::click(window, button);
        harness::check(
            unsafe { text(Rlabel_get_text(label)) } == "Clicked 1 time",
            "the first click updates the label",
        );

        harness::click(window, button);
        harness::check(
            unsafe { text(Rlabel_get_text(label)) } == "Clicked 2 times",
            "the second click counts too",
        );

        harness::click(window, widget_find(window, "quit"));
        harness::check(unsafe { Ris_running() } == 0, "Quit ends the loop");

        std::process::exit(harness::finish("01-first-window"));
    }

    // The loop: `Rwait_frame` draws a frame, then sleeps until something happens.
    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("01-first-window"));
}
