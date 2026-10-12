#include "gui/tab_reports.hpp"
#include "employees/department.hpp"
#include "utilities/common_utils.hpp"
#include <filesystem>

namespace hrms::gui {

ReportsTab::ReportsTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Reports") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    int filterH = 32;

    Fl_Box* lRep = new Fl_Box(x + margin, curY, 80, filterH, "Report:");
    lRep->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lRep->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lRep->labelfont(FL_HELVETICA_BOLD);
    lRep->labelsize(12);

    m_choiceReport = new Fl_Choice(x + margin + 85, curY, 260, filterH);
    Theme::styleChoice(m_choiceReport);
    m_choiceReport->add("1. Employee Directory Report");
    m_choiceReport->add("2. Monthly Attendance Summary");
    m_choiceReport->add("3. Annual Leave Utilization");
    m_choiceReport->add("4. Monthly Payroll Register");
    m_choiceReport->add("5. Department Headcount & Budget");
    m_choiceReport->add("6. Recruitment Pipeline Funnel");
    m_choiceReport->value(0);
    m_choiceReport->callback(onReportChanged, this);

    m_inputMonth = new Fl_Input(x + margin + 355, curY, 100, filterH);
    Theme::styleInput(m_inputMonth);
    m_inputMonth->value(utils::getCurrentDate().substr(0, 7).c_str());

    m_choiceDept = new Fl_Choice(x + margin + 465, curY, 150, filterH);
    Theme::styleChoice(m_choiceDept);
    m_choiceDept->add("All Departments");
    m_choiceDept->value(0);

    m_btnGenerate = new Fl_Button(x + margin + 625, curY, 110, filterH, "📊 Generate");
    Theme::stylePrimaryButton(m_btnGenerate);
    m_btnGenerate->callback(onGenerateClicked, this);

    m_btnExportCsv = new Fl_Button(x + w - margin - 140, curY, 140, filterH, "💾 Export to CSV");
    Theme::styleSuccessButton(m_btnExportCsv);
    m_btnExportCsv->callback(onExportClicked, this);

    curY += filterH + 14;

    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);

    end();
}

void ReportsTab::refreshData() {
    auto depts = emp::DepartmentService::getAll();
    int curDeptVal = m_choiceDept->value();
    m_choiceDept->clear();
    m_choiceDept->add("All Departments");
    for (const auto& d : depts) m_choiceDept->add(d.name.c_str());
    if (curDeptVal >= 0 && curDeptVal < m_choiceDept->size()) m_choiceDept->value(curDeptVal);
    else m_choiceDept->value(0);

    generateCurrentReport();
}

void ReportsTab::generateCurrentReport() {
    int repType = m_choiceReport->value();
    std::string month = m_inputMonth->value() ? m_inputMonth->value() : utils::getCurrentDate().substr(0, 7);

    auto depts = emp::DepartmentService::getAll();
    int deptIdx = m_choiceDept->value();
    int deptId = (deptIdx > 0 && deptIdx - 1 < (int)depts.size()) ? depts[deptIdx - 1].id : 0;

    switch (repType) {
    case 0: // Employee Directory
        m_currentReport = rep::ReportService::getEmployeeReport(deptId);
        break;
    case 1: // Attendance Summary
        m_currentReport = rep::ReportService::getAttendanceReport(month, deptId);
        break;
    case 2: // Leave Utilization
        m_currentReport = rep::ReportService::getLeaveReport(2026);
        break;
    case 3: // Payroll Register
        m_currentReport = rep::ReportService::getPayrollReport(month);
        break;
    case 4: // Department Budget
        m_currentReport = rep::ReportService::getDepartmentReport();
        break;
    case 5: // Recruitment Funnel
        m_currentReport = rep::ReportService::getRecruitmentReport();
        break;
    default:
        break;
    }

    m_table->setHeaders(m_currentReport.headers);
    m_table->setData(m_currentReport.rows);
}

void ReportsTab::exportCsv() {
    if (m_currentReport.rows.empty()) {
        showError("No report data available to export. Please generate a report first.");
        return;
    }

    std::filesystem::create_directories("reports");
    std::string repName = "report";
    switch (m_choiceReport->value()) {
        case 0: repName = "employees"; break;
        case 1: repName = "attendance"; break;
        case 2: repName = "leave"; break;
        case 3: repName = "payroll"; break;
        case 4: repName = "departments"; break;
        case 5: repName = "recruitment"; break;
    }
    std::string fileName = "reports/" + repName + "_" + utils::getCurrentDate() + ".csv";

    std::string err;
    if (rep::ReportService::exportToCsv(m_currentReport, fileName, err)) {
        showSuccess("Report successfully exported to:\n" + fileName);
    } else {
        showError("Failed to export report: " + err);
    }
}

void ReportsTab::onGenerateClicked(Fl_Widget* /*w*/, void* data) {
    ReportsTab* self = static_cast<ReportsTab*>(data);
    self->generateCurrentReport();
}

void ReportsTab::onExportClicked(Fl_Widget* /*w*/, void* data) {
    ReportsTab* self = static_cast<ReportsTab*>(data);
    self->exportCsv();
}

void ReportsTab::onReportChanged(Fl_Widget* /*w*/, void* data) {
    ReportsTab* self = static_cast<ReportsTab*>(data);
    self->generateCurrentReport();
}

} // namespace hrms::gui
