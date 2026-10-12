#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>

namespace hrms::gui {

class Theme {
public:
    // Core Palette (Modern Executive Dark)
    static Fl_Color COLOR_BG;
    static Fl_Color COLOR_HEADER_BG;
    static Fl_Color COLOR_CARD_BG;
    static Fl_Color COLOR_CARD_HOVER;
    static Fl_Color COLOR_INPUT_BG;
    static Fl_Color COLOR_BORDER;
    
    // Typography Colors
    static Fl_Color COLOR_TEXT_PRIMARY;
    static Fl_Color COLOR_TEXT_SECONDARY;
    static Fl_Color COLOR_TEXT_MUTED;
    static Fl_Color COLOR_TEXT_INVERTED;

    // Accent Colors
    static Fl_Color COLOR_PRIMARY;
    static Fl_Color COLOR_PRIMARY_HOVER;
    static Fl_Color COLOR_SUCCESS;
    static Fl_Color COLOR_SUCCESS_HOVER;
    static Fl_Color COLOR_WARNING;
    static Fl_Color COLOR_DANGER;
    static Fl_Color COLOR_PURPLE;
    static Fl_Color COLOR_CYAN;

    // Table Colors
    static Fl_Color COLOR_TABLE_HEADER_BG;
    static Fl_Color COLOR_TABLE_ROW_EVEN;
    static Fl_Color COLOR_TABLE_ROW_ODD;
    static Fl_Color COLOR_TABLE_ROW_SELECTED;

    // Initialization & Global Application
    static void init();

    // Widget Styling Helpers
    static void stylePrimaryButton(Fl_Button* btn);
    static void styleSecondaryButton(Fl_Button* btn);
    static void styleSuccessButton(Fl_Button* btn);
    static void styleDangerButton(Fl_Button* btn);
    static void styleWarningButton(Fl_Button* btn);
    static void styleInput(Fl_Input* input);
    static void styleChoice(Fl_Choice* choice);
    static void styleCard(Fl_Widget* widget);
    static void styleTabs(Fl_Tabs* tabs);
};

} // namespace hrms::gui
