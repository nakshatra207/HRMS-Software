#pragma once

#include "gui/gui_common.hpp"
#include "employees/employee.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class EmployeesTab : public Fl_Group {
public:
    EmployeesTab(int x, int y, int w, int h);

    void refreshData();

private:
    // Filters
    Fl_Input* m_inputSearch{nullptr};
    Fl_Choice* m_choiceDept{nullptr};
    Fl_Choice* m_choiceStatus{nullptr};
    Fl_Button* m_btnSearch{nullptr};
    Fl_Button* m_btnReset{nullptr};

    // Actions
    Fl_Button* m_btnAdd{nullptr};
    Fl_Button* m_btnEdit{nullptr};
    Fl_Button* m_btnView{nullptr};
    Fl_Button* m_btnHistory{nullptr};
    Fl_Button* m_btnToggleActive{nullptr};

    // Main Table
    DataTable* m_table{nullptr};

    std::vector<emp::Employee> m_currentEmployees;

    int getSelectedEmployeeId() const;
    const emp::Employee* getSelectedEmployee() const;

    void openAddEditDialog(const emp::Employee* existingEmp);
    void openProfileDialog(const emp::Employee& emp);
    void openHistoryDialog(int employeeId, const std::string& empName);

    static void onSearchClicked(Fl_Widget* w, void* data);
    static void onResetClicked(Fl_Widget* w, void* data);
    static void onAddClicked(Fl_Widget* w, void* data);
    static void onEditClicked(Fl_Widget* w, void* data);
    static void onViewClicked(Fl_Widget* w, void* data);
    static void onHistoryClicked(Fl_Widget* w, void* data);
    static void onToggleActiveClicked(Fl_Widget* w, void* data);
    static void onTableDoubleClicked(int rowIndex, void* data);
};

} // namespace hrms::gui
