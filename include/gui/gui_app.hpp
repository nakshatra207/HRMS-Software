#pragma once

#include "gui/gui_common.hpp"
#include "gui/tab_dashboard.hpp"
#include "gui/tab_employees.hpp"
#include "gui/tab_attendance.hpp"
#include "gui/tab_leave.hpp"
#include "gui/tab_payroll.hpp"
#include "gui/tab_recruitment.hpp"
#include "gui/tab_departments.hpp"
#include "gui/tab_reports.hpp"
#include "gui/tab_settings.hpp"

#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class MainWindow : public Fl_Window {
public:
    MainWindow(int w, int h, const char* title);
    ~MainWindow() override;

    void updateSessionHeader();
    void refreshActiveTab();

private:
    // Header widgets
    Fl_Box* m_headerBox{nullptr};
    Fl_Box* m_logoBox{nullptr};
    Fl_Box* m_statusBadge{nullptr};
    Fl_Box* m_userBadge{nullptr};
    Fl_Box* m_clockBadge{nullptr};
    Fl_Button* m_btnLogout{nullptr};

    // Tabs container
    Fl_Tabs* m_tabs{nullptr};
    DashboardTab*   m_tabDashboard{nullptr};
    EmployeesTab*   m_tabEmployees{nullptr};
    AttendanceTab*  m_tabAttendance{nullptr};
    LeaveTab*       m_tabLeave{nullptr};
    PayrollTab*     m_tabPayroll{nullptr};
    RecruitmentTab* m_tabRecruitment{nullptr};
    DepartmentsTab* m_tabDepartments{nullptr};
    ReportsTab*     m_tabReports{nullptr};
    SettingsTab*    m_tabSettings{nullptr};

    // Bottom Status Bar
    Fl_Box* m_statusBar{nullptr};

    static void onLogoutClicked(Fl_Widget* w, void* data);
    static void onTabsChanged(Fl_Widget* w, void* data);
    static void timerCallback(void* data);
};

} // namespace hrms::gui
