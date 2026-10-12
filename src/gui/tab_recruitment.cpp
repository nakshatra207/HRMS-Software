#include "gui/tab_recruitment.hpp"
#include "employees/department.hpp"
#include "employees/employee.hpp"
#include "utilities/common_utils.hpp"
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>

namespace hrms::gui {

RecruitmentTab::RecruitmentTab(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h, "Recruitment") {
    box(FL_FLAT_BOX);
    color(Theme::COLOR_BG);

    int margin = 16;
    int curY = y + 16;

    // View switcher buttons (Left)
    int viewBtnW = 130, barH = 34;
    m_btnViewJobs = new Fl_Button(x + margin, curY, viewBtnW, barH, "💼 Job Openings");
    m_btnViewJobs->callback(onViewJobsClicked, this);

    m_btnViewCandidates = new Fl_Button(x + margin + viewBtnW + 8, curY, viewBtnW + 10, barH, "👤 Candidate Pool");
    m_btnViewCandidates->callback(onViewCandidatesClicked, this);

    m_btnViewApps = new Fl_Button(x + margin + (viewBtnW * 2) + 26, curY, viewBtnW + 20, barH, "📋 Applications Pipeline");
    m_btnViewApps->callback(onViewAppsClicked, this);

    // Actions aligned right
    int actX = x + w - margin;

    actX -= 80;
    m_btnRefresh = new Fl_Button(actX, curY, 80, barH, "🔄 Refresh");
    Theme::styleSecondaryButton(m_btnRefresh);
    m_btnRefresh->callback(onRefreshClicked, this);

    actX -= 150;
    m_btnConvert = new Fl_Button(actX, curY, 142, barH, "🎉 Convert to Staff");
    Theme::styleSuccessButton(m_btnConvert);
    m_btnConvert->callback(onConvertClicked, this);

    actX -= 140;
    m_btnAction3 = new Fl_Button(actX, curY, 132, barH, "⭐ Rate Interview");
    Theme::styleSecondaryButton(m_btnAction3);
    m_btnAction3->callback(onAction3Clicked, this);

    actX -= 140;
    m_btnAction2 = new Fl_Button(actX, curY, 132, barH, "📅 Schedule Intvw");
    Theme::styleSecondaryButton(m_btnAction2);
    m_btnAction2->callback(onAction2Clicked, this);

    actX -= 130;
    m_btnAction1 = new Fl_Button(actX, curY, 122, barH, "➕ Post Job");
    Theme::stylePrimaryButton(m_btnAction1);
    m_btnAction1->callback(onAction1Clicked, this);

    curY += barH + 14;

    // Table
    int tableH = h - (curY - y) - margin;
    m_table = new DataTable(x + margin, curY, w - (margin * 2), tableH);

    end();
    switchView(0);
}

void RecruitmentTab::switchView(int viewIdx) {
    m_currentView = viewIdx;

    if (m_currentView == 0) { // Jobs
        Theme::stylePrimaryButton(m_btnViewJobs);
        Theme::styleSecondaryButton(m_btnViewCandidates);
        Theme::styleSecondaryButton(m_btnViewApps);

        m_btnAction1->copy_label("➕ Post Job");
        m_btnAction1->show();
        m_btnAction2->hide();
        m_btnAction3->hide();
        m_btnConvert->hide();

        m_table->setHeaders(
            {"ID", "Job Title", "Department", "Vacancies", "Status", "Applications", "Posted Date"},
            {50, 240, 180, 90, 100, 100, 120}
        );
    } else if (m_currentView == 1) { // Candidates
        Theme::styleSecondaryButton(m_btnViewJobs);
        Theme::stylePrimaryButton(m_btnViewCandidates);
        Theme::styleSecondaryButton(m_btnViewApps);

        m_btnAction1->copy_label("➕ Add Candidate");
        m_btnAction1->show();
        m_btnAction2->copy_label("Apply to Job");
        m_btnAction2->show();
        m_btnAction3->hide();
        m_btnConvert->hide();

        m_table->setHeaders(
            {"ID", "Candidate Name", "Email", "Phone", "Experience", "Key Skills", "Status"},
            {50, 170, 180, 120, 100, 220, 90}
        );
    } else { // Applications
        Theme::styleSecondaryButton(m_btnViewJobs);
        Theme::styleSecondaryButton(m_btnViewCandidates);
        Theme::stylePrimaryButton(m_btnViewApps);

        m_btnAction1->copy_label("Advance Stage");
        m_btnAction1->show();
        m_btnAction2->copy_label("📅 Schedule");
        m_btnAction2->show();
        m_btnAction3->copy_label("⭐ Rate Feedback");
        m_btnAction3->show();
        m_btnConvert->show();

        m_table->setHeaders(
            {"App ID", "Candidate Name", "Job Opening", "Department", "Stage", "Applied Date", "Candidate Email", "Phone"},
            {60, 160, 180, 130, 110, 100, 160, 110}
        );
    }

    refreshData();
}

void RecruitmentTab::refreshData() {
    if (m_currentView == 0) {
        m_jobs = rec::RecruitmentService::getJobOpenings();
        std::vector<std::vector<std::string>> rows;
        for (const auto& j : m_jobs) {
            rows.push_back({
                std::to_string(j.id),
                j.title,
                j.departmentName,
                std::to_string(j.vacancies) + " positions",
                j.status,
                std::to_string(j.applicationCount),
                j.postedDate
            });
        }
        m_table->setData(rows);
    } else if (m_currentView == 1) {
        m_candidates = rec::RecruitmentService::getCandidates();
        std::vector<std::vector<std::string>> rows;
        for (const auto& c : m_candidates) {
            rows.push_back({
                std::to_string(c.id),
                c.fullName(),
                c.email,
                c.phone,
                std::to_string((int)c.experienceYears) + " years",
                c.skills,
                c.status
            });
        }
        m_table->setData(rows);
    } else {
        m_applications = rec::RecruitmentService::getApplications();
        std::vector<std::vector<std::string>> rows;
        for (const auto& a : m_applications) {
            rows.push_back({
                std::to_string(a.id),
                a.candidateName,
                a.jobTitle,
                a.departmentName,
                a.status,
                a.applicationDate,
                a.candidateEmail,
                a.candidatePhone
            });
        }
        m_table->setData(rows);
    }
}

void RecruitmentTab::onViewJobsClicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->switchView(0);
}

void RecruitmentTab::onViewCandidatesClicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->switchView(1);
}

void RecruitmentTab::onViewAppsClicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->switchView(2);
}

void RecruitmentTab::onRefreshClicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->refreshData();
}

void RecruitmentTab::onAction1Clicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    if (self->m_currentView == 0) {
        self->openCreateJobDialog();
    } else if (self->m_currentView == 1) {
        self->openAddCandidateDialog();
    } else {
        int row = self->m_table->getSelectedRow();
        if (row < 0 || row >= (int)self->m_applications.size()) {
            showError("Please select an application row to advance its stage.");
            return;
        }
        const auto& app = self->m_applications[row];
        std::string nextStage = "Interviewing";
        if (app.status == "Applied") nextStage = "Screening";
        else if (app.status == "Screening") nextStage = "Interviewing";
        else if (app.status == "Interviewing") nextStage = "Offered";
        else if (app.status == "Offered") nextStage = "Hired";

        std::string err;
        if (rec::RecruitmentService::updateApplicationStatus(app.id, nextStage, err)) {
            showSuccess("Application #" + std::to_string(app.id) + " moved to stage: " + nextStage);
            self->refreshData();
        } else {
            showError("Failed to update status: " + err);
        }
    }
}

void RecruitmentTab::onAction2Clicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    if (self->m_currentView == 1) {
        self->openApplyCandidateDialog();
    } else if (self->m_currentView == 2) {
        self->openScheduleInterviewDialog();
    }
}

void RecruitmentTab::onAction3Clicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->openRecordFeedbackDialog();
}

void RecruitmentTab::onConvertClicked(Fl_Widget* /*w*/, void* data) {
    RecruitmentTab* self = static_cast<RecruitmentTab*>(data);
    self->openConvertWizardDialog();
}

// =============================================================================
// Dialog: Post Job Opening
// =============================================================================
void RecruitmentTab::openCreateJobDialog() {
    Fl_Window win(500, 380, "Post New Job Opening");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 500, 50, "Create Job Opening");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 440, fieldH = 30;

    Fl_Box* lTitle = new Fl_Box(30, curY, fieldW, 16, "Job Title *");
    lTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lTitle->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lTitle->labelsize(11);
    Fl_Input inTitle(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inTitle);
    inTitle.value("Senior Backend C++ Engineer");

    curY += 56;
    Fl_Box* lDept = new Fl_Box(30, curY, 210, 16, "Department *");
    lDept->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDept->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDept->labelsize(11);
    Fl_Choice chDept(30, curY + 18, 210, fieldH);
    Theme::styleChoice(&chDept);
    auto depts = emp::DepartmentService::getAll();
    for (const auto& d : depts) chDept.add(d.name.c_str());
    chDept.value(0);

    Fl_Box* lVac = new Fl_Box(260, curY, 210, 16, "Vacancies (Count) *");
    lVac->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lVac->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lVac->labelsize(11);
    Fl_Input inVac(260, curY + 18, 210, fieldH);
    Theme::styleInput(&inVac);
    inVac.value("2");

    curY += 56;
    Fl_Box* lReq = new Fl_Box(30, curY, fieldW, 16, "Requirements Summary");
    lReq->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lReq->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lReq->labelsize(11);
    Fl_Input inReq(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inReq);
    inReq.value("C++20, Linux Systems, Multi-threading, SQLite");

    curY += 65;
    Fl_Button btnSave(30, curY, 150, 36, "Publish Opening");
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
            std::string title = inTitle.value() ? inTitle.value() : "";
            int deptIdx = chDept.value();
            int deptId = (deptIdx >= 0 && deptIdx < (int)depts.size()) ? depts[deptIdx].id : 1;
            int vac = 1;
            try { vac = std::stoi(inVac.value() ? inVac.value() : "1"); } catch (...) { vac = 1; }

            if (title.empty()) {
                showError("Please enter a job title.");
                saved = false;
                continue;
            }

            rec::JobOpening job;
            job.title = title;
            job.departmentId = deptId;
            job.vacancies = vac;
            job.requirements = inReq.value() ? inReq.value() : "";
            job.status = "Open";
            job.postedDate = utils::getCurrentDate();

            std::string err;
            if (rec::RecruitmentService::createJobOpening(job, err)) {
                showSuccess("Job opening published successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed to create opening: " + err);
                saved = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Add Candidate
// =============================================================================
void RecruitmentTab::openAddCandidateDialog() {
    Fl_Window win(500, 420, "Register Candidate");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 500, 50, "Candidate Talent Profile");
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 210, fieldH = 30;

    Fl_Box* l1 = new Fl_Box(30, curY, fieldW, 16, "First Name *");
    l1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l1->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l1->labelsize(11);
    Fl_Input inFirst(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inFirst);

    Fl_Box* l2 = new Fl_Box(260, curY, fieldW, 16, "Last Name *");
    l2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l2->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l2->labelsize(11);
    Fl_Input inLast(260, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inLast);

    curY += 56;
    Fl_Box* l3 = new Fl_Box(30, curY, fieldW, 16, "Email Address *");
    l3->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l3->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l3->labelsize(11);
    Fl_Input inEmail(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inEmail);

    Fl_Box* l4 = new Fl_Box(260, curY, fieldW, 16, "Phone Number *");
    l4->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l4->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l4->labelsize(11);
    Fl_Input inPhone(260, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inPhone);

    curY += 56;
    Fl_Box* l5 = new Fl_Box(30, curY, fieldW, 16, "Years of Experience");
    l5->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l5->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l5->labelsize(11);
    Fl_Input inExp(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inExp);
    inExp.value("3");

    Fl_Box* l6 = new Fl_Box(260, curY, fieldW, 16, "Key Skills");
    l6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    l6->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    l6->labelsize(11);
    Fl_Input inSkills(260, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inSkills);
    inSkills.value("C++, Python, SQL");

    curY += 65;
    Fl_Button btnSave(30, curY, 150, 36, "Register Candidate");
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
            std::string first = inFirst.value() ? inFirst.value() : "";
            std::string last = inLast.value() ? inLast.value() : "";
            std::string email = inEmail.value() ? inEmail.value() : "";
            std::string phone = inPhone.value() ? inPhone.value() : "";

            if (first.empty() || last.empty() || email.empty()) {
                showError("Please enter first name, last name, and email.");
                saved = false;
                continue;
            }

            rec::Candidate cand;
            cand.firstName = first;
            cand.lastName = last;
            cand.email = email;
            cand.phone = phone;
            try { cand.experienceYears = std::stod(inExp.value() ? inExp.value() : "0"); } catch (...) {}
            cand.skills = inSkills.value() ? inSkills.value() : "";
            cand.status = "Active";

            std::string err;
            if (rec::RecruitmentService::createCandidate(cand, err)) {
                showSuccess("Candidate registered successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed to register candidate: " + err);
                saved = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Apply Candidate to Job
// =============================================================================
void RecruitmentTab::openApplyCandidateDialog() {
    int row = m_table->getSelectedRow();
    if (row < 0 || row >= (int)m_candidates.size()) {
        showError("Please select a candidate to link to a job opening.");
        return;
    }
    const auto& cand = m_candidates[row];

    Fl_Window win(460, 260, "Submit Job Application");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 460, 50, ("Apply: " + cand.fullName()).c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 400, fieldH = 30;
    Fl_Box* lJob = new Fl_Box(30, curY, fieldW, 16, "Select Job Opening *");
    lJob->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lJob->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lJob->labelsize(11);
    Fl_Choice chJob(30, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chJob);
    auto jobs = rec::RecruitmentService::getJobOpenings("Open");
    for (const auto& j : jobs) {
        chJob.add((j.title + " (" + j.departmentName + ")").c_str());
    }
    chJob.value(0);

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
            int sel = chJob.value();
            if (sel < 0 || sel >= (int)jobs.size()) {
                showError("Please select a valid open job position.");
                submitted = false;
                continue;
            }

            std::string err;
            if (rec::RecruitmentService::applyForJob(jobs[sel].id, cand.id, "Candidate Profile Submission", err)) {
                showSuccess("Application submitted successfully!");
                win.hide();
                switchView(2); // Switch to applications view
                return;
            } else {
                showError("Application failed: " + err);
                submitted = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Schedule Interview
// =============================================================================
void RecruitmentTab::openScheduleInterviewDialog() {
    int row = m_table->getSelectedRow();
    if (row < 0 || row >= (int)m_applications.size()) {
        showError("Please select an application to schedule an interview.");
        return;
    }
    const auto& app = m_applications[row];

    Fl_Window win(480, 340, "Schedule Interview");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 480, 50, ("Interview: " + app.candidateName).c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 420, fieldH = 30;

    Fl_Box* lRound = new Fl_Box(30, curY, 200, 16, "Interview Round *");
    lRound->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lRound->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lRound->labelsize(11);
    Fl_Choice chRound(30, curY + 18, 200, fieldH);
    Theme::styleChoice(&chRound);
    chRound.add("Technical|HR Screening|Cultural Fit|Managerial Final");
    chRound.value(0);

    Fl_Box* lDate = new Fl_Box(250, curY, 200, 16, "Date & Time *");
    lDate->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDate->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDate->labelsize(11);
    Fl_Input inDate(250, curY + 18, 200, fieldH);
    Theme::styleInput(&inDate);
    inDate.value((utils::getCurrentDate() + " 14:00:00").c_str());

    curY += 56;
    Fl_Box* lIntv = new Fl_Box(30, curY, fieldW, 16, "Select Interviewer *");
    lIntv->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lIntv->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lIntv->labelsize(11);
    Fl_Choice chIntv(30, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chIntv);
    emp::EmployeeFilter filter;
    filter.status = "Active";
    filter.limit = 100;
    auto interviewers = emp::EmployeeService::search(filter);
    for (const auto& i : interviewers) {
        chIntv.add((i.employeeCode + " - " + i.fullName() + " (" + i.designation + ")").c_str());
    }
    chIntv.value(0);

    curY += 65;
    Fl_Button btnSchedule(30, curY, 160, 36, "Confirm Schedule");
    Theme::stylePrimaryButton(&btnSchedule);

    Fl_Button btnCancel(200, curY, 100, 36, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool scheduled = false;
    btnSchedule.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &scheduled);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (scheduled) {
            int intvSel = chIntv.value();
            int interviewerId = (intvSel >= 0 && intvSel < (int)interviewers.size()) ? interviewers[intvSel].id : 1;
            std::string round = chRound.text();
            std::string date = inDate.value() ? inDate.value() : "";

            std::string err;
            if (rec::RecruitmentService::scheduleInterview(app.id, interviewerId, date, round, err)) {
                showSuccess("Interview scheduled successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Scheduling failed: " + err);
                scheduled = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Record Interview Feedback
// =============================================================================
void RecruitmentTab::openRecordFeedbackDialog() {
    int row = m_table->getSelectedRow();
    if (row < 0 || row >= (int)m_applications.size()) {
        showError("Please select an application to record feedback.");
        return;
    }
    const auto& app = m_applications[row];
    auto interviews = rec::RecruitmentService::getInterviews(app.id);
    if (interviews.empty()) {
        showError("No scheduled interviews found for this application. Please schedule an interview first.");
        return;
    }
    const auto& intv = interviews.back();

    Fl_Window win(480, 320, "Record Interview Feedback");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 480, 50, ("Feedback: " + app.candidateName + " (" + intv.roundName + ")").c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(15);

    int curY = 65, fieldW = 420, fieldH = 30;

    Fl_Box* lRating = new Fl_Box(30, curY, 200, 16, "Rating (1 - 5 Stars) *");
    lRating->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lRating->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lRating->labelsize(11);
    Fl_Choice chRating(30, curY + 18, 200, fieldH);
    Theme::styleChoice(&chRating);
    chRating.add("5 Stars - Outstanding|4 Stars - Exceeds Expectations|3 Stars - Meets Standards|2 Stars - Below Standards|1 Star - Failed");
    chRating.value(1);

    Fl_Box* lRes = new Fl_Box(250, curY, 200, 16, "Interview Result *");
    lRes->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lRes->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lRes->labelsize(11);
    Fl_Choice chRes(250, curY + 18, 200, fieldH);
    Theme::styleChoice(&chRes);
    chRes.add("Passed|Failed|Pending Decision");
    chRes.value(0);

    curY += 56;
    Fl_Box* lNotes = new Fl_Box(30, curY, fieldW, 16, "Detailed Technical & Cultural Evaluation Notes *");
    lNotes->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lNotes->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lNotes->labelsize(11);
    Fl_Input inNotes(30, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inNotes);
    inNotes.value("Strong systems programming and algorithmic fundamentals. Clear communication.");

    curY += 65;
    Fl_Button btnSave(30, curY, 150, 36, "Save Feedback");
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
            int rating = 5 - chRating.value();
            std::string result = (chRes.value() == 0) ? "Passed" : (chRes.value() == 1 ? "Failed" : "Pending");
            std::string feedback = inNotes.value() ? inNotes.value() : "";

            std::string err;
            if (rec::RecruitmentService::recordInterviewResult(intv.id, rating, feedback, result, err)) {
                showSuccess("Interview result recorded successfully!");
                win.hide();
                refreshData();
                return;
            } else {
                showError("Failed to record result: " + err);
                saved = false;
            }
        }
    }
}

// =============================================================================
// Dialog: Candidate-to-Employee Conversion Wizard (1-Click Pipeline)
// =============================================================================
void RecruitmentTab::openConvertWizardDialog() {
    int row = m_table->getSelectedRow();
    if (row < 0 || row >= (int)m_applications.size()) {
        showError("Please select an application from the table to convert.");
        return;
    }
    const auto& app = m_applications[row];

    Fl_Window win(600, 520, "Candidate Conversion Wizard");
    win.set_modal();
    win.color(Theme::COLOR_BG);

    Fl_Box* header = new Fl_Box(0, 0, 600, 55, ("Onboard Candidate: " + app.candidateName).c_str());
    header->box(FL_FLAT_BOX);
    header->color(Theme::COLOR_HEADER_BG);
    header->labelcolor(Theme::COLOR_SUCCESS);
    header->labelfont(FL_HELVETICA_BOLD);
    header->labelsize(16);

    int col1X = 30, col2X = 310, fieldW = 260, fieldH = 30;
    int curY = 65;

    // Row 1: Designation & Department
    Fl_Box* lDes = new Fl_Box(col1X, curY, fieldW, 16, "Official Designation *");
    lDes->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDes->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDes->labelsize(11);
    Fl_Input inDesig(col1X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inDesig);
    inDesig.value(app.jobTitle.c_str());

    Fl_Box* lSal = new Fl_Box(col2X, curY, fieldW, 16, "Monthly Salary (₹) *");
    lSal->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lSal->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lSal->labelsize(11);
    Fl_Input inSalary(col2X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inSalary);
    inSalary.value("95000");

    // Row 2: DOB & Gender
    curY += 56;
    Fl_Box* lDob = new Fl_Box(col1X, curY, fieldW, 16, "Date of Birth (YYYY-MM-DD) *");
    lDob->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lDob->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lDob->labelsize(11);
    Fl_Input inDob(col1X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inDob);
    inDob.value("1996-05-15");

    Fl_Box* lGen = new Fl_Box(col2X, curY, fieldW, 16, "Gender *");
    lGen->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lGen->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lGen->labelsize(11);
    Fl_Choice chGender(col2X, curY + 18, fieldW, fieldH);
    Theme::styleChoice(&chGender);
    chGender.add("Male|Female|Other");
    chGender.value(0);

    // Row 3: Joining Date & Address
    curY += 56;
    Fl_Box* lJoin = new Fl_Box(col1X, curY, fieldW, 16, "Joining Date (YYYY-MM-DD) *");
    lJoin->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lJoin->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lJoin->labelsize(11);
    Fl_Input inJoin(col1X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inJoin);
    inJoin.value(utils::getCurrentDate().c_str());

    Fl_Box* lAddr = new Fl_Box(col2X, curY, fieldW, 16, "Residential Address");
    lAddr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lAddr->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lAddr->labelsize(11);
    Fl_Input inAddr(col2X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inAddr);
    inAddr.value("Bangalore, Karnataka");

    // Row 4: Bank Account & IFSC
    curY += 56;
    Fl_Box* lAcc = new Fl_Box(col1X, curY, fieldW, 16, "Bank Account Number");
    lAcc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lAcc->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lAcc->labelsize(11);
    Fl_Input inAcc(col1X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inAcc);
    inAcc.value("987654321098");

    Fl_Box* lIfsc = new Fl_Box(col2X, curY, fieldW, 16, "Bank IFSC Code");
    lIfsc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lIfsc->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lIfsc->labelsize(11);
    Fl_Input inIfsc(col2X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inIfsc);
    inIfsc.value("HDFC0001234");

    // Row 5: Emergency Contact & PAN
    curY += 56;
    Fl_Box* lEm = new Fl_Box(col1X, curY, fieldW, 16, "Emergency Contact Name");
    lEm->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lEm->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lEm->labelsize(11);
    Fl_Input inEmerg(col1X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inEmerg);
    inEmerg.value("Parent / Spouse");

    Fl_Box* lPan = new Fl_Box(col2X, curY, fieldW, 16, "PAN / Tax ID");
    lPan->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lPan->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    lPan->labelsize(11);
    Fl_Input inPan(col2X, curY + 18, fieldW, fieldH);
    Theme::styleInput(&inPan);
    inPan.value("ABCDE1234F");

    // Buttons
    curY += 65;
    Fl_Button btnConvert(col1X, curY, 200, 38, "🎉 Complete Onboarding");
    Theme::styleSuccessButton(&btnConvert);

    Fl_Button btnCancel(col1X + 215, curY, 100, 38, "Cancel");
    Theme::styleSecondaryButton(&btnCancel);

    bool converted = false;
    btnConvert.callback([](Fl_Widget*, void* data) { *static_cast<bool*>(data) = true; }, &converted);
    btnCancel.callback([](Fl_Widget* w, void*) { w->window()->hide(); });

    win.end();
    win.show();

    while (win.shown()) {
        Fl::wait();
        if (converted) {
            std::string desig = inDesig.value() ? inDesig.value() : "";
            std::string dob = inDob.value() ? inDob.value() : "";
            std::string gender = chGender.text();
            std::string addr = inAddr.value() ? inAddr.value() : "";
            std::string joinDate = inJoin.value() ? inJoin.value() : "";
            std::string emName = inEmerg.value() ? inEmerg.value() : "";
            std::string emPhone = app.candidatePhone;
            std::string bAcc = inAcc.value() ? inAcc.value() : "";
            std::string bIfsc = inIfsc.value() ? inIfsc.value() : "";
            std::string bName = "HDFC Bank";
            std::string pan = inPan.value() ? inPan.value() : "";
            int64_t salPaise = utils::parsePaise(inSalary.value() ? inSalary.value() : "0");

            std::string err, outCode;
            if (rec::RecruitmentService::convertCandidateToEmployee(
                    app.candidateId, app.id, dob, gender, addr, desig, salPaise,
                    joinDate, emName, emPhone, bAcc, bIfsc, bName, pan, err, outCode)) {
                showSuccess("Candidate converted to full employee record successfully!\nGenerated Employee Code: " + outCode);
                win.hide();
                refreshData();
                return;
            } else {
                showError("Conversion failed: " + err);
                converted = false;
            }
        }
    }
}

} // namespace hrms::gui
