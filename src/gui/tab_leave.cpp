#include "gui/tab_leave.hpp"
#include "employees/employee.hpp"
#include "security/auth.hpp"
#include "utilities/common_utils.hpp"
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <sstream>
#include <iomanip>

namespace hrms::gui {

LeaveTab::LeaveTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Leaves") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // Row 1: Employee balance selector
    int filterH = 32;
    Fl_Box* lEmp = new Fl_Box(x + margin, curY, 110, filterH, "Staff Balances:");
    lEmp->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lEmp->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lEmp->labelfont(FL_HELVETICA_BOLD);
    lEmp->labelsize(12);

    m_choiceEmp = new Fl_Choice(x + margin + 115, curY, 260, filterH);
    Theme::styleChoice(m_choiceEmp);
    m_choiceEmp->callback(onEmpChanged, this);

    m_btnLoadBalances = new Fl_Button(x + margin + 385, curY, 80, filterH, "View");
    Theme::styleSecondaryButton(m_btnLoadBalances);
    m_btnLoadBalances->callback(onEmpChanged, this);

    // Action buttons aligned right
    int btnW = 120;
    int actX = x + w - margin;

    actX -= btnW;
    m_btnRefresh = new Fl_Button(actX, curY, btnW, filterH, "🔄 Refresh");
    Theme::styleSecondaryButton(m_btnRefresh);
    m_btnRefresh->callback(onRefreshClicked, this);

    actX -= (btnW + 8);
    m_btnReject = new Fl_Button(actX, curY, btnW, filterH, "❌ Reject");
    Theme::styleDangerButton(m_btnReject);
    m_btnReject->callback(onRejectClicked, this);

    actX -= (btnW + 8);
    m_btnApprove = new Fl_Button(actX, curY, btnW, filterH, "✅ Approve");
    Theme::styleSuccessButton(m_btnApprove);
    m_btnApprove->callback(onApproveClicked, this);

    actX -= (btnW + 8);
    m_btnApply = new Fl_Button(actX, curY, btnW + 10, filterH, "🌴 Apply Leave");
    Theme::stylePrimaryButton(m_btnApply);
    m_btnApply->callback(onApplyClicked, this);

    curY += filterH + 14;

    // Row 2: 4 Leave Balance Cards
    int cardCount = 4;
    int cardSpacing = 12;
    int cardW = (w - (margin * 2) - ((cardCount - 1) * cardSpacing)) / cardCount;
    int cardH = 75;

    m_cardSL = new StatCard(x + margin + 0 * (cardW + cardSpacing), curY, cardW, cardH, "Sick Leave (SL)", "0 Days", Theme::COLOR_PRIMARY, "Remaining");
    m_cardCL = new StatCard(x + margin + 1 * (cardW + cardSpacing), curY, cardW, cardH, "Casual Leave (CL)", "0 Days", Theme::COLOR_CYAN, "Remaining");
    m_cardPL = new StatCard(x + margin + 2 * (cardW + cardSpacing), curY, cardW, cardH, "Paid Leave (PL)", "0 Days", Theme::COLOR_SUCCESS, "Remaining");
    m_cardUL = new StatCard(x + margin + 3 * (cardW + cardSpacing), curY, cardW, cardH, "Unpaid Leave (UL)", "0 Days", Theme::COLOR_WARNING, "Used");

    curY += cardH + 16;

    // Row 3: Split Tables (Pending Requests on Top 45%, All History on Bottom 55%)
    int availH = h - (curY - y) - margin;
    int pendingH = (availH * 45) / 100 - 30;
    int historyH = availH - pendingH - 50;

    Fl_Box* pTitle = new Fl_Box(x + margin, curY, w - (margin * 2), 20, "Pending Approval Requests Queue");
    pTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    pTitle->labelcolor(Theme::COLOR_WARNING);
    pTitle->labelfont(FL_HELVETICA_BOLD);
    pTitle->labelsize(12);

    m_pendingTable = new DataTable(x + margin, curY + 22, w - (margin * 2), pendingH);
    m_pendingTable->setHeaders(
        {"ID", "Emp Code", "Employee Name", "Department", "Leave Type", "Start Date", "End Date", "Days", "Reason", "Applied On"},
        {50, 90, 160, 140, 100, 100, 100, 60, 200, 110}
    );

    curY += 22 + pendingH + 12;

    Fl_Box* hTitle = new Fl_Box(x + margin, curY, w - (margin * 2), 20, "All Leave Requests & History");
    hTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    hTitle->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    hTitle->labelfont(FL_HELVETICA_BOLD);
    hTitle->labelsize(12);

    m_historyTable = new DataTable(x + margin, curY + 22, w - (margin * 2), historyH);
    m_historyTable->setHeaders(
        {"ID", "Emp Code", "Employee Name", "Leave Type", "Start Date", "End Date", "Days", "Reason", "Status", "Decision By"},
        {50, 90, 160, 100, 100, 100, 60, 200, 90, 120}
    );

    end();
}

void LeaveTab::refreshData() {
    // Populate employee choice if empty
    emp::EmployeeFilter filter;
    filter.status = "Active";
    filter.limit = 200;
    auto employees = emp::EmployeeService::search(filter);

    int currentEmpVal = m_choiceEmp->value();
    m_choiceEmp->clear();
    for (const auto& e : employees) {
        m_choiceEmp->add((e.employeeCode + " - " + e.fullName()).c_str());
    }

    auto& auth = sec::AuthService::instance();
    int currentSessionEmpId = auth.isLoggedIn() ? auth.currentSession()->employeeId : 0;

    int selIdx = 0;
    if (currentEmpVal >= 0 && currentEmpVal < (int)employees.size()) {
        selIdx = currentEmpVal;
    } else if (currentSessionEmpId > 0) {
        for (size_t i = 0; i < employees.size(); ++i) {
            if (employees[i].id == currentSessionEmpId) {
                selIdx = (int)i;
                break;
            }
        }
    }
    if (!employees.empty()) {
        m_choiceEmp->value(selIdx);
        int targetEmpId = employees[selIdx].id;
        auto balances = leave::LeaveService::getBalances(targetEmpId, 2026);
        for (const auto& b : balances) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << b.remainingDays() << " / " << b.totalDays << " d";
            if (b.leaveTypeCode == "SL") {
                m_cardSL->setValue(oss.str());
                m_cardSL->setSubtext("Used: " + std::to_string((int)b.usedDays) + " | Pending: " + std::to_string((int)b.pendingDays));
            } else if (b.leaveTypeCode == "CL") {
                m_cardCL->setValue(oss.str());
                m_cardCL->setSubtext("Used: " + std::to_string((int)b.usedDays) + " | Pending: " + std::to_string((int)b.pendingDays));
            } else if (b.leaveTypeCode == "PL") {
                m_cardPL->setValue(oss.str());
                m_cardPL->setSubtext("Used: " + std::to_string((int)b.usedDays) + " | Pending: " + std::to_string((int)b.pendingDays));
            } else if (b.leaveTypeCode == "UL") {
                m_cardUL->setValue(std::to_string((int)b.usedDays) + " Days");
                m_cardUL->setSubtext("Unpaid days taken");
            }
        }
    }

    // Pending Requests
    m_pendingRequests = leave::LeaveService::getRequests(0, "Pending");
    std::vector<std::vector<std::string>> pendingRows;
    for (const auto& r : m_pendingRequests) {
        std::ostringstream ossDays;
        ossDays << std::fixed << std::setprecision(1) << r.totalDays;
        pendingRows.push_back({
            std::to_string(r.id),
            r.employeeCode,
            r.employeeName,
            r.departmentName,
            r.leaveTypeName,
            r.startDate,
            r.endDate,
            ossDays.str(),
            r.reason,
            r.createdAt.length() > 10 ? r.createdAt.substr(0, 10) : r.createdAt
        });
    }
    m_pendingTable->setData(pendingRows);

    // All Requests
    m_allRequests = leave::LeaveService::getRequests(0, "");
    std::vector<std::vector<std::string>> historyRows;
    for (const auto& r : m_allRequests) {
        std::ostringstream ossDays;
        ossDays << std::fixed << std::setprecision(1) << r.totalDays;
        historyRows.push_back({
            std::to_string(r.id),
            r.employeeCode,
            r.employeeName,
            r.leaveTypeName,
            r.startDate,
            r.endDate,
            ossDays.str(),
            r.reason,
            r.status,
            r.approvedByName.empty() ? "-" : r.approvedByName
        });
    }
    m_historyTable->setData(historyRows);
}

void LeaveTab::onEmpChanged(Fl_Widget* /*w*/, void* data) {
    LeaveTab* self = static_cast<LeaveTab*>(data);
    self->refreshData();
}

void LeaveTab::onRefreshClicked(Fl_Widget* /*w*/, void* data) {
    LeaveTab* self = static_cast<LeaveTab*>(data);
    self->refreshData();
}

void LeaveTab::onApplyClicked(Fl_Widget* /*w*/, void* data) {
    LeaveTab* self = static_cast<LeaveTab*>(data);
    self->openApplyDialog();
}

void LeaveTab::onApproveClicked(Fl_Widget* /*w*/, void* data) {
    LeaveTab* self = static_cast<LeaveTab*>(data);
    int row = self->m_pendingTable->getSelectedRow();
    if (row < 0 || row >= (int)self->m_pendingRequests.size()) {
        showError("Please select a request row from the Pending table to approve.");
        return;
    }

    const auto& req = self->m_pendingRequests[row];
    if (!askConfirm("Approve leave request #" + std::to_string(req.id) + " for " + req.employeeName + " (" + req.leaveTypeName + ")?")) {
        return;
    }

    auto& auth = sec::AuthService::instance();
    int userId = auth.isLoggedIn() ? auth.currentSession()->userId : 1;

    std::string err;
    if (leave::LeaveService::approveLeave(req.id, userId, err)) {
        showSuccess("Leave request approved successfully!");
        self->refreshData();
    } else {
        showError("Failed to approve leave: " + err);
    }
}

void LeaveTab::onRejectClicked(Fl_Widget* /*w*/, void* data) {
    LeaveTab* self = static_cast<LeaveTab*>(data);
    int row = self->m_pendingTable->getSelectedRow();
    if (row < 0 || row >= (int)self->m_pendingRequests.size()) {
        showError("Please select a request row from the Pending table to reject.");
        return;
    }
    self->openRejectDialog(self->m_pendingRequests[row].id);
}

// =============================================================================
// Dialog: Apply for Leave
// =============================================================================
void LeaveTab::openApplyDialog() {
    Fl_Window win(500, 440, "Apply for Leave");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 500, 50, "Submit Leave Application");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 440, fieldH = 30;

    // Employee
    Fl_Box* lEmp = new Fl_Box(30, curY, fieldW, 16, "Employee *");
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
    chEmp.value(m_choiceEmp->value());

    // Leave Type
    curY += 56;
    Fl_Box* lType = new Fl_Box(30, curY, fieldW, 16, "Leave Type *");
    lType->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lType->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lType->labelsize(11);
    Fl_Choice chType(30, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chType);
    auto types = leave::LeaveService::getLeaveTypes();
    for (const auto& t : types) {
        chType.add((t.code + " - " + t.name).c_str());
    }
    chType.value(0);

    // Dates
    curY += 56;
    Fl_Box* lStart = new Fl_Box(30, curY, 210, 16, "Start Date (YYYY-MM-DD) *");
    lStart->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lStart->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lStart->labelsize(11);
    Fl_Input inStart(30, curY + 18, 210, fieldH);
    Theme::styleInput(&inStart);
    inStart.value(utils::getCurrentDate().c_str());

    Fl_Box* lEnd = new Fl_Box(260, curY, 210, 16, "End Date (YYYY-MM-DD) *");
    lEnd->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lEnd->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lEnd->labelsize(11);
    Fl_Input inEnd(260, curY + 18, 210, fieldH);
    Theme::styleInput(&inEnd);
    inEnd.value(utils::getCurrentDate().c_str());

    // Total Days & Reason
    curY += 56;
    Fl_Box* lDays = new Fl_Box(30, curY, 120, 16, "Total Days *");
    lDays->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDays->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDays->labelsize(11);
    Fl_Input inDays(30, curY + 18, 120, fieldH);
    Theme::styleInput(&inDays);
    inDays.value("1");

    Fl_Box* lReason = new Fl_Box(170, curY, 300, 16, "Reason for Leave *");
    lReason->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lReason->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lReason->labelsize(11);
    Fl_Input inReason(170, curY + 18, 300, fieldH);
    Theme::styleInput(&inReason);
    inReason.value("Personal errand");

    // Buttons
    curY += 65;
    Fl_Button btnSubmit(30, curY, 150, 36, "Submit Application");
    Theme::stylePrimaryButton(&btnSubmit);

    Fl_Button btnCancel(190, curY, 100, 36, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool submitted = false;
    btnSubmit.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &submitted);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (submitted) {
            int empSel = chEmp.value();
            int typeSel = chType.value();
            if (empSel < 0 || empSel >= (int)employees.size() || typeSel < 0 || typeSel >= (int)types.size()) {
                showError("Invalid employee or leave type selection.");
                submitted = false;
                continue;
            }

            int empId = employees[empSel].id;
            int typeId = types[typeSel].id;
            std::string start = inStart.value() ? inStart.value() : "";
            std::string end = inEnd.value() ? inEnd.value() : "";
            std::string daysStr = inDays.value() ? inDays.value() : "1";
            std::string reason = inReason.value() ? inReason.value() : "";
            double days = 1.0;
            try { days = std::stod(daysStr); } catch (...) { days = 1.0; }

            std::string err;
            if (leave::LeaveService::requestLeave(empId, typeId, start, end, days, reason, err)) {
                showSuccess("Leave application submitted successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Submission failed: " + err);
                submitted = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Reject Leave with Mandatory Reason
// =============================================================================
void LeaveTab::openRejectDialog(int requestId) {
    Fl_Window win(440, 230, "Reject Leave Request");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 440, 45, "Provide Rejection Reason");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_DANGER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(14);

    Fl_Box* lReason = new Fl_Box(30, 60, 380, 16, "Reason for Rejection *");
    lReason->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lReason->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lReason->labelsize(11);

    Fl_Input inReason(30, 80, 380, 32);
    Theme::styleInput(&inReason);
    inReason.value("Critical project deliverables pending.");

    Fl_Button btnConfirm(30, 135, 140, 34, "Confirm Rejection");
    Theme::styleDangerButton(&btnConfirm);

    Fl_Button btnCancel(180, 135, 100, 34, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool confirmed = false;
    btnConfirm.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &confirmed);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (confirmed) {
            std::string reason = inReason.value() ? inReason.value() : "";
            if (reason.empty()) {
                showError("Please enter a reason for rejecting the leave request.");
                confirmed = false;
                continue;
            }

            auto& auth = sec::AuthService::instance();
            int userId = auth.isLoggedIn() ? auth.currentSession()->userId : 1;

            std::string err;
            if (leave::LeaveService::rejectLeave(requestId, userId, reason, err)) {
                showSuccess("Leave request rejected and balance restored.");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed to reject: " + err);
                confirmed = false;
            }
        }
    }
}

} // namespace hrms::gui
