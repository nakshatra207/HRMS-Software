#pragma once

#include "gui/gui_common.hpp"
#include "leave/leave.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>

namespace hrms::gui {

class LeaveTab : public Fl_Group {
public:
    LeaveTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Choice* m_choiceEmp{nullptr};
    Fl_Button* m_btnLoadBalances{nullptr};

    StatCard* m_cardSL{nullptr};
    StatCard* m_cardCL{nullptr};
    StatCard* m_cardPL{nullptr};
    StatCard* m_cardUL{nullptr};

    Fl_Button* m_btnApply{nullptr};
    Fl_Button* m_btnApprove{nullptr};
    Fl_Button* m_btnReject{nullptr};
    Fl_Button* m_btnRefresh{nullptr};

    DataTable* m_pendingTable{nullptr};
    DataTable* m_historyTable{nullptr};

    std::vector<leave::LeaveRequest> m_pendingRequests;
    std::vector<leave::LeaveRequest> m_allRequests;

    void openApplyDialog();
    void openRejectDialog(int requestId);

    static void onApplyClicked(Fl_Widget* w, void* data);
    static void onApproveClicked(Fl_Widget* w, void* data);
    static void onRejectClicked(Fl_Widget* w, void* data);
    static void onRefreshClicked(Fl_Widget* w, void* data);
    static void onEmpChanged(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
