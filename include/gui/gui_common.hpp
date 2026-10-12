#pragma once

#include "gui/gui_theme.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>

#include <string>
#include <vector>
#include <functional>

namespace hrms::gui {

// =============================================================================
// StatCard: Executive KPI Metric Card Widget
// =============================================================================
class StatCard : public Fl_Group {
public:
    StatCard(int x, int y, int w, int h, const char* title, const char* value, Fl_Color accentColor, const char* subtext = nullptr);

    void setValue(const std::string& val);
    void setSubtext(const std::string& sub);

protected:
    void draw() override;

private:
    std::string m_title;
    std::string m_value;
    std::string m_subtext;
    Fl_Color m_accentColor;
};

// =============================================================================
// DataTable: High-Performance Virtualized Table Widget
// =============================================================================
class DataTable : public Fl_Table_Row {
public:
    using SelectionCallback = std::function<void(int rowIndex)>;
    using DoubleClickCallback = std::function<void(int rowIndex)>;

    DataTable(int x, int y, int w, int h, const char* label = nullptr);

    void setHeaders(const std::vector<std::string>& headers, const std::vector<int>& widths = {});
    void setData(const std::vector<std::vector<std::string>>& rows);
    void addRow(const std::vector<std::string>& row);
    void clear();

    int getSelectedRow() const { return m_selectedRow; }
    void setSelectedRow(int row);
    std::string getCell(int row, int col) const;
    const std::vector<std::string>& getRowData(int row) const;
    size_t getRowCount() const { return m_rows.size(); }

    void onSelection(SelectionCallback cb) { m_onSelection = std::move(cb); }
    void onDoubleClick(DoubleClickCallback cb) { m_onDoubleClick = std::move(cb); }

    int handle(int event) override;

protected:
    void draw_cell(TableContext context, int R = 0, int C = 0, int X = 0, int Y = 0, int W = 0, int H = 0) override;

private:
    std::vector<std::string> m_headers;
    std::vector<int> m_colWidths;
    std::vector<std::vector<std::string>> m_rows;
    int m_selectedRow{-1};

    SelectionCallback m_onSelection;
    DoubleClickCallback m_onDoubleClick;

    static void tableCallback(Fl_Widget* w, void* data);
};

// =============================================================================
// Modal Dialog & Notification Helpers
// =============================================================================
void showInfo(const std::string& msg, const std::string& title = "Notice");
void showSuccess(const std::string& msg, const std::string& title = "Success");
void showError(const std::string& msg, const std::string& title = "Error");
bool askConfirm(const std::string& msg, const std::string& title = "Confirm");
std::string promptTextInput(const std::string& prompt, const std::string& defVal = "");

} // namespace hrms::gui
