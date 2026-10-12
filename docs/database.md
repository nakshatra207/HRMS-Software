# Database Architecture & Optimization Guide

This document covers the SQLite database architecture, table schemas, indexing strategy, transaction isolation, and optimization techniques.

---

## 1. Engine Configuration & Pragmas

On startup, HRMS applies enterprise pragmatic settings to SQLite:

```sql
PRAGMA foreign_keys = ON;       -- Enforce relational integrity
PRAGMA journal_mode = WAL;      -- Write-Ahead Logging (concurrent readers & writers)
PRAGMA synchronous = NORMAL;    -- Maximizes transaction speed with WAL crash safety
PRAGMA busy_timeout = 5000;     -- Wait up to 5000ms if a lock is contested
PRAGMA cache_size = -8000;      -- 8 MB dedicated in-memory page cache
PRAGMA temp_store = MEMORY;     -- Execute sorting and temporary tables in RAM
```

### Why WAL Mode?
Write-Ahead Logging (WAL) significantly improves read/write concurrency. In traditional rollback journal mode, writing to the database completely blocks all reading queries. Under WAL mode, read transactions read from the original DB file or the WAL index while write transactions append to the WAL log without blocking readers.

---

## 2. Relational Schema & Tables

### Normalized Tables
1. **`departments`**: Code, name, description, manager reference.
2. **`roles`**: System permission roles (Administrator, HR Manager, HR Staff, Manager, Employee).
3. **`employees`**: Employee code, personal details, contact, department, designation, joining date, status, manager, bank info, tax info, and `salary_paise` (stored as integer paise).
4. **`users`**: Authentication credentials, username, salted PBKDF2 hash, role reference, active state.
5. **`attendance`**: Daily check-in/out records, total minutes, late minutes, overtime minutes, status.
6. **`leave_types`**: Leave categories (SL, CL, PL, UL), default annual quotas, paid indicator.
7. **`leave_balances`**: Per-employee, per-year allocated, used, and pending leave quotas.
8. **`leave_requests`**: Employee leave requests with status lifecycle (Pending, Approved, Rejected).
9. **`payroll`**: Monthly payroll summaries (Basic, Allowances, Bonus, Overtime, Deductions, Tax, Net Salary).
10. **`payroll_items`**: Granular earning/deduction line items for payslips.
11. **`job_openings`**: Open requisitions, vacancies, job description, status.
12. **`candidates`**: External job applicants with skills, experience, contact details.
13. **`applications`**: Links candidate to job opening with status tracking.
14. **`interviews`**: Scheduled interview rounds, interviewer reference, ratings (1-5), and feedback.
15. **`audit_logs`**: System security and mutation audit trail.
16. **`employee_history`**: Career timeline recording role, department, salary, and status updates.

---

## 3. Indexing Strategy

To guarantee high throughput even when tables grow to tens of thousands of records, indexes are created on frequently filtered and joined columns:

```sql
-- Employee Lookups
CREATE INDEX idx_emp_code ON employees(employee_code);
CREATE INDEX idx_emp_email ON employees(email);
CREATE INDEX idx_emp_dept ON employees(department_id);
CREATE INDEX idx_emp_status ON employees(employment_status);
CREATE INDEX idx_emp_name ON employees(last_name, first_name);

-- Attendance Queries
CREATE INDEX idx_att_date ON attendance(attendance_date);
CREATE INDEX idx_att_emp_date ON attendance(employee_id, attendance_date);

-- Leave Management
CREATE INDEX idx_leave_emp_status ON leave_requests(employee_id, status);
CREATE INDEX idx_leave_bal ON leave_balances(employee_id, year);

-- Payroll Processing
CREATE INDEX idx_payroll_period ON payroll(payroll_period);
CREATE INDEX idx_payroll_emp_period ON payroll(employee_id, payroll_period);
```

---

## 4. Parameterized Statements & Anti-SQL-Injection

SQL queries are **never** constructed by concatenating user input strings. All queries utilize SQLite prepared statements with parameter binding:

```cpp
auto stmt = db.prepare("SELECT id, name FROM employees WHERE department_id = ? AND employment_status = ?;");
stmt->bind(1, departmentId);
stmt->bind(2, "Active");
while (stmt->step()) {
    // Process results
}
```

This guarantees compile-time validation of query structure and completely neutralizes SQL injection vulnerabilities.

---

## 5. Transaction Safety & Nested Autocommit Detection

All mutations spanning multiple tables (such as batch payroll runs, leave approvals, or hiring conversions) are wrapped in `hrms::db::Transaction`.

The `Transaction` class uses `sqlite3_get_autocommit()` to detect if an outer transaction is already active. This allows services to safely compose transactional methods without throwing SQLite nested transaction errors.
