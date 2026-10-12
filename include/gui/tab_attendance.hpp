#pragma once

#include "gui/gui_common.hpp"
#include "attendance/attendance.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

namespace hrms::gui {

class AttendanceTab : public Fl_Group {
public:
    AttendanceTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Input* m_inputDate{nullptr};
    Fl_Choice* m_choiceDept{nullptr};
    Fl_Button* m_btnLoad{nullptr};
    Fl_Button* m_btnToday{nullptr};

    Fl_Button* m_btnCheckIn{nullptr};
    Fl_Button* m_btnCheckOut{nullptr};
    Fl_Box* m_statusText{nullptr};

    Fl_Button* m_btnManual{nullptr};
    Fl_Button* m_btnMonthlyStats{nullptr};

    DataTable* m_table{nullptr};

    void openManualDialog();
    void openMonthlyStatsDialog();

    static void onLoadClicked(Fl_Widget* w, void* data);
    static void onTodayClicked(Fl_Widget* w, void* data);
    static void onCheckInClicked(Fl_Widget* w, void* data);
    static void onCheckOutClicked(Fl_Widget* w, void* data);
    static void onManualClicked(Fl_Widget* w, void* data);
    static void onStatsClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
