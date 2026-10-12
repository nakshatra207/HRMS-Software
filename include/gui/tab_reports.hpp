#pragma once

#include "gui/gui_common.hpp"
#include "reports/reports.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class ReportsTab : public Fl_Group {
public:
    ReportsTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Choice* m_choiceReport{nullptr};
    Fl_Input* m_inputMonth{nullptr};
    Fl_Choice* m_choiceDept{nullptr};

    Fl_Button* m_btnGenerate{nullptr};
    Fl_Button* m_btnExportCsv{nullptr};

    DataTable* m_table{nullptr};

    rep::ReportTable m_currentReport;

    void generateCurrentReport();
    void exportCsv();

    static void onGenerateClicked(Fl_Widget* w, void* data);
    static void onExportClicked(Fl_Widget* w, void* data);
    static void onReportChanged(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
