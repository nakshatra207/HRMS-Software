#pragma once

#include "gui/gui_common.hpp"
#include "employees/department.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class DepartmentsTab : public Fl_Group {
public:
    DepartmentsTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Button* m_btnAdd{nullptr};
    Fl_Button* m_btnEdit{nullptr};
    Fl_Button* m_btnDelete{nullptr};
    Fl_Button* m_btnRefresh{nullptr};

    DataTable* m_table{nullptr};

    std::vector<emp::Department> m_departments;

    void openAddEditDialog(const emp::Department* existing);

    static void onAddClicked(Fl_Widget* w, void* data);
    static void onEditClicked(Fl_Widget* w, void* data);
    static void onDeleteClicked(Fl_Widget* w, void* data);
    static void onRefreshClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
