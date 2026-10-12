#pragma once

#include "gui/gui_common.hpp"
#include "payroll/payroll.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class PayrollTab : public Fl_Group {
public:
    PayrollTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Input* m_inputMonth{nullptr};
    Fl_Button* m_btnLoad{nullptr};
    Fl_Button* m_btnProcess{nullptr};
    Fl_Button* m_btnMarkPaid{nullptr};
    Fl_Button* m_btnViewPayslip{nullptr};
    Fl_Button* m_btnRefresh{nullptr};

    StatCard* m_cardGross{nullptr};
    StatCard* m_cardDeductions{nullptr};
    StatCard* m_cardNet{nullptr};
    StatCard* m_cardStatus{nullptr};

    DataTable* m_table{nullptr};

    std::vector<pay::PayrollRecord> m_currentRecords;

    void openProcessDialog();
    void openPayslipDialog(const pay::PayrollRecord& record);

    static void onLoadClicked(Fl_Widget* w, void* data);
    static void onProcessClicked(Fl_Widget* w, void* data);
    static void onMarkPaidClicked(Fl_Widget* w, void* data);
    static void onViewPayslipClicked(Fl_Widget* w, void* data);
    static void onRefreshClicked(Fl_Widget* w, void* data);
    static void onTableDoubleClicked(int rowIndex, void* data);
};

} // namespace hrms::gui
