/**
 * Raven tutorial 06 (TypeScript) - menus.
 *
 * A menu bar is XML like everything else:
 *
 *   <MenuBar>                     the bar (first child of the window's VBox)
 *   <Menu Text="File">            a title, or a sub-menu when nested in a Menu
 *   <MenuItem Text="New" .../>    an entry; <Separator/> a line
 *   Shortcut="Ctrl+N"             works with the menu closed, and shows in it
 *   Checkable="true"              an entry with a check mark
 *   OnClicked="name"              fires the action, same as a button
 *
 * Raven also has built-in actions that need no TypeScript at all: app.quit,
 * theme.dark, theme.light, show:NAME, toggle:NAME...
 *
 * Run:  deno task 06        Test: deno task 06 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
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
</Window>`;

function main(): number {
  if (!t.init("Raven tutorial 06")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const status = r.find(window, "status");
  const showStatus = r.find(window, "view.status");

  let last = "none";

  const onMenu = (_sender: number, action: string) => {
    last = action;
    r.setText(status, `Menu: ${action.slice(action.lastIndexOf(".") + 1)}`);
  };
  for (const name of ["menu.new", "menu.open", "menu.recent"]) r.actionBind(name, onMenu);

  r.actionBind("view.status", (sender) => {
    // A checkable entry keeps its own state: ask it, never mirror it.
    r.setText(status, r.menuItemIsChecked(sender) ? "Status shown" : "Status hidden");
  });

  if (t.isTest()) {
    r.drawFrame();

    // Geometry of the menus: the bar is one control high, its rows are
    // R_METRIC_ROW_HEIGHT. A click on a title opens it, a click below picks the
    // entry at that row.
    const bar = r.metric(r.R_METRIC_CONTROL_HEIGHT);
    const row = r.metric(r.R_METRIC_ROW_HEIGHT);

    t.clickAt(window, 15, bar * 0.5);
    t.clickAt(window, 30, bar + 4 + row * 0.5);
    t.check(last === "menu.new", "File > New fires menu.new");
    t.check(r.getText(status) === "Menu: new", "the status label names the entry");

    // The shortcut of the same menu, with the menu closed.
    t.key(window, r.R_KEY_O, r.R_MOD_CTRL);
    t.check(last === "menu.open", "Ctrl+O fires menu.open");

    // A checkable entry toggles itself, then reports it.
    t.check(r.menuItemIsChecked(showStatus), "Show status starts checked");
    t.key(window, r.R_KEY_G, r.R_MOD_CTRL);
    t.check(!r.menuItemIsChecked(showStatus), "Ctrl+G clears the check mark");
    t.check(r.getText(status) === "Status hidden", "the entry reports its own new state");

    // The built-in app.quit needs no binding at all.
    t.key(window, r.R_KEY_Q, r.R_MOD_CTRL);
    t.check(!r.isRunning(), "Ctrl+Q runs the built-in app.quit");

    return t.finish("06-menus");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("06-menus");
}

Deno.exit(main());
