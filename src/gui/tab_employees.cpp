#include "gui/tab_employees.hpp"
#include "employees/department.hpp"
#include "utilities/common_utils.hpp"
#include <FL/Fl_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

namespace hrms::gui {

EmployeesTab::EmployeesTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Employees") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // 1. Filter Bar
    int filterH = 32;
    int searchW = 220;

    m_inputSearch = new Fl_Input(x + margin, curY, searchW, filterH);
    Theme::styleInput(m_inputSearch);

    m_choiceDept = new Fl_Choice(x + margin + searchW + 10, curY, 160, filterH);
    Theme::styleChoice(m_choiceDept);
    m_choiceDept->add("All Departments");
    m_choiceDept->value(0);

    m_choiceStatus = new Fl_Choice(x + margin + searchW + 180, curY, 120, filterH);
    Theme::styleChoice(m_choiceStatus);
    m_choiceStatus->add("All Statuses|Active|Inactive");
    m_choiceStatus->value(0);

    m_btnSearch = new Fl_Button(x + margin + searchW + 310, curY, 90, filterH, "🔍 Filter");
    Theme::stylePrimaryButton(m_btnSearch);
    m_btnSearch->callback(onSearchClicked, this);

    m_btnReset = new Fl_Button(x + margin + searchW + 410, curY, 80, filterH, "Reset");
    Theme::styleSecondaryButton(m_btnReset);
    m_btnReset->callback(onResetClicked, this);

    // Action buttons aligned right
    int actionW = 100;
    int actX = x + w - margin;

    actX -= actionW;
    m_btnToggleActive = new Fl_Button(actX, curY, actionW, filterH, "Deactivate");
    Theme::styleDangerButton(m_btnToggleActive);
    m_btnToggleActive->callback(onToggleActiveClicked, this);

    actX -= (actionW + 8);
    m_btnHistory = new Fl_Button(actX, curY, actionW, filterH, "📜 History");
    Theme::styleSecondaryButton(m_btnHistory);
    m_btnHistory->callback(onHistoryClicked, this);

    actX -= (actionW + 8);
    m_btnView = new Fl_Button(actX, curY, actionW, filterH, "👁️ Profile");
    Theme::styleSecondaryButton(m_btnView);
    m_btnView->callback(onViewClicked, this);

    actX -= (actionW + 8);
    m_btnEdit = new Fl_Button(actX, curY, actionW, filterH, "✏️ Edit");
    Theme::styleSecondaryButton(m_btnEdit);
    m_btnEdit->callback(onEditClicked, this);

    actX -= (actionW + 8);
    m_btnAdd = new Fl_Button(actX, curY, actionW, filterH, "➕ Add New");
    Theme::styleSuccessButton(m_btnAdd);
    m_btnAdd->callback(onAddClicked, this);

    curY += filterH + 14;

    // 2. Main Employee Table
    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);
    m_table->setHeaders(
        {"ID", "Emp Code", "Full Name", "Department", "Designation", "Salary / Month", "Phone", "Email", "Status"},
        {50, 90, 160, 150, 170, 120, 110, 170, 90}
    );
    m_table->onDoubleClick([this](int row) {
        onTableDoubleClicked(row, this);
    });

    end();
}

void EmployeesTab::refreshData() {
    // Populate departments choice if needed
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

    // Build filter
    emp::EmployeeFilter filter;
    filter.limit = 500;
    if (m_inputSearch->value() && strlen(m_inputSearch->value()) > 0) {
        filter.searchQuery = m_inputSearch->value();
    }

    int deptIdx = m_choiceDept->value();
    if (deptIdx > 0 && deptIdx - 1 < (int)depts.size()) {
        filter.departmentId = depts[deptIdx - 1].id;
    }

    int statusIdx = m_choiceStatus->value();
    if (statusIdx == 1) filter.status = "Active";
    else if (statusIdx == 2) filter.status = "Inactive";

    m_currentEmployees = emp::EmployeeService::search(filter);

    std::vector<std::vector<std::string>> rows;
    for (const auto& e : m_currentEmployees) {
        rows.push_back({
            std::to_string(e.id),
            e.employeeCode,
            e.fullName(),
            e.departmentName,
            e.designation,
            utils::formatPaise(e.salaryPaise),
            e.phone,
            e.email,
            e.employmentStatus
        });
    }
    m_table->setData(rows);
}

int EmployeesTab::getSelectedEmployeeId() const {
    int row = m_table->getSelectedRow();
    if (row >= 0 && row < (int)m_currentEmployees.size()) {
        return m_currentEmployees[row].id;
    }
    return -1;
}

const emp::Employee* EmployeesTab::getSelectedEmployee() const {
    int row = m_table->getSelectedRow();
    if (row >= 0 && row < (int)m_currentEmployees.size()) {
        return &m_currentEmployees[row];
    }
    return nullptr;
}

void EmployeesTab::onSearchClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    self->refreshData();
}

void EmployeesTab::onResetClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    self->m_inputSearch->value("");
    self->m_choiceDept->value(0);
    self->m_choiceStatus->value(0);
    self->refreshData();
}

void EmployeesTab::onAddClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    self->openAddEditDialog(nullptr);
}

void EmployeesTab::onEditClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    const auto* emp = self->getSelectedEmployee();
    if (!emp) {
        showError("Please select an employee row to edit.");
        return;
    }
    self->openAddEditDialog(emp);
}

void EmployeesTab::onViewClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    const auto* emp = self->getSelectedEmployee();
    if (!emp) {
        showError("Please select an employee row to view profile.");
        return;
    }
    self->openProfileDialog(*emp);
}

void EmployeesTab::onHistoryClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    const auto* emp = self->getSelectedEmployee();
    if (!emp) {
        showError("Please select an employee to view career history.");
        return;
    }
    self->openHistoryDialog(emp->id, emp->fullName());
}

void EmployeesTab::onToggleActiveClicked(Fl_Widget* /*w*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    const auto* emp = self->getSelectedEmployee();
    if (!emp) {
        showError("Please select an employee row first.");
        return;
    }

    std::string actionName = (emp->employmentStatus == "Active") ? "deactivate" : "activate";
    if (!askConfirm("Are you sure you want to " + actionName + " " + emp->fullName() + "?")) {
        return;
    }

    std::string err;
    if (emp->employmentStatus == "Active") {
        if (emp::EmployeeService::deactivate(emp->id, err)) {
            showSuccess("Employee deactivated successfully.");
            self->refreshData();
        } else {
            showError("Failed to deactivate: " + err);
        }
    } else {
        emp::Employee updated = *emp;
        updated.employmentStatus = "Active";
        if (emp::EmployeeService::update(updated, err)) {
            showSuccess("Employee activated successfully.");
            self->refreshData();
        } else {
            showError("Failed to activate: " + err);
        }
    }
}

void EmployeesTab::onTableDoubleClicked(int /*rowIndex*/, void* data) {
    EmployeesTab* self = static_cast<EmployeesTab*>(data);
    const auto* emp = self->getSelectedEmployee();
    if (emp) {
        self->openProfileDialog(*emp);
    }
}

// =============================================================================
// Dialog: Add / Edit Employee
// =============================================================================
void EmployeesTab::openAddEditDialog(const emp::Employee* existing) {
    bool isEdit = (existing != nullptr);
    std::string title = isEdit ? ("Edit Employee: " + existing->fullName()) : "Add New Employee";

    Fl_Window win(620, 560, title.c_str());
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 620, 50, title.c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(16);

    int col1X = 30, col2X = 320;
    int fieldW = 270, fieldH = 30;
    int yStart = 65;

    // Row 1: Code & Joining Date
    Fl_Box* l1 = new Fl_Box(col1X, yStart, fieldW, 16, "Employee Code *");
    l1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l1->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l1->labelsize(11);
    Fl_Input inCode(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inCode);
    inCode.value(isEdit ? existing->employeeCode.c_str() : emp::EmployeeService::generateNextEmployeeCode().c_str());

    Fl_Box* l2 = new Fl_Box(col2X, yStart, fieldW, 16, "Joining Date (YYYY-MM-DD) *");
    l2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l2->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l2->labelsize(11);
    Fl_Input inJoin(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inJoin);
    inJoin.value(isEdit ? existing->joiningDate.c_str() : utils::getCurrentDate().c_str());

    // Row 2: First Name & Last Name
    yStart += 56;
    Fl_Box* l3 = new Fl_Box(col1X, yStart, fieldW, 16, "First Name *");
    l3->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l3->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l3->labelsize(11);
    Fl_Input inFirst(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inFirst);
    if (isEdit) inFirst.value(existing->firstName.c_str());

    Fl_Box* l4 = new Fl_Box(col2X, yStart, fieldW, 16, "Last Name *");
    l4->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l4->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l4->labelsize(11);
    Fl_Input inLast(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inLast);
    if (isEdit) inLast.value(existing->lastName.c_str());

    // Row 3: Email & Phone
    yStart += 56;
    Fl_Box* l5 = new Fl_Box(col1X, yStart, fieldW, 16, "Email Address *");
    l5->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l5->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l5->labelsize(11);
    Fl_Input inEmail(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inEmail);
    if (isEdit) inEmail.value(existing->email.c_str());

    Fl_Box* l6 = new Fl_Box(col2X, yStart, fieldW, 16, "Phone Number *");
    l6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l6->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l6->labelsize(11);
    Fl_Input inPhone(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inPhone);
    if (isEdit) inPhone.value(existing->phone.c_str());

    // Row 4: Department & Designation
    yStart += 56;
    Fl_Box* l7 = new Fl_Box(col1X, yStart, fieldW, 16, "Department *");
    l7->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l7->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l7->labelsize(11);
    Fl_Choice chDept(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleChoice(&chDept);
    auto depts = emp::DepartmentService::getAll();
    int selDeptIdx = 0;
    for (size_t i = 0; i < depts.size(); ++i) {
        chDept.add(depts[i].name.c_str());
        if (isEdit && depts[i].id == existing->departmentId) {
            selDeptIdx = (int)i;
        }
    }
    chDept.value(selDeptIdx);

    Fl_Box* l8 = new Fl_Box(col2X, yStart, fieldW, 16, "Designation *");
    l8->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l8->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l8->labelsize(11);
    Fl_Input inDesig(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inDesig);
    if (isEdit) inDesig.value(existing->designation.c_str());

    // Row 5: Salary & Status
    yStart += 56;
    Fl_Box* l9 = new Fl_Box(col1X, yStart, fieldW, 16, "Monthly Salary (₹) *");
    l9->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l9->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l9->labelsize(11);
    Fl_Input inSalary(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inSalary);
    if (isEdit) inSalary.value(std::to_string(existing->salaryPaise / 100).c_str());
    else inSalary.value("50000");

    Fl_Box* l10 = new Fl_Box(col2X, yStart, fieldW, 16, "Status");
    l10->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l10->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l10->labelsize(11);
    Fl_Choice chStatus(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleChoice(&chStatus);
    chStatus.add("Active|Inactive|Terminated");
    chStatus.value(isEdit && existing->employmentStatus == "Inactive" ? 1 : 0);

    // Row 6: Bank Account & IFSC
    yStart += 56;
    Fl_Box* l11 = new Fl_Box(col1X, yStart, fieldW, 16, "Bank Account Number");
    l11->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l11->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l11->labelsize(11);
    Fl_Input inBankAcc(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inBankAcc);
    if (isEdit) inBankAcc.value(existing->bankAccountNo.c_str());

    Fl_Box* l12 = new Fl_Box(col2X, yStart, fieldW, 16, "Bank IFSC Code");
    l12->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l12->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l12->labelsize(11);
    Fl_Input inBankIfsc(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inBankIfsc);
    if (isEdit) inBankIfsc.value(existing->bankIfsc.c_str());

    // Row 7: Emergency Contact & PAN
    yStart += 56;
    Fl_Box* l13 = new Fl_Box(col1X, yStart, fieldW, 16, "Emergency Contact Name");
    l13->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l13->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l13->labelsize(11);
    Fl_Input inEmerg(col1X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inEmerg);
    if (isEdit) inEmerg.value(existing->emergencyContactName.c_str());

    Fl_Box* l14 = new Fl_Box(col2X, yStart, fieldW, 16, "PAN / Tax ID");
    l14->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l14->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l14->labelsize(11);
    Fl_Input inPan(col2X, yStart + 18, fieldW, fieldH);
    Theme::styleInput(&inPan);
    if (isEdit) inPan.value(existing->panTaxId.c_str());

    // Buttons
    yStart += 65;
    Fl_Button btnSave(col1X, yStart, 160, 36, isEdit ? "Update Details" : "Save Employee");
    Theme::stylePrimaryButton(&btnSave);

    Fl_Button btnCancel(col1X + 175, yStart, 110, 36, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool saved = false;
    btnSave.callback([](Fl_Widget* /*w*/, void* data) {
        *static_cast<bool*>(data) = true;
    }, &saved);

    btnCancel.callback([](Fl_Widget* w, void*) {
        w->window()->hide();
    });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (saved) {
            std::string code = inCode.value() ? inCode.value() : "";
            std::string first = inFirst.value() ? inFirst.value() : "";
            std::string last = inLast.value() ? inLast.value() : "";
            std::string email = inEmail.value() ? inEmail.value() : "";
            std::string phone = inPhone.value() ? inPhone.value() : "";
            std::string desig = inDesig.value() ? inDesig.value() : "";
            std::string join = inJoin.value() ? inJoin.value() : "";
            std::string salStr = inSalary.value() ? inSalary.value() : "0";

            if (code.empty() || first.empty() || last.empty() || email.empty() || phone.empty() || desig.empty()) {
                showError("Please fill all required fields marked with an asterisk (*).");
                saved = false;
                continue;
            }

            int deptId = 1;
            int sel = chDept.value();
            if (sel >= 0 && sel < (int)depts.size()) {
                deptId = depts[sel].id;
            }

            emp::Employee empData;
            if (isEdit) empData = *existing;
            empData.employeeCode = code;
            empData.firstName = first;
            empData.lastName = last;
            empData.email = email;
            empData.phone = phone;
            empData.departmentId = deptId;
            empData.designation = desig;
            empData.joiningDate = join;
            empData.salaryPaise = utils::parsePaise(salStr);
            empData.employmentStatus = (chStatus.value() == 1) ? "Inactive" : "Active";
            empData.bankAccountNo = inBankAcc.value() ? inBankAcc.value() : "";
            empData.bankIfsc = inBankIfsc.value() ? inBankIfsc.value() : "";
            empData.emergencyContactName = inEmerg.value() ? inEmerg.value() : "";
            empData.panTaxId = inPan.value() ? inPan.value() : "";

            std::string err;
            bool ok = false;
            if (isEdit) {
                ok = emp::EmployeeService::update(empData, err);
            } else {
                ok = emp::EmployeeService::create(empData, err);
            }

            if (ok) {
                showSuccess(isEdit ? "Employee updated successfully!" : "Employee added successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Operation failed: " + err);
                saved = false;
            }
        }
    }
}

// =============================================================================
// Dialog: View Full Profile
// =============================================================================
void EmployeesTab::openProfileDialog(const emp::Employee& emp) {
    Fl_Window win(560, 480, ("Employee Profile: " + emp.fullName()).c_str());
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 560, 60, emp.fullName().c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(18);

    Fl_Text_Buffer* buf = new Fl_Text_Buffer();
    std::string text =
        "EMPLOYEE DOSSIER\n"
        "============================================================\n"
        "Employee Code     : " + emp.employeeCode + "\n"
        "Full Name         : " + emp.fullName() + "\n"
        "Department        : " + emp.departmentName + "\n"
        "Designation       : " + emp.designation + "\n"
        "Employment Status : " + emp.employmentStatus + "\n"
        "Monthly Salary    : " + utils::formatPaise(emp.salaryPaise) + "\n"
        "Joining Date      : " + emp.joiningDate + "\n"
        "\nCONTACT DETAILS\n"
        "------------------------------------------------------------\n"
        "Email             : " + emp.email + "\n"
        "Phone             : " + emp.phone + "\n"
        "Address           : " + (emp.address.empty() ? "N/A" : emp.address) + "\n"
        "Emergency Contact : " + (emp.emergencyContactName.empty() ? "N/A" : emp.emergencyContactName) +
        " (" + emp.emergencyContactPhone + ")\n"
        "\nBANK & STATUTORY INFORMATION\n"
        "------------------------------------------------------------\n"
        "Bank Name         : " + (emp.bankName.empty() ? "N/A" : emp.bankName) + "\n"
        "Account Number    : " + (emp.bankAccountNo.empty() ? "N/A" : emp.bankAccountNo) + "\n"
        "IFSC Code         : " + (emp.bankIfsc.empty() ? "N/A" : emp.bankIfsc) + "\n"
        "PAN / Tax ID      : " + (emp.panTaxId.empty() ? "N/A" : emp.panTaxId) + "\n"
        "============================================================\n";
    buf->text(text.c_str());

    Fl_Text_Display* disp = new Fl_Text_Display(20, 75, 520, 340);
    disp->box(FL_FLAT_BOX);
    disp->color(Theme::COLOR_CARD_BG);
    disp->textcolor(Theme::COLOR_TEXT_PRIMARY);
    disp->textfont(FL_COURIER);
    disp->textsize(12);
    disp->buffer(buf);

    Fl_Button* btnClose = new Fl_Button(220, 430, 120, 34, "Close");
    Theme::stylePrimaryButton(btnClose);
    btnClose->callback([](Fl_Widget* w, void*) {
        w->window()->hide();
    });

    win.end();
    win.show();
    while (win.shown()) {
        Fl::wait();
    }
}

// =============================================================================
// Dialog: Employee Career History
// =============================================================================
void EmployeesTab::openHistoryDialog(int employeeId, const std::string& empName) {
    Fl_Window win(680, 420, ("Career History: " + empName).c_str());
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 680, 48, ("Career Audit Trail: " + empName).c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    DataTable* table = new DataTable(20, 60, 640, 300);
    table->setHeaders({"Date / Time", "Event", "Previous Value", "New Value", "Updated By"}, {130, 120, 140, 140, 100});

    auto history = emp::EmployeeService::getHistory(employeeId);
    std::vector<std::vector<std::string>> rows;
    for (const auto& h : history) {
        rows.push_back({
            h.changedAt,
            h.changeType,
            h.oldValue,
            h.newValue,
            h.changedByName
        });
    }
    table->setData(rows);

    Fl_Button* btnClose = new Fl_Button(280, 372, 120, 34, "Close");
    Theme::stylePrimaryButton(btnClose);
    btnClose->callback([](Fl_Widget* w, void*) {
        w->window()->hide();
    });

    win.end();
    win.show();
    while (win.shown()) {
        Fl::wait();
    }
}

} // namespace hrms::gui
