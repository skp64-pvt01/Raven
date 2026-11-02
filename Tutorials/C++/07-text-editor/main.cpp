/**
 * Raven tutorial 07 - a text editor and files.
 *
 * The whole editor is one widget:
 *
 *   Rtext_editor_create(text)          the document, the caret, the selection
 *   Rtext_editor_get_text / get_length what to save
 *   Rtext_editor_set_changed_callback  "the user typed"
 *   Rtext_editor_set_language          colors, proposals, brackets
 *   Rtext_editor_undo / redo / select_all
 *
 * Files go through the library too, so an application reads the same paths on
 * every platform: Rfile_read / Rfile_write / Rmemory_free, and the file dialogs
 * of the system, Rfile_dialog_open / Rfile_dialog_save.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cstdio>
#include <cstring>

#include "../common/tutorial.h"

struct App {
	Rid window = R_INVALID_ID;
	Rid editor = R_INVALID_ID;
	Rid status = R_INVALID_ID;
	char path[512] = "";
	bool modified = false;
};

static App app;

static const char* base_name(const char* path) {
	const char* slash = std::strrchr(path, '/');
	return slash ? slash + 1 : path;
}

static void update_title() {
	char title[640];
	std::snprintf(title, sizeof(title), "%s%s - Tutorial 07",
		app.path[0] ? base_name(app.path) : "Untitled", app.modified ? " *" : "");
	Rwindow_set_title(app.window, title);
}

static void set_status(const char* text) {
	Rlabel_set_text(app.status, text);
}

/** Loads a file into the editor. Returns false when the file cannot be read. */
static bool open_path(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	if (!text) return false;

	Rtext_editor_set_text(app.editor, text);
	Rmemory_free(text);                    /* the library gave it, the library frees it */

	std::snprintf(app.path, sizeof(app.path), "%s", path);
	app.modified = false;
	Rtext_editor_set_language(app.editor, Rlanguage_for_file(app.path));
	update_title();

	char status[600];
	std::snprintf(status, sizeof(status), "Opened %s (%zu bytes)", path, size);
	set_status(status);
	return true;
}

/** Writes the editor to a path. */
static bool save_to(const char* path) {
	const char* text = Rtext_editor_get_text(app.editor);
	const int length = Rtext_editor_get_length(app.editor);
	if (!Rfile_write(path, text, static_cast<size_t>(length))) return false;

	std::snprintf(app.path, sizeof(app.path), "%s", path);
	app.modified = false;
	update_title();

	char status[600];
	std::snprintf(status, sizeof(status), "Saved %s (%d bytes)", path, length);
	set_status(status);
	return true;
}

static void on_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (app.modified) return;
	app.modified = true;
	update_title();
	set_status("Modified");
}

/* Only used in a normal run: a dialog needs a window and a user. */
static void ask_open() {
	RFileDialogDesc desc{};
	desc.parent = app.window;
	desc.title = "Open a text";
	const char* chosen = Rfile_dialog_open(&desc);
	if (chosen) open_path(chosen);
}

static void ask_save() {
	RFileDialogDesc desc{};
	desc.parent = app.window;
	desc.title = "Save the text";
	desc.file_name = app.path[0] ? base_name(app.path) : "untitled.txt";
	desc.default_extension = "txt";
	const char* chosen = app.path[0] ? app.path : Rfile_dialog_save(&desc);
	if (chosen) save_to(chosen);
}

int main(int argc, char** argv) {
	const bool test = tutorial::is_test(argc, argv);
	if (!tutorial::init(argc, argv, "Raven tutorial 07")) return 1;

	static const char WELCOME[] =
		"Raven tutorial 07\n"
		"\n"
		"This text lives in a TextEditor: several lines, undo, clipboard,\n"
		"and colors once the file has a name the library knows.\n";

	app.window = Rwindow_create("Tutorial 07 - Editor", 720, 520, R_WINDOW_DEFAULT);

	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 0.0f);
	Rwindow_set_content(app.window, column);

	/* --- a small toolbar --------------------------------------------------- */
	Rid row = Rwidget_add_child(column, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 6.0f);
	Rslot_set_padding(row, 8.0f, 8.0f, 8.0f, 8.0f);

	Rid open_button = Rwidget_add_child(row, Rbutton_create("Open..."));
	Rbutton_set_clicked_callback(open_button, [](Rid, void*) { ask_open(); }, nullptr);

	Rid save_button = Rwidget_add_child(row, Rbutton_create("Save"));
	Rbutton_set_clicked_callback(save_button, [](Rid, void*) { ask_save(); }, nullptr);
	Rwidget_set_tooltip(save_button, "Ctrl+S in a real application");

	/* --- the editor -------------------------------------------------------- */
	app.editor = Rwidget_add_child(column, Rtext_editor_create(WELCOME));
	Rslot_set_fill(app.editor, 1.0f);
	Rtext_editor_set_line_numbers(app.editor, R_TRUE);
	Rtext_editor_set_word_wrap(app.editor, R_TRUE);
	Rtext_editor_set_changed_callback(app.editor, on_changed, nullptr);

	/* --- the status bar ---------------------------------------------------- */
	app.status = Rwidget_add_child(column, Rlabel_create("Ready"));
	Rslot_set_padding(app.status, 10.0f, 6.0f, 10.0f, 8.0f);
	Rwidget_set_opacity(app.status, 0.8f);

	update_title();

	if (test) {
		Rdraw_frame();

		/* 1. A file written by the application, read back into the editor. */
		const char* path = "/tmp/raven_tutorial_07.txt";
		const char sample[] = "first line\nsecond line\n";
		TUTORIAL_CHECK(Rfile_write(path, sample, sizeof(sample) - 1));
		TUTORIAL_CHECK(open_path(path));
		TUTORIAL_CHECK(std::strcmp(Rtext_editor_get_text(app.editor), sample) == 0);
		TUTORIAL_CHECK(Rtext_editor_get_length(app.editor) == static_cast<int>(sizeof(sample) - 1));
		TUTORIAL_CHECK(!app.modified);

		/* 2. Typing marks the document modified. */
		tutorial::click(app.window, app.editor);
		Rwindow_inject_text(app.window, "typed");
		TUTORIAL_CHECK(app.modified);
		TUTORIAL_CHECK(std::strstr(Rtext_editor_get_text(app.editor), "typed") != nullptr);

		/* 3. Saving writes what the editor holds, and clears the mark. */
		TUTORIAL_CHECK(save_to(path));
		TUTORIAL_CHECK(!app.modified);
		size_t size = 0;
		char* written = Rfile_read(path, &size);
		TUTORIAL_CHECK(written != nullptr);
		TUTORIAL_CHECK(written && std::strstr(written, "typed") != nullptr);
		Rmemory_free(written);

		/* 4. The library cleans up after itself. */
		std::remove(path);
		return tutorial::finish("07-text-editor");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("07-text-editor");
}
