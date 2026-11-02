//! Raven tutorial 04 (Rust) - actions and shortcuts.
//!
//! A button, a menu entry and a keyboard shortcut can run the same code when
//! they all name the same **action**:
//!
//!   `OnClicked="counter.add"` in XML,
//!   `Raction_fire("counter.add")` from code,
//!   `Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL as u32, "counter.add")`.
//!
//! `action_bind` gives each name a closure. Closures cannot take a `&mut` from
//! outside, so the state is an `Rc<RefCell<_>>` shared by the bindings.

mod harness;

use raven::*;
use std::cell::RefCell;
use std::rc::Rc;

const XML: &str = r#"
<Window Title="Tutorial 04 - Actions" Width="480" Height="420">
    <VBox Spacing="10" Slot.Padding="20">
        <Label Text="Actions" TextSize="22"/>
        <Label Name="count" Text="0" TextSize="40" Slot.HAlign="Center"/>

        <HBox Spacing="8" Slot.HAlign="Center">
            <Button Name="add" Text="+1" Style="Primary" OnClicked="counter.add" Tooltip="Ctrl+I"/>
            <Button Name="add_ten" Text="+10" OnClicked="counter.add_ten"/>
            <Button Name="reset" Text="Reset" OnClicked="counter.reset" Tooltip="Ctrl+0"/>
        </HBox>

        <Checkbox Name="enabled" Text="Add buttons enabled" Checked="true"
            OnChanged="counter.enabled"/>

        <Spacer Slot.Fill="1"/>
        <Label Name="status" Text="Ready." Opacity="0.7"/>
    </VBox>
</Window>"#;

fn show(count: &Rc<RefCell<i32>>, label: Rid, status: Rid, what: &str) {
    let value = *count.borrow();
    unsafe {
        Rlabel_set_text(label, cstr(&value.to_string()).as_ptr());
        Rlabel_set_text(status, cstr(&format!("Last action: {what}")).as_ptr());
    }
}

fn main() {
    if !harness::init("Raven tutorial 04") {
        std::process::exit(1);
    }

    let window = window_from_xml(XML);
    let label = widget_find(window, "count");
    let status = widget_find(window, "status");
    let add = widget_find(window, "add");
    let add_ten = widget_find(window, "add_ten");
    let enabled = widget_find(window, "enabled");

    let count = Rc::new(RefCell::new(0i32));

    for (name, step) in [("counter.add", 1), ("counter.add_ten", 10)] {
        let count = count.clone();
        action_bind(name, move |_sender, action| {
            *count.borrow_mut() += step;
            show(&count, label, status, action);
        });
    }
    {
        let count = count.clone();
        action_bind("counter.reset", move |_sender, action| {
            *count.borrow_mut() = 0;
            show(&count, label, status, action);
        });
    }

    // Enabling: the widget decides, the action only reads it.
    action_bind("counter.enabled", move |_sender, _action| unsafe {
        let on = Rcheckbox_is_checked(enabled);
        Rwidget_set_enabled(add, on);
        Rwidget_set_enabled(add_ten, on);
        Rlabel_set_text(
            status,
            cstr(if on != 0 { "Add buttons enabled." } else { "Add buttons disabled." }).as_ptr(),
        );
    });

    // Keyboard shortcuts run the same actions, wherever the focus is.
    unsafe {
        let add_name = cstr("counter.add");
        Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL as u32, add_name.as_ptr());
        let reset_name = cstr("counter.reset");
        Rwindow_register_shortcut(window, R_KEY_0, R_MOD_CTRL as u32, reset_name.as_ptr());
    }

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        harness::click(window, add);
        harness::check(*count.borrow() == 1, "the button fires counter.add");

        harness::key(window, R_KEY_I, R_MOD_CTRL);
        harness::check(*count.borrow() == 2, "Ctrl+I fires the same action");

        unsafe { Raction_fire(cstr("counter.add_ten").as_ptr(), R_INVALID_ID) };
        harness::check(*count.borrow() == 12, "an action can be fired from code");

        // A disabled widget takes no clicks...
        harness::click(window, enabled);
        harness::check(unsafe { Rcheckbox_is_checked(enabled) } == 0, "the checkbox is cleared");
        harness::click(window, add);
        harness::check(*count.borrow() == 12, "a disabled button does not fire");

        // ...but the shortcut still does: it belongs to the window, not the button.
        harness::key(window, R_KEY_I, R_MOD_CTRL);
        harness::check(*count.borrow() == 13, "the shortcut ignores the disabled button");

        harness::key(window, R_KEY_0, R_MOD_CTRL);
        harness::check(*count.borrow() == 0, "Ctrl+0 resets");
        harness::check(
            unsafe { text(Rlabel_get_text(status)) } == "Last action: counter.reset",
            "the status names the action",
        );

        std::process::exit(harness::finish("04-actions"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("04-actions"));
}
