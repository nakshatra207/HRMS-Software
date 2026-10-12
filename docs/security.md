# Security Model & Access Control Architecture

This document describes the cryptographic controls, authentication pipeline, Role-Based Access Control (RBAC) model, and audit logging system.

---

## 1. Cryptographic Password Hashing

HRMS strictly enforces salted password hashing with key stretching to protect credentials against offline dictionary and rainbow-table attacks.

### Algorithm
* **Primitive**: SHA-256 (FIPS PUB 180-4).
* **Salt Generation**: 16 bytes (128 bits) of cryptographically secure random entropy drawn from Linux `/dev/urandom`, formatted as a 32-character hexadecimal string.
* **Key Stretching**: 1,000 iterative rounds of SHA-256:
  $$\text{Round } 0: H_0 = \text{SHA-256}(\text{salt} \parallel \text{password})$$
  $$\text{Round } i: H_i = \text{SHA-256}(H_{i-1} \parallel \text{salt}) \quad \text{for } 1 \le i < 1000$$
* **Storage**: Stored in `users.password_hash` as a 64-character lowercase hexadecimal string alongside `users.salt`.

### Constant-Time Verification
Password comparison uses a constant-time XOR accumulator to ensure verification execution time is independent of how many initial characters match, defeating timing side-channel attacks:

```cpp
int diff = 0;
for (size_t i = 0; i < computed.size(); ++i) {
    diff |= (computed[i] ^ storedHash[i]);
}
return diff == 0;
```

### Memory Sanitization
Sensitive plaintext password strings in memory are explicitly zeroed out via `secure_zero_memory()` immediately following verification.

---

## 2. Role-Based Access Control (RBAC)

HRMS establishes 5 distinct system roles with granular permission masks:

| Permission | Administrator | HR Manager | HR Staff | Manager | Employee |
|---|:---:|:---:|:---:|:---:|:---:|
| **View Self Profile / Attendance** | Yes | Yes | Yes | Yes | Yes |
| **Check-in / Check-out Self** | Yes | Yes | Yes | Yes | Yes |
| **Submit Leave Request** | Yes | Yes | Yes | Yes | Yes |
| **View Self Payslip** | Yes | Yes | Yes | Yes | Yes |
| **View All Employees** | Yes | Yes | Yes | Yes | No |
| **Create / Edit Employees** | Yes | Yes | No | No | No |
| **Deactivate / Delete Employees**| Yes | No | No | No | No |
| **Approve Leave Requests** | Yes | Yes | No | Yes | No |
| **Manage Attendance Records** | Yes | Yes | No | No | No |
| **Process Payroll Batch** | Yes | Yes | No | No | No |
| **View / Manage Recruitment** | Yes | Yes | Yes | Yes (View) | No |
| **Manage Departments** | Yes | Yes | No | No | No |
| **System Settings / Audit Logs** | Yes | No | No | No | No |

---

## 3. Session Management

* Upon successful authentication, a `Session` object is instantiated containing `userId`, `username`, `role`, `employeeId`, and a cryptographic session token.
* **Inactivity Expiration**: Each user interaction updates `lastActivityTime`. If the elapsed time exceeds `session_timeout` (configured in `config/hrms.conf`, default 1800s / 30 mins), the session is invalidated and the user is redirected to the login prompt.

---

## 4. Comprehensive Audit Logging

All system events, security logins, and data mutations are logged in the `audit_logs` table:
* `user_id`: Acting operator ID (or `NULL` for system startup).
* `action`: Event classification (`LOGIN`, `LOGOUT`, `CREATE`, `UPDATE`, `DELETE`, `APPROVE_LEAVE`, `PROCESS_PAYROLL`, `EXPORT_REPORT`).
* `entity`: Affected subsystem (`Employee`, `Attendance`, `Leave`, `Payroll`, `Department`, `User`).
* `entity_id`: Primary key of the affected row.
* `details`: Human-readable summary of the action.
* `created_at`: UTC/local timestamp.

### Redaction Policy
Passwords, session tokens, bank account numbers, and cryptographic keys are **never** logged to disk or stored in `audit_logs` details.
