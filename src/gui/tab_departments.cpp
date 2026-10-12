#include "gui/tab_departments.hpp"
#include "employees/employee.hpp"
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>

namespace hrms::gui {

DepartmentsTab::DepartmentsTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Departments") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // Action buttons
    int btnW = 120, barH = 34;

    m_btnAdd = new Fl_Button(x + margin, curY, btnW + 10, barH, "➕ Add Dept");
    Theme::styleSuccessButton(m_btnAdd);
    m_btnAdd->callback(onAddClicked, this);

    m_btnEdit = new Fl_Button(x + margin + btnW + 18, curY, btnW, barH, "✏️ Edit Dept");
    Theme::styleSecondaryButton(m_btnEdit);
    m_btnEdit->callback(onEditClicked, this);

    m_btnDelete = new Fl_Button(x + margin + (btnW * 2) + 26, curY, btnW, barH, "🗑️ Delete");
    Theme::styleDangerButton(m_btnDelete);
    m_btnDelete->callback(onDeleteClicked, this);

    m_btnRefresh = new Fl_Button(x + w - margin - 100, curY, 100, barH, "🔄 Refresh");
    Theme::styleSecondaryButton(m_btnRefresh);
    m_btnRefresh->callback(onRefreshClicked, this);

    curY += barH + 14;

    // Table
    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);
    m_table->setHeaders(
        {"ID", "Dept Code", "Department Name", "Description", "Manager", "Total Staff"},
        {50, 100, 200, 280, 180, 100}
    );

    end();
}

void DepartmentsTab::refreshData() {
    m_departments = emp::DepartmentService::getAll();
    std::vector<std::vector<std::string>> rows;
    for (const auto& d : m_departments) {
        rows.push_back({
            std::to_string(d.id),
            d.code,
            d.name,
            d.description,
            d.managerName.empty() ? "-" : d.managerName,
            std::to_string(d.employeeCount) + " employees"
        });
    }
    m_table->setData(rows);
}

void DepartmentsTab::onRefreshClicked(Fl_Widget* /*w*/, void* data) {
    DepartmentsTab* self = static_cast<DepartmentsTab*>(data);
    self->refreshData();
}

void DepartmentsTab::onAddClicked(Fl_Widget* /*w*/, void* data) {
    DepartmentsTab* self = static_cast<DepartmentsTab*>(data);
    self->openAddEditDialog(nullptr);
}

void DepartmentsTab::onEditClicked(Fl_Widget* /*w*/, void* data) {
    DepartmentsTab* self = static_cast<DepartmentsTab*>(data);
    int row = self->m_table->getSelectedRow();
    if (row < 0 || row >= (int)self->m_departments.size()) {
        showError("Please select a department row to edit.");
        return;
    }
    self->openAddEditDialog(&self->m_departments[row]);
}

void DepartmentsTab::onDeleteClicked(Fl_Widget* /*w*/, void* data) {
    DepartmentsTab* self = static_cast<DepartmentsTab*>(data);
    int row = self->m_table->getSelectedRow();
    if (row < 0 || row >= (int)self->m_departments.size()) {
        showError("Please select a department row to delete.");
        return;
    }

    const auto& dept = self->m_departments[row];
    if (dept.employeeCount > 0) {
        showError("Cannot delete department '" + dept.name + "' because it has " +
                  std::to_string(dept.employeeCount) + " assigned employee(s). Please transfer employees first.");
        return;
    }

    if (!askConfirm("Are you sure you want to delete department '" + dept.name + "'?")) {
        return;
    }

    std::string err;
    if (emp::DepartmentService::deleteDepartment(dept.id, err)) {
        showSuccess("Department deleted successfully.");
        self->refreshData();
    } else {
        showError("Failed to delete: " + err);
    }
}

// =============================================================================
// Dialog: Add / Edit Department
// =============================================================================
void DepartmentsTab::openAddEditDialog(const emp::Department* existing) {
    bool isEdit = (existing != nullptr);
    std::string title = isEdit ? ("Edit Department: " + existing->name) : "Create Department";

    Fl_Window win(460, 320, title.c_str());
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 460, 50, title.c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 400, fieldH = 30;

    Fl_Box* lCode = new Fl_Box(30, curY, 150, 16, "Department Code *");
    lCode->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lCode->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lCode->labelsize(11);
    Fl_Input inCode(30, curY + 18, 150, fieldH);
    Theme::styleInput(&inCode);
    if (isEdit) {
        inCode.value(existing->code.c_str());
        inCode.deactivate();
    } else {
        inCode.value("ENG");
    }

    Fl_Box* lName = new Fl_Box(190, curY, 240, 16, "Department Name *");
    lName->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lName->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lName->labelsize(11);
    Fl_Input inName(190, curY + 18, 240, fieldH);
    Theme::styleInput(&inName);
    if (isEdit) inName.value(existing->name.c_str());

    curY += 56;
    Fl_Box* lDesc = new Fl_Box(30, curY, fieldW, 16, "Description");
    lDesc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDesc->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDesc->labelsize(11);
    Fl_Input inDesc(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inDesc);
    if (isEdit) inDesc.value(existing->description.c_str());

    curY += 56;
    Fl_Box* lMgr = new Fl_Box(30, curY, fieldW, 16, "Department Manager");
    lMgr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lMgr->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lMgr->labelsize(11);
    Fl_Choice chMgr(30, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chMgr);
    chMgr.add("None / Unassigned");
    emp::EmployeeFilter filter;
    filter.status = "Active";
    filter.limit = 100;
    auto employees = emp::EmployeeService::search(filter);
    int selMgrIdx = 0;
    for (size_t i = 0; i < employees.size(); ++i) {
        chMgr.add((employees[i].employeeCode + " - " + employees[i].fullName()).c_str());
        if (isEdit && employees[i].id == existing->managerId) {
            selMgrIdx = (int)i + 1;
        }
    }
    chMgr.value(selMgrIdx);

    curY += 65;
    Fl_Button btnSave(30, curY, 150, 36, isEdit ? "Update Dept" : "Create Dept");
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
            std::string code = inCode.value() ? inCode.value() : "";
            std::string name = inName.value() ? inName.value() : "";
            std::string desc = inDesc.value() ? inDesc.value() : "";
            int mgrIdx = chMgr.value();
            int mgrId = (mgrIdx > 0 && mgrIdx - 1 < (int)employees.size()) ? employees[mgrIdx - 1].id : 0;

            if (code.empty() || name.empty()) {
                showError("Please enter both department code and name.");
                saved = false;
                continue;
            }

            std::string err;
            bool ok = false;
            if (isEdit) {
                ok = emp::DepartmentService::update(existing->id, name, desc, mgrId, err);
            } else {
                ok = emp::DepartmentService::create(code, name, desc, mgrId, err);
            }

            if (ok) {
                showSuccess(isEdit ? "Department updated successfully!" : "Department created successfully!");
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

} // namespace hrms::gui
