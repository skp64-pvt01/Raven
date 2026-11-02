/**
 * Raven tutorial 01 (TypeScript) - a first window.
 *
 * Three ideas only:
 *   1. init() starts the library, shutdown() ends it.
 *   2. A window is built from XML; find() gets a widget back by name.
 *   3. actionBind() answers OnClicked="..."; the frame loop is yours.
 *
 * Run:  deno task 01            (or: deno run --allow-ffi --allow-env --allow-read main.ts)
 * Test: deno task 01 --test     (no window, no GPU)
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
<Window Title="Tutorial 01 - First window" Width="440" Height="240">
    <VBox Spacing="12" Slot.Padding="24" Slot.HAlign="Center" Slot.VAlign="Center">
        <Label Text="Hello from TypeScript" TextSize="24" Slot.HAlign="Center"/>
        <Label Name="counter" Text="Press the button" Slot.HAlign="Center"/>
        <HBox Spacing="8" Slot.HAlign="Center">
            <Button Name="click" Text="Click me" Style="Primary" OnClicked="counter.add"
                Tooltip="Counts the clicks"/>
            <Button Name="quit" Text="Quit" OnClicked="app.quit"/>
        </HBox>
    </VBox>
</Window>`;

function main(): number {
  if (!t.init("Raven tutorial 01")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const label = r.find(window, "counter");

  // The counter lives in this closure: actionBind keeps it for the life of the
  // program, so it must not depend on anything that goes out of scope.
  let clicks = 0;
  r.actionBind("counter.add", () => {
    clicks += 1;
    r.setText(label, `Clicked ${clicks} time${clicks === 1 ? "" : "s"}`);
  });
  // app.quit is a built-in action of Raven: no binding needed.

  if (t.isTest()) {
    r.drawFrame(); // lays the widgets out: before it, every rect is empty

    const button = r.find(window, "click");
    t.click(window, button);
    t.check(r.getText(label) === "Clicked 1 time", "the first click updates the label");

    t.click(window, button);
    t.check(r.getText(label) === "Clicked 2 times", "the second click counts too");

    t.click(window, r.find(window, "quit"));
    t.check(!r.isRunning(), "Quit ends the loop");
    return t.finish("01-first-window");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("01-first-window");
}

Deno.exit(main());
