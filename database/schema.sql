-- ==============================================================================
-- Lightweight HRMS - SQLite3 Database Schema
-- Normalized, Fully Indexed, Foreign Key Enabled
-- ==============================================================================

PRAGMA foreign_keys = ON;

-- -----------------------------------------------------------------------------
-- 1. Departments
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS departments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    description TEXT,
    manager_id INTEGER,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- -----------------------------------------------------------------------------
-- 2. System Roles (RBAC)
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS roles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL
);

-- -----------------------------------------------------------------------------
-- 3. Employees
-- Salaries stored strictly in integer PAISE (e.g. ₹10,500.75 = 1050075 paise)
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS employees (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_code TEXT NOT NULL UNIQUE,
    first_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    dob TEXT NOT NULL,                         -- YYYY-MM-DD
    gender TEXT NOT NULL,                      -- Male, Female, Other
    phone TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    address TEXT NOT NULL,
    department_id INTEGER NOT NULL REFERENCES departments(id) ON UPDATE CASCADE,
    designation TEXT NOT NULL,
    joining_date TEXT NOT NULL,                -- YYYY-MM-DD
    employment_status TEXT NOT NULL DEFAULT 'Active', -- Active, On Leave, Terminated, Resigned, Probation
    salary_paise INTEGER NOT NULL DEFAULT 0,   -- Fixed-point salary in paise
    manager_id INTEGER REFERENCES employees(id) ON DELETE SET NULL,
    emergency_contact_name TEXT NOT NULL,
    emergency_contact_phone TEXT NOT NULL,
    bank_account_no TEXT NOT NULL,
    bank_ifsc TEXT NOT NULL,
    bank_name TEXT NOT NULL,
    pan_tax_id TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Circular foreign key from departments.manager_id to employees.id
-- SQLite allows deferring or checking via application logic / alter table

-- -----------------------------------------------------------------------------
-- 4. Users & Authentication
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,               -- PBKDF2/SHA256 hex string
    salt TEXT NOT NULL,                        -- Random salt hex string
    role_id INTEGER NOT NULL REFERENCES roles(id) ON UPDATE CASCADE,
    employee_id INTEGER REFERENCES employees(id) ON DELETE SET NULL,
    is_active INTEGER NOT NULL DEFAULT 1,      -- 1=Active, 0=Disabled
    last_login TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- -----------------------------------------------------------------------------
-- 5. Attendance
-- Timestamps stored efficiently in ISO8601 strings, minutes in integer
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS attendance (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE CASCADE,
    attendance_date TEXT NOT NULL,             -- YYYY-MM-DD
    check_in TEXT,                             -- YYYY-MM-DD HH:MM:SS
    check_out TEXT,                            -- YYYY-MM-DD HH:MM:SS
    total_minutes INTEGER NOT NULL DEFAULT 0,
    late_minutes INTEGER NOT NULL DEFAULT 0,
    early_departure_minutes INTEGER NOT NULL DEFAULT 0,
    overtime_minutes INTEGER NOT NULL DEFAULT 0,
    status TEXT NOT NULL DEFAULT 'Present',    -- Present, Absent, Half Day, On Leave, Weekend, Holiday
    notes TEXT,
    CONSTRAINT uq_emp_att_date UNIQUE (employee_id, attendance_date)
);

-- -----------------------------------------------------------------------------
-- 6. Leave Types & Balances
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS leave_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,                 -- SL, CL, PL, UL
    name TEXT NOT NULL,                        -- Sick Leave, Casual Leave, Paid Leave, Unpaid Leave
    default_days_per_year REAL NOT NULL DEFAULT 0,
    is_paid INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS leave_balances (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE CASCADE,
    leave_type_id INTEGER NOT NULL REFERENCES leave_types(id) ON DELETE CASCADE,
    year INTEGER NOT NULL,
    total_days REAL NOT NULL DEFAULT 0,
    used_days REAL NOT NULL DEFAULT 0,
    pending_days REAL NOT NULL DEFAULT 0,
    CONSTRAINT uq_emp_leave_year UNIQUE (employee_id, leave_type_id, year)
);

CREATE TABLE IF NOT EXISTS leave_requests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE CASCADE,
    leave_type_id INTEGER NOT NULL REFERENCES leave_types(id) ON DELETE RESTRICT,
    start_date TEXT NOT NULL,                  -- YYYY-MM-DD
    end_date TEXT NOT NULL,                    -- YYYY-MM-DD
    total_days REAL NOT NULL,
    reason TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'Pending',    -- Pending, Approved, Rejected, Cancelled
    approved_by INTEGER REFERENCES users(id),
    approved_at TEXT,
    rejection_reason TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- -----------------------------------------------------------------------------
-- 7. Payroll & Payslips
-- All monetary figures in integer paise
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS payroll (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    payroll_period TEXT NOT NULL,              -- YYYY-MM
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE RESTRICT,
    basic_salary_paise INTEGER NOT NULL DEFAULT 0,
    allowances_paise INTEGER NOT NULL DEFAULT 0,
    bonuses_paise INTEGER NOT NULL DEFAULT 0,
    overtime_paise INTEGER NOT NULL DEFAULT 0,
    deductions_paise INTEGER NOT NULL DEFAULT 0,
    tax_paise INTEGER NOT NULL DEFAULT 0,
    net_salary_paise INTEGER NOT NULL DEFAULT 0,
    status TEXT NOT NULL DEFAULT 'Draft',      -- Draft, Processed, Paid
    payment_date TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    CONSTRAINT uq_emp_period UNIQUE (employee_id, payroll_period)
);

CREATE TABLE IF NOT EXISTS payroll_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    payroll_id INTEGER NOT NULL REFERENCES payroll(id) ON DELETE CASCADE,
    item_type TEXT NOT NULL,                   -- Allowance, Bonus, Deduction, Tax, Overtime
    description TEXT NOT NULL,
    amount_paise INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------------
-- 8. Recruitment
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS job_openings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    department_id INTEGER NOT NULL REFERENCES departments(id) ON UPDATE CASCADE,
    description TEXT NOT NULL,
    requirements TEXT NOT NULL,
    vacancies INTEGER NOT NULL DEFAULT 1,
    status TEXT NOT NULL DEFAULT 'Open',       -- Draft, Open, Closed, Filled
    posted_date TEXT NOT NULL DEFAULT (date('now')),
    closing_date TEXT
);

CREATE TABLE IF NOT EXISTS candidates (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    first_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    phone TEXT NOT NULL,
    experience_years REAL NOT NULL DEFAULT 0,
    skills TEXT NOT NULL,
    resume_text TEXT,
    status TEXT NOT NULL DEFAULT 'Active',     -- Active, Hired, Rejected, Archived
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS applications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    job_opening_id INTEGER NOT NULL REFERENCES job_openings(id) ON DELETE CASCADE,
    candidate_id INTEGER NOT NULL REFERENCES candidates(id) ON DELETE CASCADE,
    application_date TEXT NOT NULL DEFAULT (date('now')),
    status TEXT NOT NULL DEFAULT 'Applied',    -- Applied, Screening, Interviewing, Offered, Hired, Rejected
    notes TEXT,
    CONSTRAINT uq_job_candidate UNIQUE (job_opening_id, candidate_id)
);

CREATE TABLE IF NOT EXISTS interviews (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    application_id INTEGER NOT NULL REFERENCES applications(id) ON DELETE CASCADE,
    interviewer_id INTEGER REFERENCES employees(id),
    interview_date TEXT NOT NULL,              -- YYYY-MM-DD HH:MM:SS
    round_name TEXT NOT NULL,                  -- Technical, HR, Cultural, Final
    rating INTEGER CHECK (rating BETWEEN 1 AND 5),
    feedback TEXT,
    result TEXT NOT NULL DEFAULT 'Scheduled',  -- Scheduled, Passed, Failed, Cancelled
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- -----------------------------------------------------------------------------
-- 9. Audit Logging & History
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS audit_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER REFERENCES users(id),
    action TEXT NOT NULL,                      -- CREATE, UPDATE, DELETE, LOGIN, LOGOUT, APPROVE, EXPORT
    entity TEXT NOT NULL,                      -- Employee, Attendance, Leave, Payroll, etc.
    entity_id INTEGER,
    details TEXT NOT NULL,
    ip_address TEXT DEFAULT '127.0.0.1',
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS employee_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE CASCADE,
    change_type TEXT NOT NULL,                 -- Designation, Salary, Department, Status
    old_value TEXT,
    new_value TEXT,
    changed_by INTEGER REFERENCES users(id),
    changed_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- ==============================================================================
-- Indexes for Maximum Query Performance
-- ==============================================================================

-- Employees indexes
CREATE INDEX IF NOT EXISTS idx_emp_code ON employees(employee_code);
CREATE INDEX IF NOT EXISTS idx_emp_email ON employees(email);
CREATE INDEX IF NOT EXISTS idx_emp_dept ON employees(department_id);
CREATE INDEX IF NOT EXISTS idx_emp_status ON employees(employment_status);
CREATE INDEX IF NOT EXISTS idx_emp_joining ON employees(joining_date);
CREATE INDEX IF NOT EXISTS idx_emp_mgr ON employees(manager_id);
CREATE INDEX IF NOT EXISTS idx_emp_name ON employees(last_name, first_name);

-- Attendance indexes
CREATE INDEX IF NOT EXISTS idx_att_date ON attendance(attendance_date);
CREATE INDEX IF NOT EXISTS idx_att_emp_date ON attendance(employee_id, attendance_date);
CREATE INDEX IF NOT EXISTS idx_att_status ON attendance(status);

-- Leave indexes
CREATE INDEX IF NOT EXISTS idx_leave_emp_status ON leave_requests(employee_id, status);
CREATE INDEX IF NOT EXISTS idx_leave_status ON leave_requests(status);
CREATE INDEX IF NOT EXISTS idx_leave_dates ON leave_requests(start_date, end_date);
CREATE INDEX IF NOT EXISTS idx_leave_bal ON leave_balances(employee_id, year);

-- Payroll indexes
CREATE INDEX IF NOT EXISTS idx_payroll_period ON payroll(payroll_period);
CREATE INDEX IF NOT EXISTS idx_payroll_emp_period ON payroll(employee_id, payroll_period);
CREATE INDEX IF NOT EXISTS idx_payroll_items_pid ON payroll_items(payroll_id);

-- Recruitment indexes
CREATE INDEX IF NOT EXISTS idx_jobs_status ON job_openings(status);
CREATE INDEX IF NOT EXISTS idx_app_job ON applications(job_opening_id);
CREATE INDEX IF NOT EXISTS idx_app_candidate ON applications(candidate_id);
CREATE INDEX IF NOT EXISTS idx_interviews_app ON interviews(application_id);

-- Users & Audit indexes
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_audit_created ON audit_logs(created_at);
CREATE INDEX IF NOT EXISTS idx_audit_entity ON audit_logs(entity, entity_id);
