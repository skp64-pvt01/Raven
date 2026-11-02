//! Raven tutorial 06 (Rust) - menus.
//!
//! A menu bar is XML like everything else:
//!
//!   `<MenuBar>`                 the bar (put it first in the window's VBox)
//!   `<Menu Text="File">`        a title, or a sub-menu when nested in a Menu
//!   `<MenuItem Text="New" .../>` an entry, `<Separator/>` a line
//!   `Shortcut="Ctrl+N"`         works with the menu closed, and shows in it
//!   `Checkable="true"`          an entry with a check mark
//!   `OnClicked="name"`          fires the action, same as a button
//!
//! Raven also has built-in actions that need no Rust at all: `app.quit`,
//! `theme.dark`, `theme.light`, `show:NAME`, `toggle:NAME`...

mod harness;

use raven::*;
use std::cell::RefCell;
use std::rc::Rc;

const XML: &str = r#"
<Window Title="Tutorial 06 - Menus" Width="640" Height="420">
    <VBox Spacing="0">
        <MenuBar>
            <Menu Text="File">
                <MenuItem Text="New" Shortcut="Ctrl+N" OnClicked="menu.new"/>
                <MenuItem Text="Open..." Shortcut="Ctrl+O" OnClicked="menu.open"/>
                <Separator/>
                <Menu Text="Recent">
                    <MenuItem Text="notes.txt" OnClicked="menu.recent"/>
                </Menu>
                <Separator/>
                <MenuItem Text="Quit" Shortcut="Ctrl+Q" OnClicked="app.quit"/>
            </Menu>
            <Menu Text="View">
                <MenuItem Name="view.status" Text="Show status" Shortcut="Ctrl+G"
                    Checkable="true" Checked="true" OnClicked="view.status"/>
                <Separator/>
                <MenuItem Text="Dark" OnClicked="theme.dark"/>
                <MenuItem Text="Light" OnClicked="theme.light"/>
            </Menu>
        </MenuBar>

        <Label Name="body" Slot.Fill="1" Slot.Padding="24"
            Text="Open the File menu, or press Ctrl+N, Ctrl+O, Ctrl+G."/>
        <Label Name="status" Text="Menu: none" Slot.Padding="12,0,12,12" Opacity="0.8"/>
    </VBox>
</Window>"#;

fn main() {
    if !harness::init("Raven tutorial 06") {
        std::process::exit(1);
    }

    let window = window_from_xml(XML);
    let status = widget_find(window, "status");
    let show_status = widget_find(window, "view.status");
    let last = Rc::new(RefCell::new(String::from("none")));

    for name in ["menu.new", "menu.open", "menu.recent"] {
        let last = last.clone();
        action_bind(name, move |_sender, action| unsafe {
            *last.borrow_mut() = action.to_string();
            Rlabel_set_text(status, cstr(&format!("Menu: {}", action.rsplit('.').next().unwrap_or(action))).as_ptr());
        });
    }

    // A checkable entry keeps its own state: ask it, never mirror it.
    action_bind("view.status", move |sender, _action| unsafe {
        let checked = Rmenu_item_is_checked(sender) != 0;
        Rlabel_set_text(
            status,
            cstr(if checked { "Status shown" } else { "Status hidden" }).as_ptr(),
        );
    });

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        // Geometry of the menus: the bar is one control high, its rows are
        // R_METRIC_ROW_HEIGHT. A click on a title opens it, a click below picks
        // the entry at that row.
        let bar = unsafe { Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT) };
        let row = unsafe { Rtheme_get_metric(R_METRIC_ROW_HEIGHT) };

        harness::click_at(window, 15.0, bar * 0.5);
        harness::click_at(window, 30.0, bar + 4.0 + row * 0.5);
        harness::check(*last.borrow() == "menu.new", "File > New fires menu.new");
        harness::check(
            unsafe { text(Rlabel_get_text(status)) } == "Menu: new",
            "the status label names the entry",
        );

        // The shortcut of the same menu, with the menu closed.
        harness::key(window, R_KEY_O, R_MOD_CTRL);
        harness::check(*last.borrow() == "menu.open", "Ctrl+O fires menu.open");

        // A checkable entry toggles itself, then reports it.
        harness::check(unsafe { Rmenu_item_is_checked(show_status) } != 0, "Show status starts checked");
        harness::key(window, R_KEY_G, R_MOD_CTRL);
        harness::check(unsafe { Rmenu_item_is_checked(show_status) } == 0, "Ctrl+G clears the check mark");
        harness::check(
            unsafe { text(Rlabel_get_text(status)) } == "Status hidden",
            "the entry reports its own new state",
        );

        // The built-in theme actions need no binding at all.
        harness::key(window, R_KEY_Q, R_MOD_CTRL);
        harness::check(unsafe { Ris_running() } == 0, "Ctrl+Q runs the built-in app.quit");

        std::process::exit(harness::finish("06-menus"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("06-menus"));
}
