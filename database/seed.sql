-- ==============================================================================
-- Lightweight HRMS - Seed Data
-- ==============================================================================

-- -----------------------------------------------------------------------------
-- 1. Roles
-- -----------------------------------------------------------------------------
INSERT INTO roles (id, name, description) VALUES
(1, 'Administrator', 'Full unrestricted system administration rights'),
(2, 'HR Manager', 'Complete HR operations: hiring, payroll, leave management, employees'),
(3, 'HR Staff', 'Standard HR operational duties: attendance, candidate tracking'),
(4, 'Manager', 'Team management, leave approvals, attendance monitoring'),
(5, 'Employee', 'Self-service: check-in/out, view profile, apply leave, view payslips');

-- -----------------------------------------------------------------------------
-- 2. Leave Types
-- -----------------------------------------------------------------------------
INSERT INTO leave_types (id, code, name, default_days_per_year, is_paid) VALUES
(1, 'SL', 'Sick Leave', 12.0, 1),
(2, 'CL', 'Casual Leave', 12.0, 1),
(3, 'PL', 'Paid Privilege Leave', 18.0, 1),
(4, 'UL', 'Unpaid Leave', 30.0, 0);

-- -----------------------------------------------------------------------------
-- 3. Departments
-- -----------------------------------------------------------------------------
INSERT INTO departments (id, code, name, description, manager_id) VALUES
(1, 'ENG', 'Engineering', 'Software development, infrastructure, and QA', NULL),
(2, 'HR', 'Human Resources', 'People operations, talent acquisition, culture', NULL),
(3, 'FIN', 'Finance & Accounting', 'Financial planning, payroll, audits', NULL),
(4, 'MKT', 'Sales & Marketing', 'Brand growth, enterprise sales, marketing', NULL),
(5, 'OPS', 'Operations & Logistics', 'Workplace operations, facilities, IT support', NULL);

-- -----------------------------------------------------------------------------
-- 4. Employees (Salaries stored in integer paise: 1 INR = 100 paise)
-- -----------------------------------------------------------------------------
INSERT INTO employees (
    id, employee_code, first_name, last_name, dob, gender, phone, email, address,
    department_id, designation, joining_date, employment_status, salary_paise,
    manager_id, emergency_contact_name, emergency_contact_phone,
    bank_account_no, bank_ifsc, bank_name, pan_tax_id
) VALUES
(
    1, 'EMP-1001', 'Rajesh', 'Sharma', '1984-05-12', 'Male', '+919876543210',
    'rajesh.sharma@example.com', 'Flat 402, Green Glen Heights, Bangalore',
    1, 'VP of Engineering', '2020-03-01', 'Active', 18000000, -- ₹1,80,000.00
    NULL, 'Sunita Sharma', '+919876543211', '910012345678', 'HDFC0001234', 'HDFC Bank', 'ABCPS1234F'
),
(
    2, 'EMP-1002', 'Priya', 'Patel', '1988-11-20', 'Female', '+919876543220',
    'priya.patel@example.com', 'Villa 15, Palm Meadows, Bangalore',
    2, 'Head of Human Resources', '2021-01-15', 'Active', 15000000, -- ₹1,50,000.00
    NULL, 'Kiran Patel', '+919876543221', '910012345679', 'ICIC0005678', 'ICICI Bank', 'BCDPS5678G'
),
(
    3, 'EMP-1003', 'Amit', 'Verma', '1992-08-14', 'Male', '+919876543230',
    'amit.verma@example.com', 'A-304, Silicon Residency, Bangalore',
    1, 'Senior Software Engineer', '2021-06-01', 'Active', 12000000, -- ₹1,20,000.00
    1, 'Meena Verma', '+919876543231', '910012345680', 'SBIN0009999', 'State Bank of India', 'CDEPV9999H'
),
(
    4, 'EMP-1004', 'Sneha', 'Gupta', '1995-02-18', 'Female', '+919876543240',
    'sneha.gupta@example.com', '12/4, Indiranagar 1st Stage, Bangalore',
    2, 'HR Operations Specialist', '2022-09-10', 'Active', 6500000, -- ₹65,000.00
    2, 'Ramesh Gupta', '+919876543241', '910012345681', 'KKBK0001111', 'Kotak Mahindra Bank', 'DEFG8888J'
),
(
    5, 'EMP-1005', 'Vikram', 'Singh', '1990-12-05', 'Male', '+919876543250',
    'vikram.singh@example.com', 'House 88, Koramangala 4th Block, Bangalore',
    3, 'Lead Financial Analyst', '2022-02-01', 'Active', 9500000, -- ₹95,000.00
    NULL, 'Pooja Singh', '+919876543251', '910012345682', 'AXIS0002222', 'Axis Bank', 'EFGH7777K'
),
(
    6, 'EMP-1006', 'Ananya', 'Roy', '1996-07-24', 'Female', '+919876543260',
    'ananya.roy@example.com', '201, Whitefield Gardens, Bangalore',
    1, 'Software Engineer', '2023-04-15', 'Active', 8500000, -- ₹85,000.00
    3, 'Dipak Roy', '+919876543261', '910012345683', 'HDFC0001234', 'HDFC Bank', 'FGHI6666L'
);

-- Update Department Managers
UPDATE departments SET manager_id = 1 WHERE id = 1;
UPDATE departments SET manager_id = 2 WHERE id = 2;
UPDATE departments SET manager_id = 5 WHERE id = 3;

-- -----------------------------------------------------------------------------
-- 5. Users (Salt: d8e8fca2dc0f896f, 1000 rounds of SHA-256)
-- admin: admin123
-- hr_mgr: hr12345
-- emp_user: emp12345
-- -----------------------------------------------------------------------------
INSERT INTO users (id, username, password_hash, salt, role_id, employee_id, is_active) VALUES
(1, 'admin', '0369a3b46c0a42b91eee88247c4a62fd2e811a6edde621788e100b82316c8acb', 'd8e8fca2dc0f896f', 1, 1, 1),
(2, 'hr_mgr', '3fe9558f644d568cc99c93bca41784b7354928d15353964b8d093193f99d3f93', 'd8e8fca2dc0f896f', 2, 2, 1),
(3, 'emp_user', 'c2c3d6b2f3528ea306394d0d93866cdc922f95375191752c828fb7d613f407a0', 'd8e8fca2dc0f896f', 5, 3, 1);

-- -----------------------------------------------------------------------------
-- 6. Leave Balances for 2026
-- -----------------------------------------------------------------------------
INSERT INTO leave_balances (employee_id, leave_type_id, year, total_days, used_days, pending_days)
SELECT e.id, lt.id, 2026, lt.default_days_per_year, 0.0, 0.0
FROM employees e CROSS JOIN leave_types lt;

-- -----------------------------------------------------------------------------
-- 7. Leave Requests
-- -----------------------------------------------------------------------------
INSERT INTO leave_requests (id, employee_id, leave_type_id, start_date, end_date, total_days, reason, status, approved_by, approved_at) VALUES
(1, 3, 1, '2026-08-10', '2026-08-11', 2.0, 'Viral fever and rest', 'Approved', 1, '2026-08-09 10:00:00'),
(2, 6, 2, '2026-09-30', '2026-10-01', 2.0, 'Personal family event', 'Pending', NULL, NULL);

-- Update leave balance used for approved request
UPDATE leave_balances SET used_days = 2.0 WHERE employee_id = 3 AND leave_type_id = 1 AND year = 2026;
UPDATE leave_balances SET pending_days = 2.0 WHERE employee_id = 6 AND leave_type_id = 2 AND year = 2026;

-- -----------------------------------------------------------------------------
-- 8. Sample Attendance (Today: 2026-09-29)
-- -----------------------------------------------------------------------------
INSERT INTO attendance (employee_id, attendance_date, check_in, check_out, total_minutes, late_minutes, early_departure_minutes, overtime_minutes, status, notes) VALUES
(1, '2026-09-29', '2026-09-29 09:15:00', NULL, 0, 0, 0, 0, 'Present', 'On-time arrival'),
(2, '2026-09-29', '2026-09-29 09:20:00', NULL, 0, 0, 0, 0, 'Present', 'Morning meeting scheduled'),
(3, '2026-09-29', '2026-09-29 09:45:00', NULL, 0, 15, 0, 0, 'Present', 'Traffic delay'),
(4, '2026-09-29', '2026-09-29 09:10:00', NULL, 0, 0, 0, 0, 'Present', 'Early check-in'),
(5, '2026-09-29', NULL, NULL, 0, 0, 0, 0, 'Absent', 'No check-in recorded yet');

-- Yesterday's Completed Attendance (2026-09-28)
INSERT INTO attendance (employee_id, attendance_date, check_in, check_out, total_minutes, late_minutes, early_departure_minutes, overtime_minutes, status, notes) VALUES
(1, '2026-09-28', '2026-09-28 09:10:00', '2026-09-28 17:45:00', 515, 0, 0, 35, 'Present', 'Standard day'),
(2, '2026-09-28', '2026-09-28 09:25:00', '2026-09-28 17:30:00', 485, 0, 0, 5, 'Present', 'Standard day'),
(3, '2026-09-28', '2026-09-28 09:30:00', '2026-09-28 18:30:00', 540, 0, 0, 60, 'Present', 'Sprint release overtime'),
(4, '2026-09-28', '2026-09-28 09:15:00', '2026-09-28 17:15:00', 480, 0, 0, 0, 'Present', 'Standard day'),
(5, '2026-09-28', '2026-09-28 09:20:00', '2026-09-28 17:30:00', 490, 0, 0, 10, 'Present', 'Standard day'),
(6, '2026-09-28', '2026-09-28 09:15:00', '2026-09-28 18:00:00', 525, 0, 0, 45, 'Present', 'Sprint release overtime');

-- -----------------------------------------------------------------------------
-- 9. Sample Payroll for Previous Period (2026-08)
-- -----------------------------------------------------------------------------
INSERT INTO payroll (
    id, payroll_period, employee_id, basic_salary_paise, allowances_paise,
    bonuses_paise, overtime_paise, deductions_paise, tax_paise, net_salary_paise,
    status, payment_date
) VALUES
(1, '2026-08', 1, 10800000, 5400000, 1800000, 0, 1296000, 2500000, 14204000, 'Paid', '2026-08-31'),
(2, '2026-08', 2, 9000000, 4500000, 1500000, 0, 1080000, 1800000, 12120000, 'Paid', '2026-08-31'),
(3, '2026-08', 3, 7200000, 3600000, 1200000, 250000, 864000, 1200000, 10186000, 'Paid', '2026-08-31');

-- Payroll breakdown items for Employee 3
INSERT INTO payroll_items (payroll_id, item_type, description, amount_paise) VALUES
(3, 'Basic', 'Basic Salary (60%)', 7200000),
(3, 'Allowance', 'House Rent Allowance (HRA)', 2400000),
(3, 'Allowance', 'Special Allowance', 1200000),
(3, 'Bonus', 'Quarterly Performance Bonus', 1200000),
(3, 'Overtime', 'Overtime Pay (5 hours)', 250000),
(3, 'Deduction', 'Provident Fund (PF Employee)', 864000),
(3, 'Tax', 'TDS / Income Tax', 1200000);

-- -----------------------------------------------------------------------------
-- 10. Recruitment: Job Openings, Candidates & Applications
-- -----------------------------------------------------------------------------
INSERT INTO job_openings (id, title, department_id, description, requirements, vacancies, status, posted_date) VALUES
(1, 'Senior C++ Systems Engineer', 1, 'Build high-performance low-latency distributed systems', '5+ years modern C++, Linux internals, multi-threading', 2, 'Open', '2026-09-01'),
(2, 'Talent Acquisition Specialist', 2, 'Lead tech hiring and university recruitment programs', '3+ years tech recruiting, sourcing, ATS knowledge', 1, 'Open', '2026-09-15');

INSERT INTO candidates (id, first_name, last_name, email, phone, experience_years, skills, resume_text, status) VALUES
(1, 'Rohan', 'Deshmukh', 'rohan.d@example.com', '+919876500001', 6.5, 'C++20, Linux, POSIX, NASM, SQLite', '6.5 years backend systems engineering experience at tech scale-up.', 'Active'),
(2, 'Kavita', 'Nair', 'kavita.nair@example.com', '+919876500002', 4.0, 'Technical Recruitment, Sourcing, Interviewing', '4 years corporate recruiter specializing in software engineers.', 'Active');

INSERT INTO applications (id, job_opening_id, candidate_id, application_date, status, notes) VALUES
(1, 1, 1, '2026-09-18', 'Interviewing', 'Strong candidate for low-level systems'),
(2, 2, 2, '2026-09-20', 'Screening', 'Resume under review');

INSERT INTO interviews (id, application_id, interviewer_id, interview_date, round_name, rating, feedback, result) VALUES
(1, 1, 1, '2026-09-25 14:00:00', 'Technical Systems', 5, 'Exceptional understanding of C++ memory models, cache lines and assembly.', 'Passed');

-- -----------------------------------------------------------------------------
-- 11. Initial Audit Log
-- -----------------------------------------------------------------------------
INSERT INTO audit_logs (user_id, action, entity, entity_id, details, ip_address) VALUES
(1, 'INIT', 'System', 1, 'System initialized with schema and seed data', '127.0.0.1');
