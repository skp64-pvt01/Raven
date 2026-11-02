/**
 * Raven for TypeScript - a small binding over the C library with Deno's FFI.
 *
 * It is not a complete binding: it has what the course uses, under names close
 * to the C API. The rules are the same as in C:
 *
 *   - a window, a widget, a menu is a `Rid` (an opaque number);
 *   - texts are `string` here, encoded to UTF-8 for the library;
 *   - a callback keeps a `Deno.UnsafeCallback` alive, which this module owns;
 *   - the library is `libraven.so` (build Raven with `-DRAVEN_BUILD_SHARED=ON`),
 *     looked for in `RAVEN_LIBRARY`, then beside the program, then on the system.
 *
 * Run a program with:
 *
 *     deno run --allow-ffi --allow-env --allow-read --allow-write main.ts
 */

/** An opaque handle of the library. */
export type Rid = number;

function libraryPath(): string {
  const explicit = Deno.env.get("RAVEN_LIBRARY");
  if (explicit) return explicit;
  // An installed program keeps the library beside itself: /usr/bin/notes ->
  // /usr/lib/libraven.so. In a checkout, RAVEN_LIBRARY is the way in.
  try {
    const executable = Deno.execPath();
    const folder = executable.slice(0, executable.lastIndexOf("/"));
    const beside = `${folder}/../lib/libraven.so`;
    Deno.statSync(beside);
    return beside;
  } catch {
    // No executable path, or no library there: use the system search.
  }
  return "libraven.so";
}

const symbols = {
  Rinit_ex: { parameters: ["buffer"], result: "i32" },
  Rshutdown: { parameters: [], result: "void" },
  Rdraw_frame: { parameters: [], result: "i32" },
  Rwait_frame: { parameters: ["f64"], result: "i32" },
  Rquit: { parameters: ["i32"], result: "void" },
  Ris_running: { parameters: [], result: "i32" },
  Rset_animations_enabled: { parameters: ["i32"], result: "void" },

  Rwindow_create_from_xml: { parameters: ["buffer", "usize"], result: "u32" },
  Rwindow_create_from_xml_file: { parameters: ["buffer"], result: "u32" },
  Rwindow_set_title: { parameters: ["u32", "buffer"], result: "void" },
  Rwindow_set_close_callback: { parameters: ["u32", "pointer", "pointer"], result: "void" },
  Rwindow_set_drop_callback: { parameters: ["u32", "pointer", "pointer"], result: "void" },
  Rwindow_register_shortcut: { parameters: ["u32", "i32", "u32", "buffer"], result: "void" },

  Rwidget_find: { parameters: ["u32", "buffer"], result: "u32" },
  Rwidget_is_valid: { parameters: ["u32"], result: "i32" },
  Rwidget_get_child: { parameters: ["u32", "i32"], result: "u32" },
  Rwidget_get_rect: {
    parameters: ["u32", "buffer", "buffer", "buffer", "buffer"],
    result: "void",
  },
  Rwidget_set_enabled: { parameters: ["u32", "i32"], result: "void" },
  Rwidget_focus: { parameters: ["u32"], result: "void" },
  Rwidget_set_opacity: { parameters: ["u32", "f32"], result: "void" },
  Rwidget_set_tooltip: { parameters: ["u32", "buffer"], result: "void" },

  Rlabel_set_text: { parameters: ["u32", "buffer"], result: "void" },
  Rlabel_get_text: { parameters: ["u32"], result: "pointer" },
  Rlabel_set_text_size: { parameters: ["u32", "f32"], result: "void" },

  Rtext_input_get_text: { parameters: ["u32"], result: "pointer" },
  Rtext_input_set_text: { parameters: ["u32", "buffer"], result: "void" },
  Rtext_input_set_changed_callback: { parameters: ["u32", "pointer", "pointer"], result: "void" },
  Rcheckbox_is_checked: { parameters: ["u32"], result: "i32" },
  Rslider_get_value: { parameters: ["u32"], result: "f32" },
  Rcombo_box_get_selected: { parameters: ["u32"], result: "i32" },

  Raction_bind: { parameters: ["buffer", "pointer", "pointer"], result: "void" },
  Raction_fire: { parameters: ["buffer", "u32"], result: "void" },
  Rmenu_item_is_checked: { parameters: ["u32"], result: "i32" },
  Rmenu_item_set_checked: { parameters: ["u32", "i32"], result: "void" },

  Rtext_editor_set_text: { parameters: ["u32", "buffer"], result: "void" },
  Rtext_editor_get_text: { parameters: ["u32"], result: "pointer" },
  Rtext_editor_get_length: { parameters: ["u32"], result: "i32" },
  Rtext_editor_get_cursor: { parameters: ["u32", "buffer", "buffer"], result: "void" },
  Rtext_editor_set_language: { parameters: ["u32", "buffer"], result: "void" },
  Rtext_editor_set_line_numbers: { parameters: ["u32", "i32"], result: "void" },
  Rtext_editor_set_word_wrap: { parameters: ["u32", "i32"], result: "void" },
  Rtext_editor_undo: { parameters: ["u32"], result: "void" },
  Rtext_editor_redo: { parameters: ["u32"], result: "void" },
  Rtext_editor_select_all: { parameters: ["u32"], result: "void" },
  Rtext_editor_set_changed_callback: { parameters: ["u32", "pointer", "pointer"], result: "void" },
  Rlanguage_for_file: { parameters: ["buffer"], result: "pointer" },

  Rfile_dialog_open: { parameters: ["buffer"], result: "pointer" },
  Rfile_dialog_save: { parameters: ["buffer"], result: "pointer" },
  Rmessage_box: { parameters: ["u32", "buffer", "buffer", "i32"], result: "i32" },

  Rxml_set_live_reload: { parameters: ["i32"], result: "void" },

  Rtheme_apply: { parameters: ["i32"], result: "void" },
  Rtheme_get_metric: { parameters: ["i32"], result: "f32" },
  Rtheme_is_dark: { parameters: [], result: "i32" },

  Rwindow_inject_mouse_move: { parameters: ["u32", "f32", "f32"], result: "void" },
  Rwindow_inject_mouse_button: { parameters: ["u32", "i32", "i32"], result: "void" },
  Rwindow_inject_key: { parameters: ["u32", "i32", "i32", "u32"], result: "void" },
  Rwindow_inject_text: { parameters: ["u32", "buffer"], result: "void" },
} as const;

const lib = Deno.dlopen(libraryPath(), symbols);

const encoder = new TextEncoder();

/** A text for the library: UTF-8 with a terminating zero. */
function cstr(text: string): Uint8Array {
  const bytes = encoder.encode(text);
  const result = new Uint8Array(bytes.length + 1);
  result.set(bytes);
  return result;
}

/** A text of the library as a string ("" for a null pointer). */
function readText(pointer: Deno.PointerValue): string {
  if (!pointer) return "";
  return new Deno.UnsafePointerView(pointer).getCString();
}

// The callbacks given to the library must outlive the call: the module owns them.
const kept: Deno.UnsafeCallback[] = [];

function callback(
  parameters: Deno.NativeType[],
  result: Deno.NativeResultType,
  function_: (...arguments_: never[]) => unknown,
): Deno.PointerValue {
  const call = new Deno.UnsafeCallback({ parameters, result }, function_ as never);
  kept.push(call);
  return call.pointer;
}

/* ---------------------------------------------------------------------------
 *  Constants
 * ------------------------------------------------------------------------- */

export const INVALID_ID = 0;
export const R_BACKEND_VULKAN = 1;
export const R_BACKEND_NULL = 2;
export const R_WINDOW_DEFAULT = 0;

export const R_MOD_SHIFT = 1;
export const R_MOD_CTRL = 2;
export const R_MOD_ALT = 4;

export const R_KEY_ENTER = 0x0101;
export const R_KEY_ESCAPE = 256;
export const R_KEY_0 = 48;
export const R_KEY_A = 65;
export const R_KEY_G = 71;
export const R_KEY_I = 73;
export const R_KEY_N = 78;
export const R_KEY_O = 79;
export const R_KEY_Q = 81;
export const R_KEY_S = 83;
export const R_KEY_Z = 90;
export const R_KEY_LEFT = 0x0102;
export const R_KEY_UP = 0x0103;

export const R_THEME_DARK = 0x00e0;
export const R_THEME_LIGHT = 0x00e1;
export const R_THEME_GLASS_DARK = 0x00e2;
export const R_THEME_GLASS_LIGHT = 0x00e3;

export const R_METRIC_CONTROL_HEIGHT = 0x0143;
export const R_METRIC_ROW_HEIGHT = 0x0148;

export const R_MESSAGE_OK_CANCEL = 0x0391;
export const R_MESSAGE_RESULT_OK = 1;

/* ---------------------------------------------------------------------------
 *  Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Starts the library. With `headless` no window system and no GPU are used:
 * that is how the tutorials check themselves without a screen.
 */
export function init(headless = false): boolean {
  // RInitDesc: i32 backend, padding, char* app_name (null), six ints/floats.
  const desc = new Uint8Array(40);
  const view = new DataView(desc.buffer);
  view.setInt32(0, headless ? R_BACKEND_NULL : R_BACKEND_VULKAN, true);
  view.setInt32(32, headless ? 1 : 0, true); // headless
  return lib.symbols.Rinit_ex(desc) !== 0;
}

export function shutdown(): void {
  lib.symbols.Rshutdown();
}

/** Draws one frame, then waits. Returns false once the application quit. */
export function waitFrame(timeout = -1.0): boolean {
  return lib.symbols.Rwait_frame(timeout) !== 0;
}

export function drawFrame(): void {
  lib.symbols.Rdraw_frame();
}

export function quit(code = 0): void {
  lib.symbols.Rquit(code);
}

export function isRunning(): boolean {
  return lib.symbols.Ris_running() !== 0;
}

export function setAnimationsEnabled(enabled: boolean): void {
  lib.symbols.Rset_animations_enabled(enabled ? 1 : 0);
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

export function windowFromXml(xml: string): Rid {
  const bytes = encoder.encode(xml);
  return lib.symbols.Rwindow_create_from_xml(bytes, BigInt(bytes.length));
}

export function windowFromXmlFile(path: string): Rid {
  return lib.symbols.Rwindow_create_from_xml_file(cstr(path));
}

export function windowSetTitle(window: Rid, title: string): void {
  lib.symbols.Rwindow_set_title(window, cstr(title));
}

/** The callback returns true to let the window close, false to keep it. */
export function windowSetCloseCallback(
  window: Rid,
  handler: (window: Rid) => boolean,
): void {
  const pointer = callback(["u32", "pointer"], "i32", ((id: Rid) =>
    handler(id) ? 1 : 0) as never);
  lib.symbols.Rwindow_set_close_callback(window, pointer, null);
}

export function registerShortcut(
  window: Rid,
  key: number,
  modifiers: number,
  action: string,
): void {
  lib.symbols.Rwindow_register_shortcut(window, key, modifiers >>> 0, cstr(action));
}

/* ---------------------------------------------------------------------------
 *  Widgets
 * ------------------------------------------------------------------------- */

export function find(scope: Rid, name: string): Rid {
  return lib.symbols.Rwidget_find(scope, cstr(name));
}

export function isValid(widget: Rid): boolean {
  return lib.symbols.Rwidget_is_valid(widget) !== 0;
}

export function getChild(parent: Rid, index: number): Rid {
  return lib.symbols.Rwidget_get_child(parent, index);
}

/** Bounds of a widget in its window: [x, y, width, height]. */
export function rect(widget: Rid): [number, number, number, number] {
  const [x, y, width, height] = [0, 1, 2, 3].map(() => new Float32Array(1));
  const buffer = (array: Float32Array) => new Uint8Array(array.buffer);
  lib.symbols.Rwidget_get_rect(widget, buffer(x), buffer(y), buffer(width), buffer(height));
  return [x[0], y[0], width[0], height[0]];
}

export function setEnabled(widget: Rid, enabled: boolean): void {
  lib.symbols.Rwidget_set_enabled(widget, enabled ? 1 : 0);
}

export function focus(widget: Rid): void {
  lib.symbols.Rwidget_focus(widget);
}

export function setOpacity(widget: Rid, opacity: number): void {
  lib.symbols.Rwidget_set_opacity(widget, opacity);
}

export function setTooltip(widget: Rid, tooltip: string): void {
  lib.symbols.Rwidget_set_tooltip(widget, cstr(tooltip));
}

/* ---------------------------------------------------------------------------
 *  Common controls
 * ------------------------------------------------------------------------- */

export function setText(label: Rid, text: string): void {
  lib.symbols.Rlabel_set_text(label, cstr(text));
}

export function getText(label: Rid): string {
  return readText(lib.symbols.Rlabel_get_text(label));
}

export function setTextSize(label: Rid, size: number): void {
  lib.symbols.Rlabel_set_text_size(label, size);
}

export function inputText(input: Rid): string {
  return readText(lib.symbols.Rtext_input_get_text(input));
}

export function setInputText(input: Rid, text: string): void {
  lib.symbols.Rtext_input_set_text(input, cstr(text));
}

/** Called on every edit of a text input: (input, text). */
export function onInputChanged(
  input: Rid,
  handler: (input: Rid, text: string) => void,
): void {
  const pointer = callback(["u32", "pointer", "pointer"], "void", ((
    id: Rid,
    text: Deno.PointerValue,
  ) => handler(id, readText(text))) as never);
  lib.symbols.Rtext_input_set_changed_callback(input, pointer, null);
}

export function isChecked(checkbox: Rid): boolean {
  return lib.symbols.Rcheckbox_is_checked(checkbox) !== 0;
}

export function sliderValue(slider: Rid): number {
  return lib.symbols.Rslider_get_value(slider);
}

export function selectedIndex(combo: Rid): number {
  return lib.symbols.Rcombo_box_get_selected(combo);
}

export function menuItemIsChecked(item: Rid): boolean {
  return lib.symbols.Rmenu_item_is_checked(item) !== 0;
}

export function menuItemSetChecked(item: Rid, checked: boolean): void {
  lib.symbols.Rmenu_item_set_checked(item, checked ? 1 : 0);
}

/* ---------------------------------------------------------------------------
 *  Actions
 * ------------------------------------------------------------------------- */

/** Binds an action name to `handler(sender, action)`. */
export function actionBind(
  name: string,
  handler: (sender: Rid, action: string) => void,
): void {
  const pointer = callback(["u32", "pointer", "pointer"], "void", ((
    sender: Rid,
    action: Deno.PointerValue,
  ) => handler(sender, readText(action))) as never);
  lib.symbols.Raction_bind(cstr(name), pointer, null);
}

export function actionFire(name: string, sender: Rid = INVALID_ID): void {
  lib.symbols.Raction_fire(cstr(name), sender);
}

/* ---------------------------------------------------------------------------
 *  Text editor
 * ------------------------------------------------------------------------- */

export function editorSetText(editor: Rid, text: string): void {
  lib.symbols.Rtext_editor_set_text(editor, cstr(text));
}

export function editorText(editor: Rid): string {
  return readText(lib.symbols.Rtext_editor_get_text(editor));
}

export function editorLength(editor: Rid): number {
  return lib.symbols.Rtext_editor_get_length(editor);
}

/** [line, column], both zero-based. */
export function editorCursor(editor: Rid): [number, number] {
  const line = new Int32Array(1);
  const column = new Int32Array(1);
  lib.symbols.Rtext_editor_get_cursor(
    editor,
    new Uint8Array(line.buffer),
    new Uint8Array(column.buffer),
  );
  return [line[0], column[0]];
}

/** `null` for plain text. `languageForFile` reads it from an extension. */
export function editorSetLanguage(editor: Rid, language: string | null): void {
  lib.symbols.Rtext_editor_set_language(editor, language === null ? cstr("") : cstr(language));
}

export function languageForFile(path: string): string | null {
  const text = readText(lib.symbols.Rlanguage_for_file(cstr(path)));
  return text.length ? text : null;
}

export function editorSetLineNumbers(editor: Rid, enabled: boolean): void {
  lib.symbols.Rtext_editor_set_line_numbers(editor, enabled ? 1 : 0);
}

export function editorSetWordWrap(editor: Rid, enabled: boolean): void {
  lib.symbols.Rtext_editor_set_word_wrap(editor, enabled ? 1 : 0);
}

export function editorUndo(editor: Rid): void {
  lib.symbols.Rtext_editor_undo(editor);
}

export function editorRedo(editor: Rid): void {
  lib.symbols.Rtext_editor_redo(editor);
}

export function editorSelectAll(editor: Rid): void {
  lib.symbols.Rtext_editor_select_all(editor);
}

/** Called when the user types: (editor). */
export function onEditorChanged(editor: Rid, handler: (editor: Rid) => void): void {
  const pointer = callback(["u32", "pointer"], "void", ((id: Rid) => handler(id)) as never);
  lib.symbols.Rtext_editor_set_changed_callback(editor, pointer, null);
}

/* ---------------------------------------------------------------------------
 *  Dialogs
 * ------------------------------------------------------------------------- */

/**
 * RFileDialogDesc: Rid parent, char* title, directory, file_name, filters,
 * int filter_count, char* default_extension, Ruint flags. The text buffers must
 * outlive the call, so they are kept here until it returns.
 */
function fileDialogDesc(options: {
  parent: Rid;
  title?: string;
  fileName?: string;
  defaultExtension?: string;
}): Uint8Array {
  const desc = new Uint8Array(64);
  const view = new DataView(desc.buffer);
  view.setUint32(0, options.parent, true);

  const write = (offset: number, text?: string) => {
    if (text === undefined) return;
    const bytes = cstr(text);
    keptText.push(bytes);
    view.setBigUint64(offset, Deno.UnsafePointer.value(Deno.UnsafePointer.of(bytes))!, true);
  };
  write(8, options.title);
  write(24, options.fileName);
  write(48, options.defaultExtension);
  return desc;
}

// Text buffers referenced by the last dialog description.
const keptText: Uint8Array[] = [];

export function openFileDialog(options: {
  parent: Rid;
  title?: string;
  directory?: string;
}): string | null {
  keptText.length = 0;
  const desc = fileDialogDesc(options);
  const path = readText(lib.symbols.Rfile_dialog_open(desc));
  return path.length ? path : null;
}

export function saveFileDialog(options: {
  parent: Rid;
  title?: string;
  fileName?: string;
  defaultExtension?: string;
}): string | null {
  keptText.length = 0;
  const desc = fileDialogDesc(options);
  const path = readText(lib.symbols.Rfile_dialog_save(desc));
  return path.length ? path : null;
}

export function messageBox(
  window: Rid,
  title: string,
  text: string,
  buttons = R_MESSAGE_OK_CANCEL,
): number {
  return lib.symbols.Rmessage_box(window, cstr(title), cstr(text), buttons);
}

/* ---------------------------------------------------------------------------
 *  Themes
 * ------------------------------------------------------------------------- */

export function setLiveReload(enabled: boolean): void {
  lib.symbols.Rxml_set_live_reload(enabled ? 1 : 0);
}

export function applyTheme(theme: number): void {
  lib.symbols.Rtheme_apply(theme);
}

export function metric(metricId: number): number {
  return lib.symbols.Rtheme_get_metric(metricId);
}

export function isDark(): boolean {
  return lib.symbols.Rtheme_is_dark() !== 0;
}

/* ---------------------------------------------------------------------------
 *  Synthetic input (tests)
 * ------------------------------------------------------------------------- */

export function mouseMove(window: Rid, x: number, y: number): void {
  lib.symbols.Rwindow_inject_mouse_move(window, x, y);
}

/** `button`: 0 left. `pressed`: true down, false up. */
export function mouseButton(window: Rid, button: number, pressed: boolean): void {
  lib.symbols.Rwindow_inject_mouse_button(window, button, pressed ? 1 : 0);
}

export function key(window: Rid, keyCode: number, modifiers = 0): void {
  lib.symbols.Rwindow_inject_key(window, keyCode, 1, modifiers >>> 0);
  lib.symbols.Rwindow_inject_key(window, keyCode, 0, modifiers >>> 0);
}

export function typeText(window: Rid, text: string): void {
  lib.symbols.Rwindow_inject_text(window, cstr(text));
}
