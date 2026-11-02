/**
 * Raven tutorial 08 - the starting point of an application.
 *
 * Everything the seven steps taught, in the usual shape of an editor:
 *
 *   notes.xml        the window: a header bar with the menus, the editor, a status bar
 *   this file        the actions the menu entries name, and the files they touch
 *
 * Run it with a file to open it:   ./08-capstone-notes notes.txt
 * Check it without a window:       ./08-capstone-notes --test
 *
 * Where to go from here:
 *   - a toolbar (Rbutton_create in the header bar) firing the same actions
 *   - recent files (a sub-menu built at run time with Rmenu_add_item)
 *   - a language chosen with a combo box (see examples/notepad in Raven)
 *   - a splitter with a preview, a file browser (Rfile_browser_create)
 */

#include <Raven.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef RAVEN_TUTORIAL_DIR
#define RAVEN_TUTORIAL_DIR "."
#endif

/* ---------------------------------------------------------------------------
 *  Test helpers
 *
 * This folder is meant to be copied and grown, so it depends on nothing but
 * Raven. The same helpers are shared by the earlier steps in ../common/.
 *
 *     ./notes            a window
 *     ./notes --test     no window, no GPU: checks only
 * ------------------------------------------------------------------------- */

namespace tutorial {

inline int checks = 0;
inline int failures = 0;

inline bool is_test(int argc, char** argv) {
	return argc > 1 && std::strcmp(argv[1], "--test") == 0;
}

inline void expect(bool ok, const char* expression, int line) {
	++checks;
	if (!ok) {
		++failures;
		std::printf("  FAIL line %d: %s\n", line, expression);
	}
}

inline void on_error(RError code, RSeverity severity, const char* detail) {
	std::printf("  [Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

inline bool init(int argc, char** argv, const char* app_name) {
	RInitDesc desc{};
	desc.app_name = app_name;
	desc.backend = R_BACKEND_VULKAN;
	if (is_test(argc, argv)) {
		desc.backend = R_BACKEND_NULL;
		desc.headless = R_TRUE;
	}
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		std::printf("%s: cannot start Raven\n", app_name);
		return false;
	}
	if (is_test(argc, argv)) Rset_animations_enabled(R_FALSE);
	return true;
}

inline void click(Rid window, Rid widget) {
	float x = 0, y = 0, w = 0, h = 0;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

inline void key(Rid window, RKey k, Ruint modifiers = 0) {
	Rwindow_inject_key(window, k, R_TRUE, modifiers);
	Rwindow_inject_key(window, k, R_FALSE, modifiers);
}

inline int finish(const char* name) {
	Rshutdown();
	if (checks > 0) {
		std::printf("%s: %d checks, %d failures\n", name, checks, failures);
		std::printf("%s\n", failures == 0 ? "PASS" : "FAIL");
	}
	return failures == 0 ? 0 : 1;
}

}  // namespace tutorial

#define TUTORIAL_CHECK(expression) ::tutorial::expect((expression), #expression, __LINE__)

struct App {
	Rid window = R_INVALID_ID;
	Rid editor = R_INVALID_ID;
	Rid status = R_INVALID_ID;
	Rid position = R_INVALID_ID;
	Rid numbers_item = R_INVALID_ID;
	char path[1024] = "";
	bool modified = false;
	bool ask_dialogs = true;   /* false in a --test run: no user to answer */
	int last_line = -1;
	int last_column = -1;
};

static App app;

/* ---------------------------------------------------------------------------
 *  Title, status
 * ------------------------------------------------------------------------- */

static const char* base_name(const char* path) {
	const char* slash = std::strrchr(path, '/');
	return slash ? slash + 1 : path;
}

static void update_title() {
	char title[1100];
	std::snprintf(title, sizeof(title), "%s%s - Notes",
		app.path[0] ? base_name(app.path) : "Untitled", app.modified ? " *" : "");
	Rwindow_set_title(app.window, title);
}

static void set_status(const char* text) {
	Rlabel_set_text(app.status, text);
}

/* The cursor, read once per frame: nothing to wire, nothing to miss. */
static void on_frame(double delta_seconds, void* user_data) {
	(void)delta_seconds;
	(void)user_data;
	if (!Rwidget_is_valid(app.editor)) return;

	int line = 0, column = 0;
	Rtext_editor_get_cursor(app.editor, &line, &column);
	if (line == app.last_line && column == app.last_column) return;
	app.last_line = line;
	app.last_column = column;

	char text[64];
	std::snprintf(text, sizeof(text), "Line %d, column %d", line + 1, column + 1);
	Rlabel_set_text(app.position, text);
}

static void on_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (app.modified) return;
	app.modified = true;
	update_title();
}

/* ---------------------------------------------------------------------------
 *  Files: plain functions, so a test can call them with no dialog
 * ------------------------------------------------------------------------- */

static void new_document() {
	Rtext_editor_set_text(app.editor, "");
	app.path[0] = '\0';
	app.modified = false;
	Rtext_editor_set_language(app.editor, nullptr);
	update_title();
	set_status("New document");
	Rwidget_focus(app.editor);
}

static bool open_path(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	if (!text) {
		char status[1100];
		std::snprintf(status, sizeof(status), "Cannot read %s", path);
		set_status(status);
		return false;
	}

	Rtext_editor_set_text(app.editor, text);
	Rmemory_free(text);

	std::snprintf(app.path, sizeof(app.path), "%s", path);
	app.modified = false;
	/* "main.rs" becomes Rust, "notes.txt" nothing in particular. */
	Rtext_editor_set_language(app.editor, Rlanguage_for_file(app.path));
	update_title();

	char status[1100];
	std::snprintf(status, sizeof(status), "Opened %s", path);
	set_status(status);
	Rwidget_focus(app.editor);
	return true;
}

static bool save_to(const char* path) {
	const char* text = Rtext_editor_get_text(app.editor);
	const int length = Rtext_editor_get_length(app.editor);
	if (!Rfile_write(path, text, static_cast<size_t>(length))) {
		char status[1100];
		std::snprintf(status, sizeof(status), "Cannot write %s", path);
		set_status(status);
		return false;
	}

	std::snprintf(app.path, sizeof(app.path), "%s", path);
	app.modified = false;
	Rtext_editor_set_language(app.editor, Rlanguage_for_file(app.path));
	update_title();

	char status[1100];
	std::snprintf(status, sizeof(status), "Saved %s", path);
	set_status(status);
	return true;
}

static void ask_open() {
	RFileDialogDesc desc{};
	desc.parent = app.window;
	desc.title = "Open a text";
	const char* chosen = Rfile_dialog_open(&desc);
	if (chosen) open_path(chosen);
}

/** Saves, asking for a name only when there is none yet or when asked to. */
static bool save(bool ask) {
	if (ask || !app.path[0]) {
		RFileDialogDesc desc{};
		desc.parent = app.window;
		desc.title = "Save the text";
		desc.file_name = app.path[0] ? base_name(app.path) : "untitled.txt";
		desc.default_extension = "txt";
		const char* chosen = Rfile_dialog_save(&desc);
		if (!chosen) return false;
		return save_to(chosen);
	}
	return save_to(app.path);
}

/** R_TRUE when the text can be replaced: nothing to lose, or the user agreed. */
static bool can_discard() {
	if (!app.modified) return true;
	if (!app.ask_dialogs) return true;   /* a test has no user to ask */
	return Rmessage_box(app.window, "Unsaved changes",
		"The text has changes that were not saved. Discard them?",
		R_MESSAGE_OK_CANCEL) == R_MESSAGE_RESULT_OK;
}

/* ---------------------------------------------------------------------------
 *  Actions: the names that notes.xml uses
 * ------------------------------------------------------------------------- */

static void action_file(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (std::strcmp(action, "file.new") == 0) {
		if (can_discard()) new_document();
	} else if (std::strcmp(action, "file.open") == 0) {
		if (can_discard()) ask_open();
	} else if (std::strcmp(action, "file.save") == 0) {
		save(false);
	} else if (std::strcmp(action, "file.save_as") == 0) {
		save(true);
	} else if (std::strcmp(action, "file.quit") == 0) {
		if (can_discard()) Rquit(0);
	}
}

static void action_edit(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	Rwidget_focus(app.editor);
	if (std::strcmp(action, "edit.undo") == 0) Rtext_editor_undo(app.editor);
	else if (std::strcmp(action, "edit.redo") == 0) Rtext_editor_redo(app.editor);
	else if (std::strcmp(action, "edit.select_all") == 0) Rtext_editor_select_all(app.editor);
}

static void action_view(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (std::strcmp(action, "view.numbers") == 0) {
		/* The menu item owns its check mark: read it, do not mirror it. */
		Rtext_editor_set_line_numbers(app.editor, Rmenu_item_is_checked(sender));
	} else if (std::strcmp(action, "view.wrap") == 0) {
		Rtext_editor_set_word_wrap(app.editor, Rmenu_item_is_checked(sender));
	} else if (std::strcmp(action, "view.dark") == 0) {
		Rtheme_apply(R_THEME_DARK);
	} else if (std::strcmp(action, "view.light") == 0) {
		Rtheme_apply(R_THEME_LIGHT);
	}
}

/** The close button of the header bar and Alt+F4 ask the same question. */
static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	if (!can_discard()) return R_FALSE;
	Rquit(0);
	return R_TRUE;
}

/** A file dragged from the file manager onto the window is opened. */
static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	(void)target;
	(void)user_data;
	if (event->phase != R_DROP_DROPPED) return R_TRUE;
	if (event->path_count > 0 && can_discard()) open_path(event->paths[0]);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

static const char WELCOME[] =
	"Notes - a starter project\n"
	"\n"
	"File > Open (Ctrl+O) opens a text, File > Save (Ctrl+S) writes it.\n"
	"Drop a file on the window to open it too.\n"
	"\n"
	"TODO: add a toolbar, recent files, or a preview panel.\n";

int main(int argc, char** argv) {
	const bool test = tutorial::is_test(argc, argv);
	if (!tutorial::init(argc, argv, "Raven Notes")) return 1;

	Raction_bind("file.new", action_file, nullptr);
	Raction_bind("file.open", action_file, nullptr);
	Raction_bind("file.save", action_file, nullptr);
	Raction_bind("file.save_as", action_file, nullptr);
	Raction_bind("file.quit", action_file, nullptr);
	Raction_bind("edit.undo", action_edit, nullptr);
	Raction_bind("edit.redo", action_edit, nullptr);
	Raction_bind("edit.select_all", action_edit, nullptr);
	Raction_bind("view.numbers", action_view, nullptr);
	Raction_bind("view.wrap", action_view, nullptr);
	Raction_bind("view.dark", action_view, nullptr);
	Raction_bind("view.light", action_view, nullptr);

	/* NOTES_XML overrides the interface file: the packaging step uses it to run
	 * the build-tree binary before the data file is installed. */
	const char* xml = std::getenv("NOTES_XML");
	if (!xml) xml = RAVEN_TUTORIAL_DIR "/notes.xml";
	app.window = Rwindow_create_from_xml_file(xml);
	if (app.window == R_INVALID_ID) {
		std::printf("cannot read notes.xml\n");
		return tutorial::finish("08-capstone-notes");
	}
	app.ask_dialogs = !test;

	app.editor = Rwidget_find(app.window, "editor");
	app.status = Rwidget_find(app.window, "status");
	app.position = Rwidget_find(app.window, "position");
	app.numbers_item = Rwidget_find(app.window, "view.numbers");
	TUTORIAL_CHECK(Rwidget_is_valid(app.editor));

	Rtext_editor_set_changed_callback(app.editor, on_changed, nullptr);
	Rwindow_set_close_callback(app.window, on_close, nullptr);
	Rwindow_set_drop_callback(app.window, on_drop, nullptr);
	Rset_frame_callback(on_frame, nullptr);

	if (test) {
		Rdraw_frame();

		/* 1. Open a file written by the test. */
		const char* path = "/tmp/raven_tutorial_08.txt";
		const char sample[] = "Notes\nsecond line\n";
		TUTORIAL_CHECK(Rfile_write(path, sample, sizeof(sample) - 1));
		TUTORIAL_CHECK(open_path(path));
		TUTORIAL_CHECK(std::strcmp(Rtext_editor_get_text(app.editor), sample) == 0);
		TUTORIAL_CHECK(!app.modified);

		/* 2. Save with an explicit path, as File > Save does once a name is known. */
		TUTORIAL_CHECK(save_to(path));
		TUTORIAL_CHECK(!app.modified);

		/* 3. Typing marks the document, and the title says so. */
		tutorial::click(app.window, app.editor);
		Rwindow_inject_text(app.window, "abc");
		TUTORIAL_CHECK(app.modified);

		/* 4. File > New clears the editor: the action the menu entry names. */
		Raction_fire("file.new", R_INVALID_ID);
		TUTORIAL_CHECK(std::strcmp(Rtext_editor_get_text(app.editor), "") == 0);
		TUTORIAL_CHECK(!app.modified);
		TUTORIAL_CHECK(app.path[0] == '\0');

		/* 5. The checkable View entry drives the editor through its own state. */
		Rmenu_item_set_checked(app.numbers_item, R_FALSE);
		Raction_fire("view.numbers", app.numbers_item);
		TUTORIAL_CHECK(Rmenu_item_is_checked(app.numbers_item) == R_FALSE);

		/* 6. Ctrl+N runs the same action through the shortcut of the XML. */
		Rtext_editor_set_text(app.editor, "text to lose");
		app.modified = false;
		tutorial::key(app.window, R_KEY_N, R_MOD_CTRL);
		TUTORIAL_CHECK(std::strcmp(Rtext_editor_get_text(app.editor), "") == 0);

		/* 7. Quit through the action leaves the loop. */
		Raction_fire("file.quit", R_INVALID_ID);
		TUTORIAL_CHECK(!Ris_running());

		std::remove(path);
		return tutorial::finish("08-capstone-notes");
	}

	/* A file given on the command line, else the welcome text. */
	if (argc > 1 && argv[1][0] != '-') {
		open_path(argv[1]);
	} else {
		Rtext_editor_set_text(app.editor, WELCOME);
		Rtext_editor_set_language(app.editor, nullptr);
		update_title();
		set_status("Drop a file on the window, or press Ctrl+O.");
	}
	Rwidget_focus(app.editor);

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("08-capstone-notes");
}
