#include "recruitment/recruitment.hpp"
#include "employees/employee.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"

namespace hrms::rec {

// -----------------------------------------------------------------------------
// Job Openings
// -----------------------------------------------------------------------------
std::vector<JobOpening> RecruitmentService::getJobOpenings(const std::string& status) {
    std::vector<JobOpening> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT j.id, j.title, j.department_id, COALESCE(d.name, 'Unassigned'), "
            "j.description, j.requirements, j.vacancies, j.status, j.posted_date, "
            "COALESCE(j.closing_date, ''), "
            "(SELECT count(*) FROM applications a WHERE a.job_opening_id = j.id) as app_count "
            "FROM job_openings j "
            "LEFT JOIN departments d ON j.department_id = d.id "
            "WHERE 1=1 ";

        if (!status.empty()) query += "AND j.status = ? ";
        query += "ORDER BY j.id DESC;";

        auto stmt = db.prepare(query);
        if (!status.empty()) stmt->bind(1, status);

        while (stmt->step()) {
            JobOpening j;
            j.id = stmt->getInt(0);
            j.title = stmt->getString(1);
            j.departmentId = stmt->getInt(2);
            j.departmentName = stmt->getString(3);
            j.description = stmt->getString(4);
            j.requirements = stmt->getString(5);
            j.vacancies = stmt->getInt(6);
            j.status = stmt->getString(7);
            j.postedDate = stmt->getString(8);
            j.closingDate = stmt->getString(9);
            j.applicationCount = stmt->getInt(10);
            list.push_back(std::move(j));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get job openings: " + std::string(ex.what()));
    }
    return list;
}

std::optional<JobOpening> RecruitmentService::getJobOpening(int id) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT j.id, j.title, j.department_id, COALESCE(d.name, 'Unassigned'), "
            "j.description, j.requirements, j.vacancies, j.status, j.posted_date, "
            "COALESCE(j.closing_date, ''), "
            "(SELECT count(*) FROM applications a WHERE a.job_opening_id = j.id) as app_count "
            "FROM job_openings j "
            "LEFT JOIN departments d ON j.department_id = d.id "
            "WHERE j.id = ? LIMIT 1;"
        );
        stmt->bind(1, id);

        if (stmt->step()) {
            JobOpening j;
            j.id = stmt->getInt(0);
            j.title = stmt->getString(1);
            j.departmentId = stmt->getInt(2);
            j.departmentName = stmt->getString(3);
            j.description = stmt->getString(4);
            j.requirements = stmt->getString(5);
            j.vacancies = stmt->getInt(6);
            j.status = stmt->getString(7);
            j.postedDate = stmt->getString(8);
            j.closingDate = stmt->getString(9);
            j.applicationCount = stmt->getInt(10);
            return j;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get job opening: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool RecruitmentService::createJobOpening(const JobOpening& job, std::string& outError) {
    if (job.title.empty()) {
        outError = "Job title cannot be empty";
        return false;
    }
    if (job.departmentId <= 0) {
        outError = "Valid department must be selected";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "INSERT INTO job_openings (title, department_id, description, requirements, vacancies, status, posted_date, closing_date) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
        );
        stmt->bind(1, job.title);
        stmt->bind(2, job.departmentId);
        stmt->bind(3, job.description);
        stmt->bind(4, job.requirements);
        stmt->bind(5, job.vacancies > 0 ? job.vacancies : 1);
        stmt->bind(6, job.status.empty() ? "Open" : job.status);
        stmt->bind(7, job.postedDate.empty() ? utils::getCurrentDate() : job.postedDate);
        if (job.closingDate.empty()) stmt->bindNull(8); else stmt->bind(8, job.closingDate);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CREATE_JOB", "Recruitment", static_cast<int>(db.lastInsertRowId()), "Created job opening: " + job.title);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to create job opening: " + std::string(ex.what());
        return false;
    }
}

bool RecruitmentService::updateJobOpening(const JobOpening& job, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "UPDATE job_openings SET title = ?, department_id = ?, description = ?, "
            "requirements = ?, vacancies = ?, status = ?, closing_date = ? WHERE id = ?;"
        );
        stmt->bind(1, job.title);
        stmt->bind(2, job.departmentId);
        stmt->bind(3, job.description);
        stmt->bind(4, job.requirements);
        stmt->bind(5, job.vacancies);
        stmt->bind(6, job.status);
        if (job.closingDate.empty()) stmt->bindNull(7); else stmt->bind(7, job.closingDate);
        stmt->bind(8, job.id);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "UPDATE_JOB", "Recruitment", job.id, "Updated job opening ID: " + std::to_string(job.id));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to update job opening: " + std::string(ex.what());
        return false;
    }
}

// -----------------------------------------------------------------------------
// Candidates
// -----------------------------------------------------------------------------
std::vector<Candidate> RecruitmentService::getCandidates(const std::string& status) {
    std::vector<Candidate> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT id, first_name, last_name, email, phone, experience_years, skills, "
            "COALESCE(resume_text, ''), status, created_at FROM candidates WHERE 1=1 ";

        if (!status.empty()) query += "AND status = ? ";
        query += "ORDER BY id DESC;";

        auto stmt = db.prepare(query);
        if (!status.empty()) stmt->bind(1, status);

        while (stmt->step()) {
            Candidate c;
            c.id = stmt->getInt(0);
            c.firstName = stmt->getString(1);
            c.lastName = stmt->getString(2);
            c.email = stmt->getString(3);
            c.phone = stmt->getString(4);
            c.experienceYears = stmt->getDouble(5);
            c.skills = stmt->getString(6);
            c.resumeText = stmt->getString(7);
            c.status = stmt->getString(8);
            c.createdAt = stmt->getString(9);
            list.push_back(std::move(c));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get candidates: " + std::string(ex.what()));
    }
    return list;
}

std::optional<Candidate> RecruitmentService::getCandidate(int id) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT id, first_name, last_name, email, phone, experience_years, skills, "
            "COALESCE(resume_text, ''), status, created_at FROM candidates WHERE id = ? LIMIT 1;"
        );
        stmt->bind(1, id);

        if (stmt->step()) {
            Candidate c;
            c.id = stmt->getInt(0);
            c.firstName = stmt->getString(1);
            c.lastName = stmt->getString(2);
            c.email = stmt->getString(3);
            c.phone = stmt->getString(4);
            c.experienceYears = stmt->getDouble(5);
            c.skills = stmt->getString(6);
            c.resumeText = stmt->getString(7);
            c.status = stmt->getString(8);
            c.createdAt = stmt->getString(9);
            return c;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get candidate: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool RecruitmentService::createCandidate(Candidate& candidate, std::string& outError) {
    if (candidate.firstName.empty() || candidate.lastName.empty()) {
        outError = "First name and last name are required";
        return false;
    }
    if (!utils::isValidEmail(candidate.email)) {
        outError = "Invalid email address format";
        return false;
    }
    if (!utils::isValidPhone(candidate.phone)) {
        outError = "Invalid phone number format";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "INSERT INTO candidates (first_name, last_name, email, phone, experience_years, skills, resume_text, status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
        );
        stmt->bind(1, candidate.firstName);
        stmt->bind(2, candidate.lastName);
        stmt->bind(3, candidate.email);
        stmt->bind(4, candidate.phone);
        stmt->bind(5, candidate.experienceYears);
        stmt->bind(6, candidate.skills);
        stmt->bind(7, candidate.resumeText);
        stmt->bind(8, candidate.status.empty() ? "Active" : candidate.status);
        stmt->execute();

        candidate.id = static_cast<int>(db.lastInsertRowId());
        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CREATE_CANDIDATE", "Recruitment", candidate.id, "Registered candidate: " + candidate.fullName());
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to create candidate: " + std::string(ex.what());
        return false;
    }
}

// -----------------------------------------------------------------------------
// Applications
// -----------------------------------------------------------------------------
std::vector<Application> RecruitmentService::getApplications(int jobId, const std::string& status) {
    std::vector<Application> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT a.id, a.job_opening_id, j.title, COALESCE(d.name, 'Unassigned'), "
            "a.candidate_id, (c.first_name || ' ' || c.last_name), c.email, c.phone, "
            "a.application_date, a.status, COALESCE(a.notes, '') "
            "FROM applications a "
            "JOIN job_openings j ON a.job_opening_id = j.id "
            "LEFT JOIN departments d ON j.department_id = d.id "
            "JOIN candidates c ON a.candidate_id = c.id "
            "WHERE 1=1 ";

        if (jobId > 0) query += "AND a.job_opening_id = ? ";
        if (!status.empty()) query += "AND a.status = ? ";
        query += "ORDER BY a.id DESC;";

        auto stmt = db.prepare(query);
        int idx = 1;
        if (jobId > 0) stmt->bind(idx++, jobId);
        if (!status.empty()) stmt->bind(idx++, status);

        while (stmt->step()) {
            Application app;
            app.id = stmt->getInt(0);
            app.jobOpeningId = stmt->getInt(1);
            app.jobTitle = stmt->getString(2);
            app.departmentName = stmt->getString(3);
            app.candidateId = stmt->getInt(4);
            app.candidateName = stmt->getString(5);
            app.candidateEmail = stmt->getString(6);
            app.candidatePhone = stmt->getString(7);
            app.applicationDate = stmt->getString(8);
            app.status = stmt->getString(9);
            app.notes = stmt->getString(10);
            list.push_back(std::move(app));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get applications: " + std::string(ex.what()));
    }
    return list;
}

bool RecruitmentService::applyForJob(int jobId, int candidateId, const std::string& notes, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "INSERT INTO applications (job_opening_id, candidate_id, application_date, status, notes) "
            "VALUES (?, ?, date('now'), 'Applied', ?);"
        );
        stmt->bind(1, jobId);
        stmt->bind(2, candidateId);
        stmt->bind(3, notes);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "APPLY_JOB", "Recruitment", static_cast<int>(db.lastInsertRowId()), "Candidate " + std::to_string(candidateId) + " applied for job " + std::to_string(jobId));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to submit application: " + std::string(ex.what());
        return false;
    }
}

bool RecruitmentService::updateApplicationStatus(int applicationId, const std::string& status, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("UPDATE applications SET status = ? WHERE id = ?;");
        stmt->bind(1, status);
        stmt->bind(2, applicationId);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "UPDATE_APPLICATION", "Recruitment", applicationId, "Application status changed to " + status);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to update application status: " + std::string(ex.what());
        return false;
    }
}

// -----------------------------------------------------------------------------
// Interviews
// -----------------------------------------------------------------------------
std::vector<Interview> RecruitmentService::getInterviews(int applicationId) {
    std::vector<Interview> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT i.id, i.application_id, (c.first_name || ' ' || c.last_name), j.title, "
            "COALESCE(i.interviewer_id, 0), COALESCE(e.first_name || ' ' || e.last_name, 'None'), "
            "i.interview_date, i.round_name, COALESCE(i.rating, 0), COALESCE(i.feedback, ''), "
            "i.result, i.created_at "
            "FROM interviews i "
            "JOIN applications a ON i.application_id = a.id "
            "JOIN candidates c ON a.candidate_id = c.id "
            "JOIN job_openings j ON a.job_opening_id = j.id "
            "LEFT JOIN employees e ON i.interviewer_id = e.id "
            "WHERE 1=1 ";

        if (applicationId > 0) query += "AND i.application_id = ? ";
        query += "ORDER BY i.interview_date DESC;";

        auto stmt = db.prepare(query);
        if (applicationId > 0) stmt->bind(1, applicationId);

        while (stmt->step()) {
            Interview iv;
            iv.id = stmt->getInt(0);
            iv.applicationId = stmt->getInt(1);
            iv.candidateName = stmt->getString(2);
            iv.jobTitle = stmt->getString(3);
            iv.interviewerId = stmt->getInt(4);
            iv.interviewerName = stmt->getString(5);
            iv.interviewDate = stmt->getString(6);
            iv.roundName = stmt->getString(7);
            iv.rating = stmt->getInt(8);
            iv.feedback = stmt->getString(9);
            iv.result = stmt->getString(10);
            iv.createdAt = stmt->getString(11);
            list.push_back(std::move(iv));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get interviews: " + std::string(ex.what()));
    }
    return list;
}

bool RecruitmentService::scheduleInterview(int applicationId, int interviewerId, const std::string& date,
                                           const std::string& round, std::string& outError) {
    if (round.empty()) {
        outError = "Interview round name cannot be empty";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        auto stmt = db.prepare(
            "INSERT INTO interviews (application_id, interviewer_id, interview_date, round_name, result) "
            "VALUES (?, ?, ?, ?, 'Scheduled');"
        );
        stmt->bind(1, applicationId);
        if (interviewerId > 0) stmt->bind(2, interviewerId); else stmt->bindNull(2);
        stmt->bind(3, date);
        stmt->bind(4, round);
        stmt->execute();

        // Update application status to Interviewing
        auto appStmt = db.prepare("UPDATE applications SET status = 'Interviewing' WHERE id = ?;");
        appStmt->bind(1, applicationId);
        appStmt->execute();

        txn.commit();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "SCHEDULE_INTERVIEW", "Recruitment", applicationId, "Scheduled " + round + " interview");
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to schedule interview: " + std::string(ex.what());
        return false;
    }
}

bool RecruitmentService::recordInterviewResult(int interviewId, int rating, const std::string& feedback,
                                               const std::string& result, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("UPDATE interviews SET rating = ?, feedback = ?, result = ? WHERE id = ?;");
        stmt->bind(1, rating);
        stmt->bind(2, feedback);
        stmt->bind(3, result);
        stmt->bind(4, interviewId);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "RECORD_INTERVIEW", "Recruitment", interviewId, "Interview result: " + result);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to record interview result: " + std::string(ex.what());
        return false;
    }
}

// -----------------------------------------------------------------------------
// Candidate-to-Employee Conversion
// -----------------------------------------------------------------------------
bool RecruitmentService::convertCandidateToEmployee(int candidateId, int applicationId,
                                                    const std::string& dob, const std::string& gender,
                                                    const std::string& address, const std::string& designation,
                                                    int64_t salaryPaise, const std::string& joiningDate,
                                                    const std::string& emergencyName, const std::string& emergencyPhone,
                                                    const std::string& bankAcc, const std::string& bankIfsc,
                                                    const std::string& bankName, const std::string& panTaxId,
                                                    std::string& outError, std::string& outEmployeeCode) {
    auto cand = getCandidate(candidateId);
    if (!cand) {
        outError = "Candidate not found";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        // Find department from application's job opening
        int deptId = 1; // Default
        auto appStmt = db.prepare(
            "SELECT j.department_id FROM applications a JOIN job_openings j ON a.job_opening_id = j.id WHERE a.id = ?;"
        );
        appStmt->bind(1, applicationId);
        if (appStmt->step()) {
            deptId = appStmt->getInt(0);
        }

        emp::Employee newEmp;
        newEmp.firstName = cand->firstName;
        newEmp.lastName = cand->lastName;
        newEmp.email = cand->email;
        newEmp.phone = cand->phone;
        newEmp.dob = dob;
        newEmp.gender = gender;
        newEmp.address = address;
        newEmp.departmentId = deptId;
        newEmp.designation = designation;
        newEmp.salaryPaise = salaryPaise;
        newEmp.joiningDate = joiningDate;
        newEmp.employmentStatus = "Active";
        newEmp.emergencyContactName = emergencyName;
        newEmp.emergencyContactPhone = emergencyPhone;
        newEmp.bankAccountNo = bankAcc;
        newEmp.bankIfsc = bankIfsc;
        newEmp.bankName = bankName;
        newEmp.panTaxId = panTaxId;
        newEmp.employeeCode = emp::EmployeeService::generateNextEmployeeCode();

        std::string empErr;
        if (!emp::EmployeeService::create(newEmp, empErr)) {
            outError = "Failed to create employee from candidate: " + empErr;
            return false;
        }

        // Update candidate status to Hired
        auto candStmt = db.prepare("UPDATE candidates SET status = 'Hired' WHERE id = ?;");
        candStmt->bind(1, candidateId);
        candStmt->execute();

        // Update application status to Hired
        auto aStmt = db.prepare("UPDATE applications SET status = 'Hired' WHERE id = ?;");
        aStmt->bind(1, applicationId);
        aStmt->execute();

        txn.commit();

        outEmployeeCode = newEmp.employeeCode;
        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CONVERT_CANDIDATE", "Recruitment", candidateId, "Converted candidate " + cand->fullName() + " to " + newEmp.employeeCode);
        LOG_INFO("Candidate converted to employee: " + newEmp.employeeCode);
        return true;
    } catch (const std::exception& ex) {
        outError = "Error during candidate conversion: " + std::string(ex.what());
        return false;
    }
}

} // namespace hrms::rec
