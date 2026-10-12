#include "gui/gui_common.hpp"
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <algorithm>

namespace hrms::gui {

// =============================================================================
// StatCard Implementation
// =============================================================================
StatCard::StatCard(int x, int y, int w, int h, const char* title, const char* value, Fl_Color accentColor, const char* subtext)
    : Fl_Group(x, y, w, h), m_title(title ? title : ""), m_value(value ? value : ""),
      m_subtext(subtext ? subtext : ""), m_accentColor(accentColor) {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_CARD_BG);
    end();
}

void StatCard::setValue(const std::string& val) {
    m_value = val;
    redraw();
}

void StatCard::setSubtext(const std::string& sub) {
    m_subtext = sub;
    redraw();
}

void StatCard::draw() {
    int X = x(), Y = y(), W = w(), H = h();

    // Card background
    fl_color(Theme::COLOR_CARD_BG);
    fl_rectf(X, Y, W, H);

    // Accent line on top
    fl_color(m_accentColor);
    fl_rectf(X, Y, W, 4);

    // Subtle border
    fl_color(Theme::COLOR_BORDER);
    fl_rect(X, Y, W, H);

    // Title label
    fl_color(Theme::COLOR_TEXT_SECONDARY);
    fl_font(FL_HELVETICA_BOLD, 11);
    fl_draw(m_title.c_str(), X + 14, Y + 14, W - 28, 16, FL_ALIGN_LEFT);

    // Metric Value
    fl_color(Theme::COLOR_TEXT_PRIMARY);
    fl_font(FL_HELVETICA_BOLD, 22);
    fl_draw(m_value.c_str(), X + 14, Y + 34, W - 28, 30, FL_ALIGN_LEFT);

    // Subtext
    if (!m_subtext.empty()) {
        fl_color(Theme::COLOR_TEXT_MUTED);
        fl_font(FL_HELVETICA, 10);
        fl_draw(m_subtext.c_str(), X + 14, Y + H - 22, W - 28, 14, FL_ALIGN_LEFT);
    }
}

// =============================================================================
// DataTable Implementation
// =============================================================================
DataTable::DataTable(int x, int y, int w, int h, const char* label)
    : Fl_Table_Row(x, y, w, h, label) {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_CARD_BG);
    col_header(1);
    col_header_height(32);
    col_resize(1);
    row_header(0);
    row_height_all(28);
    type(SELECT_SINGLE);
    callback(tableCallback, this);
    when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    end();
}

void DataTable::setHeaders(const std::vector<std::string>& headers, const std::vector<int>& widths) {
    m_headers = headers;
    cols((int)m_headers.size());
    m_colWidths = widths;

    int defaultW = (cols() > 0) ? std::max(60, (w() - 20) / cols()) : 100;
    for (int c = 0; c < cols(); ++c) {
        if (c < (int)m_colWidths.size() && m_colWidths[c] > 0) {
            col_width(c, m_colWidths[c]);
        } else {
            col_width(c, defaultW);
        }
    }
    redraw();
}

void DataTable::setData(const std::vector<std::vector<std::string>>& dataRows) {
    m_rows = dataRows;
    Fl_Table_Row::rows((int)m_rows.size());
    m_selectedRow = -1;
    select_all_rows(0);
    redraw();
}

void DataTable::addRow(const std::vector<std::string>& row) {
    m_rows.push_back(row);
    Fl_Table_Row::rows((int)m_rows.size());
    redraw();
}

void DataTable::clear() {
    m_rows.clear();
    Fl_Table_Row::rows(0);
    m_selectedRow = -1;
    select_all_rows(0);
    redraw();
}

void DataTable::setSelectedRow(int row) {
    m_selectedRow = row;
    select_all_rows(0);
    if (row >= 0 && row < (int)m_rows.size()) {
        select_row(row, 1);
    }
    redraw();
}

std::string DataTable::getCell(int row, int col) const {
    if (row >= 0 && row < (int)m_rows.size() && col >= 0 && col < (int)m_rows[row].size()) {
        return m_rows[row][col];
    }
    return "";
}

const std::vector<std::string>& DataTable::getRowData(int row) const {
    static const std::vector<std::string> emptyRow;
    if (row >= 0 && row < (int)m_rows.size()) {
        return m_rows[row];
    }
    return emptyRow;
}

int DataTable::handle(int event) {
    if (event == FL_PUSH && Fl::event_clicks()) {
        // Double click detection
        int r = callback_row();
        if (r >= 0 && r < (int)m_rows.size() && m_onDoubleClick) {
            m_onDoubleClick(r);
            return 1;
        }
    }
    return Fl_Table_Row::handle(event);
}

void DataTable::tableCallback(Fl_Widget* /*w*/, void* data) {
    DataTable* self = static_cast<DataTable*>(data);
    TableContext ctx = self->callback_context();
    if (ctx == CONTEXT_CELL || ctx == CONTEXT_ROW_HEADER) {
        int r = self->callback_row();
        if (r >= 0 && r < (int)self->m_rows.size()) {
            self->m_selectedRow = r;
            if (self->m_onSelection) {
                self->m_onSelection(r);
            }
        }
    }
}

void DataTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    switch (context) {
    case CONTEXT_COL_HEADER: {
        fl_push_clip(X, Y, W, H);
        fl_color(Theme::COLOR_TABLE_HEADER_BG);
        fl_rectf(X, Y, W, H);

        fl_color(Theme::COLOR_BORDER);
        fl_rect(X, Y, W, H);

        if (C >= 0 && C < (int)m_headers.size()) {
            fl_color(Theme::COLOR_TEXT_PRIMARY);
            fl_font(FL_HELVETICA_BOLD, 12);
            fl_draw(m_headers[C].c_str(), X + 8, Y, W - 16, H, FL_ALIGN_LEFT);
        }
        fl_pop_clip();
        break;
    }
    case CONTEXT_CELL: {
        fl_push_clip(X, Y, W, H);

        // Background color
        bool isSelected = (R == m_selectedRow) || row_selected(R);
        if (isSelected) {
            fl_color(Theme::COLOR_TABLE_ROW_SELECTED);
            fl_rectf(X, Y, W, H);
        } else {
            fl_color((R % 2 == 0) ? Theme::COLOR_TABLE_ROW_EVEN : Theme::COLOR_TABLE_ROW_ODD);
            fl_rectf(X, Y, W, H);
        }

        // Cell border
        fl_color(Theme::COLOR_BORDER);
        fl_line(X, Y + H - 1, X + W, Y + H - 1);
        fl_line(X + W - 1, Y, X + W - 1, Y + H);

        if (R >= 0 && R < (int)m_rows.size() && C >= 0 && C < (int)m_rows[R].size()) {
            const std::string& val = m_rows[R][C];

            // Badge detection for statuses
            if (!isSelected && (val == "Active" || val == "Present" || val == "Approved" || val == "Paid" || val == "Hired")) {
                fl_color(fl_rgb_color(20, 83, 45)); // Green badge bg
                fl_rectf(X + 6, Y + 4, W - 12, H - 8);
                fl_color(fl_rgb_color(134, 239, 172)); // Green text
                fl_font(FL_HELVETICA_BOLD, 11);
                fl_draw(val.c_str(), X + 6, Y + 4, W - 12, H - 8, FL_ALIGN_CENTER);
            } else if (!isSelected && (val == "Pending" || val == "Draft" || val == "Screening" || val == "Interviewing" || val == "Half Day" || val == "Late")) {
                fl_color(fl_rgb_color(113, 63, 18)); // Amber badge bg
                fl_rectf(X + 6, Y + 4, W - 12, H - 8);
                fl_color(fl_rgb_color(253, 224, 71)); // Amber text
                fl_font(FL_HELVETICA_BOLD, 11);
                fl_draw(val.c_str(), X + 6, Y + 4, W - 12, H - 8, FL_ALIGN_CENTER);
            } else if (!isSelected && (val == "Inactive" || val == "Absent" || val == "Rejected" || val == "Terminated")) {
                fl_color(fl_rgb_color(127, 29, 29)); // Red badge bg
                fl_rectf(X + 6, Y + 4, W - 12, H - 8);
                fl_color(fl_rgb_color(252, 165, 165)); // Red text
                fl_font(FL_HELVETICA_BOLD, 11);
                fl_draw(val.c_str(), X + 6, Y + 4, W - 12, H - 8, FL_ALIGN_CENTER);
            } else {
                fl_color(isSelected ? Theme::COLOR_TEXT_INVERTED : Theme::COLOR_TEXT_PRIMARY);
                fl_font(FL_HELVETICA, 12);
                fl_draw(val.c_str(), X + 8, Y, W - 16, H, FL_ALIGN_LEFT);
            }
        }
        fl_pop_clip();
        break;
    }
    default:
        break;
    }
}

// =============================================================================
// Dialog Helpers
// =============================================================================
void showInfo(const std::string& msg, const std::string& title) {
    fl_message_title(title.c_str());
    fl_message("%s", msg.c_str());
}

void showSuccess(const std::string& msg, const std::string& title) {
    fl_message_title(title.c_str());
    fl_message("%s", msg.c_str());
}

void showError(const std::string& msg, const std::string& title) {
    fl_message_title(title.c_str());
    fl_alert("%s", msg.c_str());
}

bool askConfirm(const std::string& msg, const std::string& title) {
    fl_message_title(title.c_str());
    return fl_choice("%s", "Cancel", "Confirm", nullptr, msg.c_str()) == 1;
}

std::string promptTextInput(const std::string& prompt, const std::string& defVal) {
    const char* val = fl_input("%s", defVal.c_str(), prompt.c_str());
    return val ? std::string(val) : "";
}

} // namespace hrms::gui
