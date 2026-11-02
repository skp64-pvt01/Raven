"""
Raven tutorial 05 (Python) - an interface written in XML.

The window exists only as window.xml. Python does three things:

    Rwindow_create_from_xml_file   builds it,
    Rwidget_find                   finds a widget by its Name,
    Raction_bind                   answers OnClicked="app.greet".

The file is found next to this script, so the program works from any directory.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    if not tutorial.init("Raven tutorial 05"):
        return 1

    window = r.Rwindow_create_from_xml_file(os.path.join(HERE, "window.xml"))
    tutorial.require(window != r.R_INVALID_ID, "window.xml could not be read")

    name = r.Rwidget_find(window, "name")
    greeting = r.Rwidget_find(window, "greeting")
    tutorial.require(r.Rwidget_is_valid(name) and r.Rwidget_is_valid(greeting),
                     "a Name is missing from window.xml")

    def greet(sender, action, user_data):
        who = r.Rtext_input_get_text(name) or "world"
        r.Rlabel_set_text(greeting, "Hello, %s!" % who)

    r.Raction_bind("app.greet", greet)

    # Uncomment to read the file again every time it is saved:
    # r.Rxml_set_live_reload(1)

    if tutorial.is_test():
        r.Rdraw_frame()

        tutorial.click(window, name)
        r.Rwindow_inject_text(window, "Grace")
        tutorial.click(window, r.Rwidget_find(window, "greet"))
        tutorial.check(r.Rlabel_get_text(greeting) == "Hello, Grace!",
                       "the button fires the action the XML names")

        # The XML names an action, not a function: it can be fired without the button.
        r.Rtext_input_set_text(name, "")
        r.Raction_fire("app.greet", r.R_INVALID_ID)
        tutorial.check(r.Rlabel_get_text(greeting) == "Hello, world!",
                       "the action works with no widget behind it")

        return tutorial.finish("05-xml-files")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("05-xml-files")


if __name__ == "__main__":
    sys.exit(main())
