#pragma once

#include "gui/gui_common.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

namespace hrms::gui {

class DashboardTab : public Fl_Group {
public:
    DashboardTab(int x, int y, int w, int h);

    void refreshData();

private:
    // Stat cards
    StatCard* m_cardEmployees{nullptr};
    StatCard* m_cardPresent{nullptr};
    StatCard* m_cardAbsent{nullptr};
    StatCard* m_cardLeaves{nullptr};
    StatCard* m_cardVacancies{nullptr};
    StatCard* m_cardPayroll{nullptr};

    // Quick Actions
    Fl_Button* m_btnCheckIn{nullptr};
    Fl_Button* m_btnCheckOut{nullptr};
    Fl_Button* m_btnRefresh{nullptr};

    // Tables
    DataTable* m_deptTable{nullptr};
    DataTable* m_auditTable{nullptr};

    // Diagnostics Box
    Fl_Box* m_diagBox{nullptr};

    static void onCheckInClicked(Fl_Widget* w, void* data);
    static void onCheckOutClicked(Fl_Widget* w, void* data);
    static void onRefreshClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
