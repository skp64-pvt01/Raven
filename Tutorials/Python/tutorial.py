"""
What every step of the Python course needs: start Raven, drive widgets without a
window, count the checks, and shut down cleanly.

The steps import it with a two-line path insert, so the course works from any
working directory:

    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    import tutorial

Step 08 copies these functions into its own file instead, so that the starter
project is one folder.
"""

import sys

import raven as r

CHECKS = 0
FAILURES = 0


def is_test():
    """True when the program was run with --test."""
    return "--test" in sys.argv


def check(condition, message):
    """One assertion. The counts are printed by finish()."""
    global CHECKS, FAILURES
    CHECKS += 1
    if not condition:
        FAILURES += 1
        print("  FAIL:", message)


def require(condition, message):
    """A condition the rest of the step depends on: stop here when it is false."""
    if not condition:
        print("  FAIL:", message)
        r.Rshutdown()
        sys.exit(1)


def init(app_name):
    """Starts Raven: a real window normally, the null backend with --test."""
    desc = r.RInitDesc()
    if is_test():
        desc.backend = r.R_BACKEND_NULL
        desc.headless = 1
    else:
        desc.backend = r.R_BACKEND_VULKAN
    if not r.Rinit_ex(desc):
        print("%s: cannot start Raven (is a Vulkan driver installed?)" % app_name)
        return False
    if is_test():
        r.Rset_animations_enabled(0)
    return True


def click(window, widget):
    """Moves the pointer to the middle of a widget and clicks it."""
    x, y, width, height = r.Rwidget_get_rect(widget)
    click_at(window, x + width / 2, y + height / 2)


def click_at(window, x, y):
    """Clicks a point of the window, in logical units."""
    r.Rwindow_inject_mouse_move(window, x, y)
    r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 1)
    r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 0)


def key(window, name, modifiers=0):
    """Presses and releases a key, with modifiers (R_MOD_CTRL, R_MOD_SHIFT...)."""
    r.Rwindow_inject_key(window, name, 1, modifiers)
    r.Rwindow_inject_key(window, name, 0, modifiers)


def finish(name):
    """Stops the library, prints the result, and returns the process exit code."""
    r.Rshutdown()
    if CHECKS:
        print("%s: %d checks, %d failures" % (name, CHECKS, FAILURES))
        print("PASS" if FAILURES == 0 else "FAIL")
    return 0 if FAILURES == 0 else 1
