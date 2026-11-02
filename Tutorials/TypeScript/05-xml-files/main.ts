/**
 * Raven tutorial 05 (TypeScript) - an interface written in XML.
 *
 * The window exists only as window.xml. TypeScript does three things:
 *
 *   windowFromXmlFile   builds it,
 *   find                finds a widget by its Name,
 *   actionBind          answers OnClicked="app.greet".
 *
 * The file is found next to this script with import.meta.url, so the program
 * works from any working directory.
 *
 * Run:  deno task 05        Test: deno task 05 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

// A file:// URL gives the path of the XML beside this source file.
const XML_PATH = new URL("./window.xml", import.meta.url).pathname;

function main(): number {
  if (!t.init("Raven tutorial 05")) return 1;

  const window = r.windowFromXmlFile(XML_PATH);
  t.require(window !== r.INVALID_ID, "window.xml could not be read");

  const name = r.find(window, "name");
  const greeting = r.find(window, "greeting");
  t.require(r.isValid(name) && r.isValid(greeting), "a Name is missing from window.xml");

  r.actionBind("app.greet", () => {
    const who = r.inputText(name) || "world";
    r.setText(greeting, `Hello, ${who}!`);
  });

  // Read the file again every time it is saved:
  r.setLiveReload(true);

  if (t.isTest()) {
    r.drawFrame();

    t.click(window, name);
    r.typeText(window, "Grace");
    t.click(window, r.find(window, "greet"));
    t.check(r.getText(greeting) === "Hello, Grace!", "the button fires the action the XML names");

    // The XML names an action, not a function: it can be fired without the button.
    r.setInputText(name, "");
    r.actionFire("app.greet");
    t.check(r.getText(greeting) === "Hello, world!", "the action works with no widget behind it");

    return t.finish("05-xml-files");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("05-xml-files");
}

Deno.exit(main());
