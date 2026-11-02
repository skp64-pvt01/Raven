/**
 * Raven tutorial 04 (TypeScript) - actions and shortcuts.
 *
 * A button, a menu entry and a keyboard shortcut can run the same code when
 * they all name the same *action*:
 *
 *   OnClicked="counter.add" in XML,
 *   actionFire("counter.add") from code,
 *   registerShortcut(window, R_KEY_I, R_MOD_CTRL, "counter.add").
 *
 * actionBind gives each name a function. The function receives the sender and
 * the action name.
 *
 * Run:  deno task 04        Test: deno task 04 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
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
</Window>`;

function main(): number {
  if (!t.init("Raven tutorial 04")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const label = r.find(window, "count");
  const status = r.find(window, "status");
  const add = r.find(window, "add");
  const addTen = r.find(window, "add_ten");
  const enabled = r.find(window, "enabled");

  let count = 0;

  const show = (what: string) => {
    r.setText(label, String(count));
    r.setText(status, `Last action: ${what}`);
  };

  r.actionBind("counter.add", (_sender, action) => {
    count += 1;
    show(action);
  });
  r.actionBind("counter.add_ten", (_sender, action) => {
    count += 10;
    show(action);
  });
  r.actionBind("counter.reset", (_sender, action) => {
    count = 0;
    show(action);
  });
  r.actionBind("counter.enabled", () => {
    const on = r.isChecked(enabled);
    r.setEnabled(add, on);
    r.setEnabled(addTen, on);
    r.setText(status, on ? "Add buttons enabled." : "Add buttons disabled.");
  });

  // The shortcuts belong to the window, whatever the focus.
  r.registerShortcut(window, r.R_KEY_I, r.R_MOD_CTRL, "counter.add");
  r.registerShortcut(window, r.R_KEY_0, r.R_MOD_CTRL, "counter.reset");

  if (t.isTest()) {
    r.drawFrame();

    t.click(window, add);
    t.check(count === 1, "the button fires counter.add");

    t.key(window, r.R_KEY_I, r.R_MOD_CTRL);
    t.check(count === 2, "Ctrl+I fires the same action");

    r.actionFire("counter.add_ten");
    t.check(count === 12, "an action can be fired from code");

    // A disabled widget takes no clicks...
    t.click(window, enabled);
    t.check(!r.isChecked(enabled), "the checkbox is cleared");
    t.click(window, add);
    t.check(count === 12, "a disabled button does not fire");

    // ...but the shortcut still does: it belongs to the window, not the button.
    t.key(window, r.R_KEY_I, r.R_MOD_CTRL);
    t.check(count === 13, "the shortcut ignores the disabled button");

    t.key(window, r.R_KEY_0, r.R_MOD_CTRL);
    t.check(count === 0, "Ctrl+0 resets");
    t.check(r.getText(status) === "Last action: counter.reset", "the status names the action");

    return t.finish("04-actions");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("04-actions");
}

Deno.exit(main());
