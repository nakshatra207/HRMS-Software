#include "gui/gui_theme.hpp"

namespace hrms::gui {

// Colors
Fl_Color Theme::COLOR_BG                = fl_rgb_color(24, 27, 34);       // #181B22
Fl_Color Theme::COLOR_HEADER_BG         = fl_rgb_color(18, 20, 26);       // #12141A
Fl_Color Theme::COLOR_CARD_BG           = fl_rgb_color(33, 38, 48);       // #212630
Fl_Color Theme::COLOR_CARD_HOVER        = fl_rgb_color(42, 48, 60);       // #2A303C
Fl_Color Theme::COLOR_INPUT_BG          = fl_rgb_color(20, 23, 29);       // #14171D
Fl_Color Theme::COLOR_BORDER            = fl_rgb_color(52, 60, 76);       // #343C4C

Fl_Color Theme::COLOR_TEXT_PRIMARY      = fl_rgb_color(240, 244, 250);    // #F0F4FA
Fl_Color Theme::COLOR_TEXT_SECONDARY    = fl_rgb_color(160, 172, 192);    // #A0ACC0
Fl_Color Theme::COLOR_TEXT_MUTED        = fl_rgb_color(110, 122, 142);    // #6E7A8E
Fl_Color Theme::COLOR_TEXT_INVERTED     = fl_rgb_color(255, 255, 255);    // #FFFFFF

Fl_Color Theme::COLOR_PRIMARY           = fl_rgb_color(37, 99, 235);      // #2563EB (Royal Blue)
Fl_Color Theme::COLOR_PRIMARY_HOVER     = fl_rgb_color(59, 130, 246);     // #3B82F6
Fl_Color Theme::COLOR_SUCCESS           = fl_rgb_color(22, 163, 74);      // #16A34A (Emerald Green)
Fl_Color Theme::COLOR_SUCCESS_HOVER     = fl_rgb_color(34, 197, 94);      // #22C55E
Fl_Color Theme::COLOR_WARNING           = fl_rgb_color(217, 119, 6);      // #D97706 (Amber)
Fl_Color Theme::COLOR_DANGER            = fl_rgb_color(220, 38, 38);      // #DC2626 (Crimson Red)
Fl_Color Theme::COLOR_PURPLE            = fl_rgb_color(147, 51, 234);     // #9333EA (Purple)
Fl_Color Theme::COLOR_CYAN              = fl_rgb_color(6, 182, 212);      // #06B6D4 (Cyan)

Fl_Color Theme::COLOR_TABLE_HEADER_BG   = fl_rgb_color(26, 30, 38);       // #1A1E26
Fl_Color Theme::COLOR_TABLE_ROW_EVEN    = fl_rgb_color(31, 36, 46);       // #1F242E
Fl_Color Theme::COLOR_TABLE_ROW_ODD     = fl_rgb_color(27, 31, 40);       // #1B1F28
Fl_Color Theme::COLOR_TABLE_ROW_SELECTED= fl_rgb_color(37, 99, 235);      // #2563EB

void Theme::init() {
    Fl::scheme("gtk+");
    Fl::background(24, 27, 34);
    Fl::foreground(240, 244, 250);
}

void Theme::stylePrimaryButton(Fl_Button* btn) {
    if (!btn) return;
    btn->box(FL_FLAT_BOX);
    btn->color(COLOR_PRIMARY);
    btn->selection_color(COLOR_PRIMARY_HOVER);
    btn->labelcolor(COLOR_TEXT_INVERTED);
    btn->labelfont(FL_HELVETICA_BOLD);
    btn->labelsize(12);
    btn->clear_visible_focus();
}

void Theme::styleSecondaryButton(Fl_Button* btn) {
    if (!btn) return;
    btn->box(FL_FLAT_BOX);
    btn->color(COLOR_CARD_BG);
    btn->selection_color(COLOR_CARD_HOVER);
    btn->labelcolor(COLOR_TEXT_PRIMARY);
    btn->labelfont(FL_HELVETICA);
    btn->labelsize(12);
    btn->clear_visible_focus();
}

void Theme::styleSuccessButton(Fl_Button* btn) {
    if (!btn) return;
    btn->box(FL_FLAT_BOX);
    btn->color(COLOR_SUCCESS);
    btn->selection_color(COLOR_SUCCESS_HOVER);
    btn->labelcolor(COLOR_TEXT_INVERTED);
    btn->labelfont(FL_HELVETICA_BOLD);
    btn->labelsize(12);
    btn->clear_visible_focus();
}

void Theme::styleDangerButton(Fl_Button* btn) {
    if (!btn) return;
    btn->box(FL_FLAT_BOX);
    btn->color(COLOR_DANGER);
    btn->selection_color(fl_rgb_color(239, 68, 68));
    btn->labelcolor(COLOR_TEXT_INVERTED);
    btn->labelfont(FL_HELVETICA_BOLD);
    btn->labelsize(12);
    btn->clear_visible_focus();
}

void Theme::styleWarningButton(Fl_Button* btn) {
    if (!btn) return;
    btn->box(FL_FLAT_BOX);
    btn->color(COLOR_WARNING);
    btn->selection_color(fl_rgb_color(245, 158, 11));
    btn->labelcolor(COLOR_TEXT_INVERTED);
    btn->labelfont(FL_HELVETICA_BOLD);
    btn->labelsize(12);
    btn->clear_visible_focus();
}

void Theme::styleInput(Fl_Input* input) {
    if (!input) return;
    input->box(FL_FLAT_BOX);
    input->color(COLOR_INPUT_BG);
    input->textcolor(COLOR_TEXT_PRIMARY);
    input->cursor_color(COLOR_PRIMARY);
    input->textfont(FL_HELVETICA);
    input->textsize(12);
    input->labelcolor(COLOR_TEXT_SECONDARY);
    input->labelfont(FL_HELVETICA_BOLD);
    input->labelsize(12);
}

void Theme::styleChoice(Fl_Choice* choice) {
    if (!choice) return;
    choice->box(FL_FLAT_BOX);
    choice->color(COLOR_INPUT_BG);
    choice->textcolor(COLOR_TEXT_PRIMARY);
    choice->textfont(FL_HELVETICA);
    choice->textsize(12);
    choice->labelcolor(COLOR_TEXT_SECONDARY);
    choice->labelfont(FL_HELVETICA_BOLD);
    choice->labelsize(12);
}

void Theme::styleCard(Fl_Widget* widget) {
    if (!widget) return;
    widget->box(FL_FLAT_BOX);
    widget->color(COLOR_CARD_BG);
}

void Theme::styleTabs(Fl_Tabs* tabs) {
    if (!tabs) return;
    tabs->box(FL_FLAT_BOX);
    tabs->color(COLOR_BG);
    tabs->selection_color(COLOR_CARD_BG);
    tabs->labelcolor(COLOR_TEXT_SECONDARY);
    tabs->labelfont(FL_HELVETICA_BOLD);
    tabs->labelsize(13);
}

} // namespace hrms::gui
