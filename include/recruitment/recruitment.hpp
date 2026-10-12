#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace hrms::rec {

struct JobOpening {
    int id{0};
    std::string title;
    int departmentId{0};
    std::string departmentName;
    std::string description;
    std::string requirements;
    int vacancies{1};
    std::string status{"Open"}; // Draft, Open, Closed, Filled
    std::string postedDate;
    std::string closingDate;
    int applicationCount{0};
};

struct Candidate {
    int id{0};
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string phone;
    double experienceYears{0.0};
    std::string skills;
    std::string resumeText;
    std::string status{"Active"}; // Active, Hired, Rejected, Archived
    std::string createdAt;

    std::string fullName() const {
        return firstName + " " + lastName;
    }
};

struct Application {
    int id{0};
    int jobOpeningId{0};
    std::string jobTitle;
    std::string departmentName;
    int candidateId{0};
    std::string candidateName;
    std::string candidateEmail;
    std::string candidatePhone;
    std::string applicationDate;
    std::string status{"Applied"}; // Applied, Screening, Interviewing, Offered, Hired, Rejected
    std::string notes;
};

struct Interview {
    int id{0};
    int applicationId{0};
    std::string candidateName;
    std::string jobTitle;
    int interviewerId{0};
    std::string interviewerName;
    std::string interviewDate;
    std::string roundName; // Technical, HR, Cultural, Final
    int rating{0};          // 1 - 5
    std::string feedback;
    std::string result{"Scheduled"}; // Scheduled, Passed, Failed, Cancelled
    std::string createdAt;
};

class RecruitmentService {
public:
    // Job Openings
    static std::vector<JobOpening> getJobOpenings(const std::string& status = "");
    static std::optional<JobOpening> getJobOpening(int id);
    static bool createJobOpening(const JobOpening& job, std::string& outError);
    static bool updateJobOpening(const JobOpening& job, std::string& outError);

    // Candidates
    static std::vector<Candidate> getCandidates(const std::string& status = "");
    static std::optional<Candidate> getCandidate(int id);
    static bool createCandidate(Candidate& candidate, std::string& outError);

    // Applications
    static std::vector<Application> getApplications(int jobId = 0, const std::string& status = "");
    static bool applyForJob(int jobId, int candidateId, const std::string& notes, std::string& outError);
    static bool updateApplicationStatus(int applicationId, const std::string& status, std::string& outError);

    // Interviews
    static std::vector<Interview> getInterviews(int applicationId = 0);
    static bool scheduleInterview(int applicationId, int interviewerId, const std::string& date,
                                  const std::string& round, std::string& outError);
    static bool recordInterviewResult(int interviewId, int rating, const std::string& feedback,
                                      const std::string& result, std::string& outError);

    // Candidate-to-Employee Conversion
    static bool convertCandidateToEmployee(int candidateId, int applicationId,
                                           const std::string& dob, const std::string& gender,
                                           const std::string& address, const std::string& designation,
                                           int64_t salaryPaise, const std::string& joiningDate,
                                           const std::string& emergencyName, const std::string& emergencyPhone,
                                           const std::string& bankAcc, const std::string& bankIfsc,
                                           const std::string& bankName, const std::string& panTaxId,
                                           std::string& outError, std::string& outEmployeeCode);
};

} // namespace hrms::rec
