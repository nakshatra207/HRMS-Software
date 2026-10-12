#include "gui/tab_dashboard.hpp"
#include "employees/employee.hpp"
#include "employees/department.hpp"
#include "attendance/attendance.hpp"
#include "leave/leave.hpp"
#include "payroll/payroll.hpp"
#include "recruitment/recruitment.hpp"
#include "security/auth.hpp"
#include "security/audit.hpp"
#include "utilities/common_utils.hpp"
#include "utilities/fast_ops.hpp"

#include <sstream>
#include <iomanip>

extern "C" {
#include "posix_sys.h"
}

namespace hrms::gui {

DashboardTab::DashboardTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Dashboard") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // 1. Metric Cards Grid (6 cards across 2 rows or 1 row depending on width)
    int cardCount = 6;
    int cardSpacing = 12;
    int cardW = (w - (margin * 2) - ((cardCount - 1) * cardSpacing)) / cardCount;
    int cardH = 85;

    m_cardEmployees = new StatCard(x + margin + 0 * (cardW + cardSpacing), curY, cardW, cardH, "Total Staff", "0", Theme::COLOR_PRIMARY, "Active records");
    m_cardPresent   = new StatCard(x + margin + 1 * (cardW + cardSpacing), curY, cardW, cardH, "Present Today", "0", Theme::COLOR_SUCCESS, "Checked-in");
    m_cardAbsent    = new StatCard(x + margin + 2 * (cardW + cardSpacing), curY, cardW, cardH, "Absent Today", "0", Theme::COLOR_DANGER, "No check-in");
    m_cardLeaves    = new StatCard(x + margin + 3 * (cardW + cardSpacing), curY, cardW, cardH, "Pending Leaves", "0", Theme::COLOR_WARNING, "Awaiting approval");
    m_cardVacancies = new StatCard(x + margin + 4 * (cardW + cardSpacing), curY, cardW, cardH, "Open Vacancies", "0", Theme::COLOR_PURPLE, "Job positions");
    m_cardPayroll   = new StatCard(x + margin + 5 * (cardW + cardSpacing), curY, cardW, cardH, "Monthly Payroll", "₹ 0", Theme::COLOR_CYAN, "Current billing");

    curY += cardH + 16;

    // 2. Action Bar
    int barH = 34;
    m_btnCheckIn = new Fl_Button(x + margin, curY, 130, barH, "⚡ Check In");
    Theme::styleSuccessButton(m_btnCheckIn);
    m_btnCheckIn->callback(onCheckInClicked, this);

    m_btnCheckOut = new Fl_Button(x + margin + 140, curY, 130, barH, "⚡ Check Out");
    Theme::styleDangerButton(m_btnCheckOut);
    m_btnCheckOut->callback(onCheckOutClicked, this);

    m_btnRefresh = new Fl_Button(x + w - margin - 120, curY, 120, barH, "🔄 Refresh Data");
    Theme::styleSecondaryButton(m_btnRefresh);
    m_btnRefresh->callback(onRefreshClicked, this);

    curY += barH + 16;

    // 3. Middle Section: Two Tables (Left: Departments Summary, Right: Recent Audit Activity)
    int availH = h - (curY - y) - margin - 40;
    int colSpacing = 16;
    int tableW = (w - (margin * 2) - colSpacing) / 2;

    // Left: Departments
    Fl_Box* deptTitle = new Fl_Box(x + margin, curY, tableW, 22, "Department Overview & Budget");
    deptTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    deptTitle->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    deptTitle->labelfont(FL_HELVETICA_BOLD);
    deptTitle->labelsize(13);

    m_deptTable = new DataTable(x + margin, curY + 24, tableW, availH);
    m_deptTable->setHeaders({"Code", "Department", "Manager", "Headcount"}, {70, 160, 160, 90});

    // Right: Audit Logs
    Fl_Box* auditTitle = new Fl_Box(x + margin + tableW + colSpacing, curY, tableW, 22, "Recent Security & Audit Activity");
    auditTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    auditTitle->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    auditTitle->labelfont(FL_HELVETICA_BOLD);
    auditTitle->labelsize(13);

    m_auditTable = new DataTable(x + margin + tableW + colSpacing, curY + 24, tableW, availH);
    m_auditTable->setHeaders({"Time", "User", "Action", "Details"}, {120, 90, 100, 200});

    // 4. Bottom System Diagnostics Footer
    curY += availH + 28;
    m_diagBox = new Fl_Box(x + margin, curY, w - (margin * 2), 26);
    m_diagBox->box(FL_FLAT_BOX);
    m_diagBox->color(Theme::COLOR_CARD_BG);
    m_diagBox->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    m_diagBox->labelfont(FL_HELVETICA);
    m_diagBox->labelsize(11);
    m_diagBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    end();
}

void DashboardTab::refreshData() {
    emp::EmployeeFilter filter;
    filter.status = "Active";
    int activeEmpCount = emp::EmployeeService::count(filter);
    m_cardEmployees->setValue(std::to_string(activeEmpCount));

    // Attendance today
    std::string today = utils::getCurrentDate();
    auto todayAtt = att::AttendanceService::getDailyAttendance(today);
    int presentCount = 0;
    for (const auto& r : todayAtt) {
        if (r.status == "Present" || r.status == "Late") {
            presentCount++;
        }
    }
    m_cardPresent->setValue(std::to_string(presentCount));
    int absentCount = std::max(0, activeEmpCount - presentCount);
    m_cardAbsent->setValue(std::to_string(absentCount));

    // Pending Leaves
    auto pendingLeaves = leave::LeaveService::getRequests(0, "Pending");
    m_cardLeaves->setValue(std::to_string(pendingLeaves.size()));

    // Open Job Vacancies
    auto jobs = rec::RecruitmentService::getJobOpenings("Open");
    int totalVacancies = 0;
    for (const auto& j : jobs) {
        totalVacancies += j.vacancies;
    }
    m_cardVacancies->setValue(std::to_string(totalVacancies));

    // Payroll Spend for current month
    std::string currentMonth = today.substr(0, 7);
    auto payrollRecords = pay::PayrollService::getPeriodRecords(currentMonth);
    int64_t totalNetPaise = 0;
    for (const auto& pr : payrollRecords) {
        totalNetPaise += pr.netSalaryPaise;
    }
    if (totalNetPaise > 0) {
        m_cardPayroll->setValue(utils::formatPaise(totalNetPaise));
        m_cardPayroll->setSubtext("Batch processed (" + currentMonth + ")");
    } else {
        m_cardPayroll->setValue("Draft");
        m_cardPayroll->setSubtext("Pending run (" + currentMonth + ")");
    }

    // Departments Table
    auto depts = emp::DepartmentService::getAll();
    std::vector<std::vector<std::string>> deptRows;
    for (const auto& d : depts) {
        deptRows.push_back({
            d.code,
            d.name,
            d.managerName.empty() ? "-" : d.managerName,
            std::to_string(d.employeeCount) + " staff"
        });
    }
    m_deptTable->setData(deptRows);

    // Audit Activity Table
    auto logs = sec::AuditLogger::getRecentLogs(15);
    std::vector<std::vector<std::string>> logRows;
    for (const auto& l : logs) {
        logRows.push_back({
            l.createdAt.length() > 10 ? l.createdAt.substr(11, 8) : l.createdAt,
            l.username,
            l.action,
            l.details
        });
    }
    m_auditTable->setData(logRows);

    // System Diagnostics Bar
    size_t rssKb = 0, vmsKb = 0;
    get_process_memory_usage(&rssKb, &vmsKb);
    double rssMb = rssKb / 1024.0;

    std::ostringstream oss;
    oss << "  Engine: SQLite 3.46 (WAL Mode, Parameterized)  |  NASM Vectorized x86-64: "
        << (utils::is_assembly_active() ? "ACTIVE (AMD64)" : "FALLBACK (C++)")
        << "  |  Memory: " << std::fixed << std::setprecision(1) << rssMb << " MB RSS  |  Status: Operational";
    m_diagBox->copy_label(oss.str().c_str());
    m_diagBox->redraw();
}

void DashboardTab::onCheckInClicked(Fl_Widget* /*w*/, void* data) {
    DashboardTab* self = static_cast<DashboardTab*>(data);
    auto& auth = sec::AuthService::instance();
    if (!auth.isLoggedIn()) return;

    int empId = auth.currentSession()->employeeId;
    if (empId <= 0) {
        showError("Current user account is not linked to an employee profile.");
        return;
    }

    std::string err;
    if (att::AttendanceService::checkIn(empId, err)) {
        showSuccess("Checked in successfully for today!");
        self->refreshData();
    } else {
        showError("Check-in failed: " + err);
    }
}

void DashboardTab::onCheckOutClicked(Fl_Widget* /*w*/, void* data) {
    DashboardTab* self = static_cast<DashboardTab*>(data);
    auto& auth = sec::AuthService::instance();
    if (!auth.isLoggedIn()) return;

    int empId = auth.currentSession()->employeeId;
    if (empId <= 0) {
        showError("Current user account is not linked to an employee profile.");
        return;
    }

    std::string err;
    if (att::AttendanceService::checkOut(empId, err)) {
        showSuccess("Checked out successfully! Total hours recorded.");
        self->refreshData();
    } else {
        showError("Check-out failed: " + err);
    }
}

void DashboardTab::onRefreshClicked(Fl_Widget* /*w*/, void* data) {
    DashboardTab* self = static_cast<DashboardTab*>(data);
    self->refreshData();
}

} // namespace hrms::gui
