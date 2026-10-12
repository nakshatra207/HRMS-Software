#include "gui/gui_app.hpp"
#include "gui/dialog_login.hpp"
#include "security/auth.hpp"
#include "security/rbac.hpp"
#include "utilities/common_utils.hpp"
#include "utilities/fast_ops.hpp"
#include <sstream>
#include <iomanip>

extern "C" {
#include "posix_sys.h"
}

namespace hrms::gui {

MainWindow::MainWindow(int w, int h, const char* title)
    : Fl_Window(w, h, title) {
    color(Theme::COLOR_BG);
    size_range(1000, 640);

    int headerH = 60;
    int footerH = 26;

    // 1. Header Bar
    m_headerBox = new Fl_Box(0, 0, w, headerH);
    m_headerBox->box(FL_FLAT_BOX);
    m_headerBox->color(Theme::COLOR_HEADER_BG);

    m_logoBox = new Fl_Box(20, 10, 280, 40, "HRMS  |  Enterprise Suite");
    m_logoBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_logoBox->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    m_logoBox->labelfont(FL_HELVETICA_BOLD);
    m_logoBox->labelsize(17);

    // Center status pills
    m_statusBadge = new Fl_Box(310, 15, 260, 30, "");
    m_statusBadge->box(FL_FLAT_BOX);
    m_statusBadge->color(Theme::COLOR_CARD_BG);
    m_statusBadge->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    m_statusBadge->labelfont(FL_HELVETICA_BOLD);
    m_statusBadge->labelsize(11);

    m_clockBadge = new Fl_Box(580, 15, 180, 30, "");
    m_clockBadge->box(FL_FLAT_BOX);
    m_clockBadge->color(Theme::COLOR_CARD_BG);
    m_clockBadge->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    m_clockBadge->labelfont(FL_HELVETICA);
    m_clockBadge->labelsize(11);

    // Right user & logout
    m_userBadge = new Fl_Box(w - 380, 15, 250, 30, "");
    m_userBadge->box(FL_FLAT_BOX);
    m_userBadge->color(Theme::COLOR_CARD_BG);
    m_userBadge->labelcolor(Theme::COLOR_SUCCESS);
    m_userBadge->labelfont(FL_HELVETICA_BOLD);
    m_userBadge->labelsize(11);

    m_btnLogout = new Fl_Button(w - 110, 15, 90, 30, "Sign Out");
    Theme::styleSecondaryButton(m_btnLogout);
    m_btnLogout->callback(onLogoutClicked, this);

    // 2. Navigation Tabs Container
    int tabY = headerH;
    int tabH = h - headerH - footerH;

    m_tabs = new Fl_Tabs(0, tabY, w, tabH);
    Theme::styleTabs(m_tabs);
    m_tabs->callback(onTabsChanged, this);

    int contentY = tabY + 32;
    int contentH = tabH - 32;

    m_tabDashboard   = new DashboardTab(0, contentY, w, contentH);
    m_tabEmployees   = new EmployeesTab(0, contentY, w, contentH);
    m_tabAttendance  = new AttendanceTab(0, contentY, w, contentH);
    m_tabLeave       = new LeaveTab(0, contentY, w, contentH);
    m_tabPayroll     = new PayrollTab(0, contentY, w, contentH);
    m_tabRecruitment = new RecruitmentTab(0, contentY, w, contentH);
    m_tabDepartments = new DepartmentsTab(0, contentY, w, contentH);
    m_tabReports     = new ReportsTab(0, contentY, w, contentH);
    m_tabSettings    = new SettingsTab(0, contentY, w, contentH);

    m_tabs->end();

    // 3. Status Bar
    m_statusBar = new Fl_Box(0, h - footerH, w, footerH, "  Database: data/hrms.db (SQLite3 WAL)  |  Security: RBAC Authenticated  |  Mode: GUI Native");
    m_statusBar->box(FL_FLAT_BOX);
    m_statusBar->color(Theme::COLOR_HEADER_BG);
    m_statusBar->labelcolor(Theme::COLOR_TEXT_MUTED);
    m_statusBar->labelfont(FL_HELVETICA);
    m_statusBar->labelsize(11);
    m_statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    resizable(m_tabs);
    end();

    updateSessionHeader();
    m_tabDashboard->refreshData();

    // Add periodic 2s timer for clock & memory stats
    Fl::add_timeout(2.0, timerCallback, this);
}

MainWindow::~MainWindow() {
    Fl::remove_timeout(timerCallback, this);
}

void MainWindow::updateSessionHeader() {
    auto& auth = sec::AuthService::instance();
    if (auth.isLoggedIn()) {
        const auto& sess = *auth.currentSession();
        std::string roleStr = sec::RBAC::roleToString(sess.role);
        std::string userText = "👤 " + sess.username + " (" + roleStr + ")";
        m_userBadge->copy_label(userText.c_str());

        // Role-based view restrictions:
        // If employee role, restrict managerial tabs
        if (sess.role == sec::Role::Employee) {
            m_tabPayroll->deactivate();
            m_tabRecruitment->deactivate();
            m_tabSettings->deactivate();
        } else {
            m_tabPayroll->activate();
            m_tabRecruitment->activate();
            m_tabSettings->activate();
        }
    }

    // Diagnostics badge
    size_t rssKb = 0, vmsKb = 0;
    get_process_memory_usage(&rssKb, &vmsKb);
    double rssMb = rssKb / 1024.0;

    std::ostringstream oss;
    oss << "⚡ NASM: " << (utils::is_assembly_active() ? "Active" : "Pure C++")
        << "  |  RAM: " << std::fixed << std::setprecision(1) << rssMb << " MB";
    m_statusBadge->copy_label(oss.str().c_str());

    m_clockBadge->copy_label(utils::getCurrentDateTime().c_str());
}

void MainWindow::refreshActiveTab() {
    Fl_Widget* cur = m_tabs->value();
    if (cur == m_tabDashboard)   m_tabDashboard->refreshData();
    else if (cur == m_tabEmployees)   m_tabEmployees->refreshData();
    else if (cur == m_tabAttendance)  m_tabAttendance->refreshData();
    else if (cur == m_tabLeave)       m_tabLeave->refreshData();
    else if (cur == m_tabPayroll)     m_tabPayroll->refreshData();
    else if (cur == m_tabRecruitment) m_tabRecruitment->refreshData();
    else if (cur == m_tabDepartments) m_tabDepartments->refreshData();
    else if (cur == m_tabReports)     m_tabReports->refreshData();
    else if (cur == m_tabSettings)    m_tabSettings->refreshData();
}

void MainWindow::onLogoutClicked(Fl_Widget* /*w*/, void* data) {
    MainWindow* self = static_cast<MainWindow*>(data);
    if (!askConfirm("Are you sure you want to sign out?")) {
        return;
    }
    sec::AuthService::instance().logout();
    self->hide();

    // Re-prompt login
    LoginDialog login;
    if (login.execute()) {
        self->updateSessionHeader();
        self->refreshActiveTab();
        self->show();
    }
}

void MainWindow::onTabsChanged(Fl_Widget* /*w*/, void* data) {
    MainWindow* self = static_cast<MainWindow*>(data);
    self->refreshActiveTab();
}

void MainWindow::timerCallback(void* data) {
    MainWindow* self = static_cast<MainWindow*>(data);
    self->updateSessionHeader();
    Fl::repeat_timeout(2.0, timerCallback, data);
}

} // namespace hrms::gui
