#include "gui/tab_attendance.hpp"
#include "employees/employee.hpp"
#include "employees/department.hpp"
#include "security/auth.hpp"
#include "utilities/common_utils.hpp"
#include <FL/Fl_Window.H>
#include <sstream>
#include <iomanip>

namespace hrms::gui {

AttendanceTab::AttendanceTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Attendance") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // Row 1: Date & Department Filters
    int filterH = 32;

    Fl_Box* lDate = new Fl_Box(x + margin, curY, 40, filterH, "Date:");
    lDate->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDate->labelfont(FL_HELVETICA_BOLD);
    lDate->labelsize(12);

    m_inputDate = new Fl_Input(x + margin + 45, curY, 110, filterH);
    Theme::styleInput(m_inputDate);
    m_inputDate->value(utils::getCurrentDate().c_str());

    m_choiceDept = new Fl_Choice(x + margin + 165, curY, 160, filterH);
    Theme::styleChoice(m_choiceDept);
    m_choiceDept->add("All Departments");
    m_choiceDept->value(0);

    m_btnLoad = new Fl_Button(x + margin + 335, curY, 80, filterH, "Load");
    Theme::stylePrimaryButton(m_btnLoad);
    m_btnLoad->callback(onLoadClicked, this);

    m_btnToday = new Fl_Button(x + margin + 425, curY, 80, filterH, "Today");
    Theme::styleSecondaryButton(m_btnToday);
    m_btnToday->callback(onTodayClicked, this);

    // Management buttons aligned right
    int btnW = 140;
    int actX = x + w - margin;

    actX -= btnW;
    m_btnMonthlyStats = new Fl_Button(actX, curY, btnW, filterH, "📊 Monthly Stats");
    Theme::styleSecondaryButton(m_btnMonthlyStats);
    m_btnMonthlyStats->callback(onStatsClicked, this);

    actX -= (btnW + 10);
    m_btnManual = new Fl_Button(actX, curY, btnW, filterH, "⚙️ Manual Record");
    Theme::styleSecondaryButton(m_btnManual);
    m_btnManual->callback(onManualClicked, this);

    curY += filterH + 12;

    // Row 2: Self-Service Quick Check-In Bar
    Fl_Box* selfBox = new Fl_Box(x + margin, curY, w - (margin * 2), 44);
    selfBox->box(FL_FLAT_BOX);
    selfBox->color(Theme::COLOR_CARD_BG);

    m_btnCheckIn = new Fl_Button(x + margin + 12, curY + 6, 120, 32, "🟢 Check In");
    Theme::styleSuccessButton(m_btnCheckIn);
    m_btnCheckIn->callback(onCheckInClicked, this);

    m_btnCheckOut = new Fl_Button(x + margin + 140, curY + 6, 120, 32, "🔴 Check Out");
    Theme::styleDangerButton(m_btnCheckOut);
    m_btnCheckOut->callback(onCheckOutClicked, this);

    m_statusText = new Fl_Box(x + margin + 275, curY + 6, w - (margin * 2) - 290, 32, "");
    m_statusText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_statusText->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    m_statusText->labelfont(FL_HELVETICA);
    m_statusText->labelsize(12);

    curY += 44 + 14;

    // Row 3: Daily Attendance Data Table
    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);
    m_table->setHeaders(
        {"ID", "Emp Code", "Employee Name", "Department", "Check-In", "Check-Out", "Worked", "Late", "Overtime", "Status", "Notes"},
        {50, 90, 160, 140, 120, 120, 80, 70, 80, 90, 140}
    );

    end();
}

void AttendanceTab::refreshData() {
    auto depts = emp::DepartmentService::getAll();
    int currentDeptVal = m_choiceDept->value();
    m_choiceDept->clear();
    m_choiceDept->add("All Departments");
    for (const auto& d : depts) {
        m_choiceDept->add(d.name.c_str());
    }
    if (currentDeptVal >= 0 && currentDeptVal < m_choiceDept->size()) {
        m_choiceDept->value(currentDeptVal);
    } else {
        m_choiceDept->value(0);
    }

    std::string date = m_inputDate->value() ? m_inputDate->value() : utils::getCurrentDate();
    int deptId = 0;
    int deptIdx = m_choiceDept->value();
    if (deptIdx > 0 && deptIdx - 1 < (int)depts.size()) {
        deptId = depts[deptIdx - 1].id;
    }

    auto records = att::AttendanceService::getDailyAttendance(date, deptId);
    std::vector<std::vector<std::string>> rows;
    for (const auto& r : records) {
        int workedHours = r.totalMinutes / 60;
        int workedMins = r.totalMinutes % 60;
        std::ostringstream ossWorked;
        if (r.totalMinutes > 0) {
            ossWorked << workedHours << "h " << workedMins << "m";
        } else {
            ossWorked << "-";
        }

        rows.push_back({
            std::to_string(r.id),
            r.employeeCode,
            r.employeeName,
            r.departmentName,
            r.checkIn.empty() ? "-" : (r.checkIn.length() > 11 ? r.checkIn.substr(11, 8) : r.checkIn),
            r.checkOut.empty() ? "-" : (r.checkOut.length() > 11 ? r.checkOut.substr(11, 8) : r.checkOut),
            ossWorked.str(),
            r.lateMinutes > 0 ? (std::to_string(r.lateMinutes) + "m") : "-",
            r.overtimeMinutes > 0 ? (std::to_string(r.overtimeMinutes) + "m") : "-",
            r.status,
            r.notes
        });
    }
    m_table->setData(rows);

    // Update self-service status
    auto& auth = sec::AuthService::instance();
    if (auth.isLoggedIn()) {
        int empId = auth.currentSession()->employeeId;
        if (empId > 0) {
            auto todayRec = att::AttendanceService::getTodayRecord(empId);
            if (todayRec) {
                if (!todayRec->checkOut.empty()) {
                    m_statusText->copy_label(("Status: Checked out at " + todayRec->checkOut.substr(11, 8) + " (Shift Complete)").c_str());
                } else {
                    m_statusText->copy_label(("Status: Currently CHECKED IN since " + todayRec->checkIn.substr(11, 8)).c_str());
                }
            } else {
                m_statusText->copy_label("Status: Not checked in yet today.");
            }
        } else {
            m_statusText->copy_label("Logged in as Administrator (no individual employee record linked).");
        }
    }
}

void AttendanceTab::onLoadClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    self->refreshData();
}

void AttendanceTab::onTodayClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    self->m_inputDate->value(utils::getCurrentDate().c_str());
    self->refreshData();
}

void AttendanceTab::onCheckInClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    auto& auth = sec::AuthService::instance();
    if (!auth.isLoggedIn()) return;
    int empId = auth.currentSession()->employeeId;
    if (empId <= 0) {
        showError("Current user is not linked to an employee profile.");
        return;
    }
    std::string err;
    if (att::AttendanceService::checkIn(empId, err)) {
        showSuccess("Checked in successfully!");
        self->refreshData();
    } else {
        showError("Check-in failed: " + err);
    }
}

void AttendanceTab::onCheckOutClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    auto& auth = sec::AuthService::instance();
    if (!auth.isLoggedIn()) return;
    int empId = auth.currentSession()->employeeId;
    if (empId <= 0) {
        showError("Current user is not linked to an employee profile.");
        return;
    }
    std::string err;
    if (att::AttendanceService::checkOut(empId, err)) {
        showSuccess("Checked out successfully!");
        self->refreshData();
    } else {
        showError("Check-out failed: " + err);
    }
}

void AttendanceTab::onManualClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    self->openManualDialog();
}

void AttendanceTab::onStatsClicked(Fl_Widget* /*w*/, void* data) {
    AttendanceTab* self = static_cast<AttendanceTab*>(data);
    self->openMonthlyStatsDialog();
}

// =============================================================================
// Dialog: Manual Attendance Override
// =============================================================================
void AttendanceTab::openManualDialog() {
    Fl_Window win(500, 420, "Manual Attendance Entry / Override");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 500, 50, "Manual Attendance Override");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 440, fieldH = 30;

    // Employee choice
    Fl_Box* lEmp = new Fl_Box(30, curY, fieldW, 16, "Select Employee *");
    lEmp->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lEmp->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lEmp->labelsize(11);

    Fl_Choice chEmp(30, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chEmp);
    emp::EmployeeFilter filter;
    filter.status = "Active";
    filter.limit = 200;
    auto employees = emp::EmployeeService::search(filter);
    for (const auto& e : employees) {
        chEmp.add((e.employeeCode + " - " + e.fullName()).c_str());
    }
    chEmp.value(0);

    // Date
    curY += 56;
    Fl_Box* lDate = new Fl_Box(30, curY, 210, 16, "Date (YYYY-MM-DD) *");
    lDate->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDate->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDate->labelsize(11);
    Fl_Input inDate(30, curY + 18, 210, fieldH);
    Theme::styleInput(&inDate);
    inDate.value(utils::getCurrentDate().c_str());

    // Status
    Fl_Box* lStat = new Fl_Box(260, curY, 210, 16, "Status *");
    lStat->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lStat->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lStat->labelsize(11);
    Fl_Choice chStat(260, curY + 18, 210, fieldH);
    Theme::styleChoice(&chStat);
    chStat.add("Present|Absent|Half Day|On Leave");
    chStat.value(0);

    // Check-in & Check-out time
    curY += 56;
    Fl_Box* lIn = new Fl_Box(30, curY, 210, 16, "Check-In (HH:MM:SS)");
    lIn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lIn->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lIn->labelsize(11);
    Fl_Input inCheckIn(30, curY + 18, 210, fieldH);
    Theme::styleInput(&inCheckIn);
    inCheckIn.value("09:30:00");

    Fl_Box* lOut = new Fl_Box(260, curY, 210, 16, "Check-Out (HH:MM:SS)");
    lOut->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lOut->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lOut->labelsize(11);
    Fl_Input inCheckOut(260, curY + 18, 210, fieldH);
    Theme::styleInput(&inCheckOut);
    inCheckOut.value("17:30:00");

    // Notes
    curY += 56;
    Fl_Box* lNote = new Fl_Box(30, curY, fieldW, 16, "Reason / Override Notes");
    lNote->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lNote->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lNote->labelsize(11);
    Fl_Input inNotes(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inNotes);
    inNotes.value("Manual HR Adjustment");

    // Buttons
    curY += 65;
    Fl_Button btnSave(30, curY, 150, 36, "Save Attendance");
    Theme::stylePrimaryButton(&btnSave);

    Fl_Button btnCancel(190, curY, 100, 36, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool saved = false;
    btnSave.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &saved);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (saved) {
            int sel = chEmp.value();
            if (sel < 0 || sel >= (int)employees.size()) {
                showError("Please select a valid employee.");
                saved = false;
                continue;
            }
            int empId = employees[sel].id;
            std::string date = inDate.value() ? inDate.value() : "";
            std::string status = chStat.text();
            std::string cIn = inCheckIn.value() ? (date + " " + inCheckIn.value()) : "";
            std::string cOut = inCheckOut.value() ? (date + " " + inCheckOut.value()) : "";
            std::string notes = inNotes.value() ? inNotes.value() : "";

            std::string err;
            if (att::AttendanceService::recordManual(empId, date, cIn, cOut, status, notes, err)) {
                showSuccess("Attendance recorded successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed: " + err);
                saved = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Monthly Attendance Stats
// =============================================================================
void AttendanceTab::openMonthlyStatsDialog() {
    Fl_Window win(680, 480, "Monthly Attendance Statistics");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 680, 50, "Monthly Attendance Summary");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 60;
    Fl_Choice chEmp(30, curY, 260, 30);
    Theme::styleChoice(&chEmp);
    emp::EmployeeFilter filter;
    filter.status = "Active";
    filter.limit = 200;
    auto employees = emp::EmployeeService::search(filter);
    for (const auto& e : employees) {
        chEmp.add((e.employeeCode + " - " + e.fullName()).c_str());
    }
    chEmp.value(0);

    Fl_Input inMonth(305, curY, 120, 30);
    Theme::styleInput(&inMonth);
    inMonth.value(utils::getCurrentDate().substr(0, 7).c_str());

    Fl_Button btnCalc(435, curY, 110, 30, "Calculate");
    Theme::stylePrimaryButton(&btnCalc);

    curY += 40;
    Fl_Box* statsBanner = new Fl_Box(30, curY, 620, 40);
    statsBanner->box(FL_FLAT_BOX);
    statsBanner->color(Theme::COLOR_CARD_BG);
    statsBanner->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    statsBanner->labelfont(FL_HELVETICA_BOLD);
    statsBanner->labelsize(12);

    curY += 50;
    DataTable* table = new DataTable(30, curY, 620, 260);
    table->setHeaders({"Date", "Check-In", "Check-Out", "Worked", "Late", "Overtime", "Status"}, {90, 90, 90, 80, 70, 70, 90});

    auto updateStats = [&]() {
        int sel = chEmp.value();
        if (sel < 0 || sel >= (int)employees.size()) return;
        int empId = employees[sel].id;
        std::string m = inMonth.value() ? inMonth.value() : "";

        auto stats = att::AttendanceService::getMonthlyStats(empId, m);
        std::ostringstream oss;
        oss << "Work Days: " << stats.totalWorkDays
            << " | Present: " << stats.daysPresent
            << " | Absent: " << stats.daysAbsent
            << " | Late: " << stats.lateDays
            << " | Overtime: " << stats.totalOvertimeMinutes << " mins"
            << " | Attendance: " << std::fixed << std::setprecision(1) << stats.attendancePercentage << "%";
        statsBanner->copy_label(oss.str().c_str());
        statsBanner->redraw();

        auto records = att::AttendanceService::getMonthlyAttendance(empId, m);
        std::vector<std::vector<std::string>> rows;
        for (const auto& r : records) {
            int hrs = r.totalMinutes / 60, mins = r.totalMinutes % 60;
            rows.push_back({
                r.attendanceDate,
                r.checkIn.length() > 11 ? r.checkIn.substr(11, 8) : r.checkIn,
                r.checkOut.length() > 11 ? r.checkOut.substr(11, 8) : r.checkOut,
                std::to_string(hrs) + "h " + std::to_string(mins) + "m",
                r.lateMinutes > 0 ? (std::to_string(r.lateMinutes) + "m") : "-",
                r.overtimeMinutes > 0 ? (std::to_string(r.overtimeMinutes) + "m") : "-",
                r.status
            });
        }
        table->setData(rows);
    };

    updateStats();

    btnCalc.callback([](Fl_Widget*, void* data) {
        auto fn = static_cast<decltype(updateStats)*>(data);
        (*fn)();
    }, &updateStats);

    Fl_Button* btnClose = new Fl_Button(280, 425, 120, 34, "Close");
    Theme::styleSecondaryButton(btnClose);
    btnClose->callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();
    while (win.shown()) {
        Fl::wait();
    }
}

} // namespace hrms::gui
