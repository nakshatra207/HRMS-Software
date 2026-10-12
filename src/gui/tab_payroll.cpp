#include "gui/tab_payroll.hpp"
#include "utilities/common_utils.hpp"
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <fstream>
#include <sstream>

namespace hrms::gui {

PayrollTab::PayrollTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Payroll") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // Row 1: Period input & actions
    int filterH = 32;

    Fl_Box* lPeriod = new Fl_Box(x + margin, curY, 90, filterH, "Billing Cycle:");
    lPeriod->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lPeriod->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lPeriod->labelfont(FL_HELVETICA_BOLD);
    lPeriod->labelsize(12);

    m_inputMonth = new Fl_Input(x + margin + 95, curY, 100, filterH);
    Theme::styleInput(m_inputMonth);
    m_inputMonth->value(utils::getCurrentDate().substr(0, 7).c_str());

    m_btnLoad = new Fl_Button(x + margin + 205, curY, 70, filterH, "Load");
    Theme::styleSecondaryButton(m_btnLoad);
    m_btnLoad->callback(onLoadClicked, this);

    // Actions aligned right
    int btnW = 130;
    int actX = x + w - margin;

    actX -= 90;
    m_btnRefresh = new Fl_Button(actX, curY, 90, filterH, "🔄 Refresh");
    Theme::styleSecondaryButton(m_btnRefresh);
    m_btnRefresh->callback(onRefreshClicked, this);

    actX -= (btnW + 8);
    m_btnViewPayslip = new Fl_Button(actX, curY, btnW, filterH, "🧾 View Payslip");
    Theme::styleSecondaryButton(m_btnViewPayslip);
    m_btnViewPayslip->callback(onViewPayslipClicked, this);

    actX -= (btnW + 8);
    m_btnMarkPaid = new Fl_Button(actX, curY, btnW, filterH, "💵 Mark as Paid");
    Theme::styleSuccessButton(m_btnMarkPaid);
    m_btnMarkPaid->callback(onMarkPaidClicked, this);

    actX -= (btnW + 12);
    m_btnProcess = new Fl_Button(actX, curY, btnW + 10, filterH, "⚡ Run Payroll");
    Theme::stylePrimaryButton(m_btnProcess);
    m_btnProcess->callback(onProcessClicked, this);

    curY += filterH + 14;

    // Row 2: 4 Summary Cards
    int cardCount = 4;
    int cardSpacing = 12;
    int cardW = (w - (margin * 2) - ((cardCount - 1) * cardSpacing)) / cardCount;
    int cardH = 75;

    m_cardGross      = new StatCard(x + margin + 0 * (cardW + cardSpacing), curY, cardW, cardH, "Total Gross Earnings", "₹ 0", Theme::COLOR_CYAN, "All employee earnings");
    m_cardDeductions = new StatCard(x + margin + 1 * (cardW + cardSpacing), curY, cardW, cardH, "Total Deductions", "₹ 0", Theme::COLOR_WARNING, "EPF & Income Tax (TDS)");
    m_cardNet        = new StatCard(x + margin + 2 * (cardW + cardSpacing), curY, cardW, cardH, "Total Net Payout", "₹ 0", Theme::COLOR_SUCCESS, "Net payable amount");
    m_cardStatus     = new StatCard(x + margin + 3 * (cardW + cardSpacing), curY, cardW, cardH, "Cycle Status", "Draft", Theme::COLOR_PRIMARY, "Current batch state");

    curY += cardH + 16;

    // Row 3: Payroll Register Table
    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);
    m_table->setHeaders(
        {"ID", "Emp Code", "Employee Name", "Department", "Designation", "Basic Salary", "Allowances", "Gross Pay", "Deductions", "Tax (TDS)", "Net Salary", "Status"},
        {45, 85, 140, 130, 140, 100, 95, 100, 95, 90, 110, 80}
    );
    m_table->onDoubleClick([this](int row) {
        onTableDoubleClicked(row, this);
    });

    end();
}

void PayrollTab::refreshData() {
    std::string month = m_inputMonth->value() ? m_inputMonth->value() : utils::getCurrentDate().substr(0, 7);
    m_currentRecords = pay::PayrollService::getPeriodRecords(month);

    int64_t totalGross = 0;
    int64_t totalDeductions = 0;
    int64_t totalNet = 0;
    std::string status = "Draft";

    std::vector<std::vector<std::string>> rows;
    for (const auto& r : m_currentRecords) {
        totalGross += r.grossEarningsPaise();
        totalDeductions += r.totalDeductionsPaise();
        totalNet += r.netSalaryPaise;
        status = r.status;

        rows.push_back({
            std::to_string(r.id),
            r.employeeCode,
            r.employeeName,
            r.departmentName,
            r.designation,
            utils::formatPaise(r.basicSalaryPaise),
            utils::formatPaise(r.allowancesPaise + r.bonusesPaise + r.overtimePaise),
            utils::formatPaise(r.grossEarningsPaise()),
            utils::formatPaise(r.deductionsPaise),
            utils::formatPaise(r.taxPaise),
            utils::formatPaise(r.netSalaryPaise),
            r.status
        });
    }

    m_cardGross->setValue(utils::formatPaise(totalGross));
    m_cardDeductions->setValue(utils::formatPaise(totalDeductions));
    m_cardNet->setValue(utils::formatPaise(totalNet));
    m_cardStatus->setValue(m_currentRecords.empty() ? "No Batch" : status);
    m_cardStatus->setSubtext(std::to_string(m_currentRecords.size()) + " records processed");

    m_table->setData(rows);
}

void PayrollTab::onLoadClicked(Fl_Widget* /*w*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    self->refreshData();
}

void PayrollTab::onRefreshClicked(Fl_Widget* /*w*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    self->refreshData();
}

void PayrollTab::onProcessClicked(Fl_Widget* /*w*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    self->openProcessDialog();
}

void PayrollTab::onMarkPaidClicked(Fl_Widget* /*w*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    std::string month = self->m_inputMonth->value() ? self->m_inputMonth->value() : "";
    if (month.empty() || self->m_currentRecords.empty()) {
        showError("Please load a processed payroll period first.");
        return;
    }

    if (!askConfirm("Mark all payroll records for period '" + month + "' as PAID?")) {
        return;
    }

    std::string err;
    if (pay::PayrollService::markPeriodAsPaid(month, err)) {
        showSuccess("Payroll period marked as Paid successfully!");
        self->refreshData();
    } else {
        showError("Failed to mark as paid: " + err);
    }
}

void PayrollTab::onViewPayslipClicked(Fl_Widget* /*w*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    int row = self->m_table->getSelectedRow();
    if (row < 0 || row >= (int)self->m_currentRecords.size()) {
        showError("Please select an employee row from the table to view their payslip.");
        return;
    }
    self->openPayslipDialog(self->m_currentRecords[row]);
}

void PayrollTab::onTableDoubleClicked(int /*rowIndex*/, void* data) {
    PayrollTab* self = static_cast<PayrollTab*>(data);
    int row = self->m_table->getSelectedRow();
    if (row >= 0 && row < (int)self->m_currentRecords.size()) {
        self->openPayslipDialog(self->m_currentRecords[row]);
    }
}

// =============================================================================
// Dialog: Run Payroll Batch
// =============================================================================
void PayrollTab::openProcessDialog() {
    Fl_Window win(440, 260, "Generate Monthly Payroll");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 440, 50, "Run Batch Payroll Generator");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 380, fieldH = 30;

    Fl_Box* lMonth = new Fl_Box(30, curY, fieldW, 16, "Billing Month (YYYY-MM) *");
    lMonth->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lMonth->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lMonth->labelsize(11);
    Fl_Input inMonth(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inMonth);
    inMonth.value(m_inputMonth->value() ? m_inputMonth->value() : utils::getCurrentDate().substr(0, 7).c_str());

    curY += 56;
    Fl_Box* lBonus = new Fl_Box(30, curY, fieldW, 16, "Performance Bonus Percentage (0 - 100%)");
    lBonus->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lBonus->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lBonus->labelsize(11);
    Fl_Input inBonus(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inBonus);
    inBonus.value("0");

    curY += 65;
    Fl_Button btnRun(30, curY, 150, 36, "⚡ Calculate Batch");
    Theme::stylePrimaryButton(&btnRun);

    Fl_Button btnCancel(190, curY, 100, 36, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool submitted = false;
    btnRun.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &submitted);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (submitted) {
            std::string month = inMonth.value() ? inMonth.value() : "";
            int bonus = 0;
            try { bonus = std::stoi(inBonus.value() ? inBonus.value() : "0"); } catch (...) { bonus = 0; }

            if (month.length() != 7 || month[4] != '-') {
                showError("Please enter a valid month in YYYY-MM format (e.g. 2026-10).");
                submitted = false;
                continue;
            }

            std::string err;
            if (pay::PayrollService::processMonthlyPayroll(month, bonus, err)) {
                showSuccess("Payroll calculated successfully for " + month + "!");
                m_inputMonth->value(month.c_str());
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed to calculate payroll: " + err);
                submitted = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Printable Payslip Viewer
// =============================================================================
void PayrollTab::openPayslipDialog(const pay::PayrollRecord& record) {
    Fl_Window win(620, 520, ("Payslip: " + record.employeeName).c_str());
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 620, 55, ("Salary Slip: " + record.employeeName + " (" + record.payrollPeriod + ")").c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    std::string receipt = pay::PayrollService::generateFormattedPayslip(record);

    Fl_Text_Buffer* buf = new Fl_Text_Buffer();
    buf->text(receipt.c_str());

    Fl_Text_Display* disp = new Fl_Text_Display(20, 70, 580, 380);
    disp->box(FL_FLAT_BOX);
    disp->color(Theme::COLOR_CARD_BG);
    disp->textcolor(Theme::COLOR_TEXT_PRIMARY);
    disp->textfont(FL_COURIER);
    disp->textsize(11);
    disp->buffer(buf);

    Fl_Button* btnSave = new Fl_Button(180, 465, 140, 36, "💾 Save to File");
    Theme::stylePrimaryButton(btnSave);

    Fl_Button* btnClose = new Fl_Button(330, 465, 110, 36, "Close");
    Theme::styleSecondaryButton(btnClose);
    btnClose->callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    btnSave->callback([](Fl_Widget*, void* data) {
        const std::string* rText = static_cast<const std::string*>(data);
        std::string fname = "reports/payslip.txt";
        std::ofstream out(fname);
        if (out.is_open()) {
            out << *rText;
            out.close();
            showSuccess("Payslip saved to " + fname);
        } else {
            showError("Could not write to " + fname);
        }
    }, &receipt);

    win.end();
    win.show();
    while (win.shown()) {
        Fl::wait();
    }
}

} // namespace hrms::gui
