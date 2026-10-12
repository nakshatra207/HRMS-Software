#pragma once

#include "gui/gui_common.hpp"
#include "recruitment/recruitment.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>

namespace hrms::gui {

class RecruitmentTab : public Fl_Group {
public:
    RecruitmentTab(int x, int y, int w, int h);

    void refreshData();

private:
    // View Switcher: 0 = Jobs, 1 = Candidates, 2 = Applications
    int m_currentView{0};

    Fl_Button* m_btnViewJobs{nullptr};
    Fl_Button* m_btnViewCandidates{nullptr};
    Fl_Button* m_btnViewApps{nullptr};

    // Action buttons
    Fl_Button* m_btnAction1{nullptr};
    Fl_Button* m_btnAction2{nullptr};
    Fl_Button* m_btnAction3{nullptr};
    Fl_Button* m_btnConvert{nullptr};
    Fl_Button* m_btnRefresh{nullptr};

    DataTable* m_table{nullptr};

    std::vector<rec::JobOpening> m_jobs;
    std::vector<rec::Candidate> m_candidates;
    std::vector<rec::Application> m_applications;

    void switchView(int viewIdx);
    void openCreateJobDialog();
    void openAddCandidateDialog();
    void openApplyCandidateDialog();
    void openScheduleInterviewDialog();
    void openRecordFeedbackDialog();
    void openConvertWizardDialog();

    static void onViewJobsClicked(Fl_Widget* w, void* data);
    static void onViewCandidatesClicked(Fl_Widget* w, void* data);
    static void onViewAppsClicked(Fl_Widget* w, void* data);
    static void onAction1Clicked(Fl_Widget* w, void* data);
    static void onAction2Clicked(Fl_Widget* w, void* data);
    static void onAction3Clicked(Fl_Widget* w, void* data);
    static void onConvertClicked(Fl_Widget* w, void* data);
    static void onRefreshClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
