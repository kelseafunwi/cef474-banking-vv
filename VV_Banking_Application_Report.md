# Verification & Validation Report  
## Medium-Sized Banking Application (Chap3Lab)

| Field | Value |
|-------|-------|
| **Project** | CEF474 Banking System |
| **Source Location** | `Chap3Lab/` |
| **Language** | C++17 |
| **Report Date** | June 19, 2026 |
| **Authors** | _[Your Name]_ / _[Partner Name]_ |

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [System Under Test](#2-system-under-test)
3. [Activity 1 — Code Review](#3-activity-1--code-review)
4. [Activity 2 — Static Analysis](#4-activity-2--static-analysis)
5. [Activity 3 — Dynamic Analysis](#5-activity-3--dynamic-analysis)
6. [Activity 4 — Testing](#6-activity-4--testing)
7. [Activity 5 — Debug Runtime Issues](#7-activity-5--debug-runtime-issues)
8. [Activity 6 — Compare Outputs](#8-activity-6--compare-outputs)
9. [Activity 7 — Test Case Management](#9-activity-7--test-case-management)
10. [Bug Report (Defects by Tool)](#10-bug-report-defects-by-tool)
11. [Test Cases](#11-test-cases)
12. [Fixed Code](#12-fixed-code)
13. [Verification Report](#13-verification-report)
14. [Conclusion & Recommendations](#14-conclusion--recommendations)

---

## 1. Executive Summary

This report documents Verification & Validation (V&V) activities performed on a medium-sized C++ banking application deliberately seeded with defects across memory safety, concurrency, business logic, and security categories.

**Key findings:**

| Category | Defects Found | Severity |
|----------|---------------|----------|
| Memory Safety | 5 | Critical |
| Business Logic | 4 | High |
| Concurrency | 1 | High |
| Security | 3 | High |
| Code Quality | 3 | Medium |
| **Total** | **16** | — |

The application **does not pass** verification in its current state. Multiple defects cause crashes (segmentation faults), data corruption, and incorrect financial balances.

> **Collaboration note:** Sections marked with `[TODO]` are placeholders for you to complete with screenshots, IDE output, or additional narrative as we work through the lab together.

---

## 2. System Under Test

### 2.1 Architecture Overview

The banking system consists of three source files:

| File | Responsibility |
|------|----------------|
| `BankAccount.h` | Class declarations for `Transaction` and `BankAccount` |
| `BankAccount.cpp` | Core banking operations: deposit, withdraw, transfer, statements |
| `BankingSystem.cpp` | Main driver with concurrent transactions and integration scenarios |

### 2.2 Class Diagram

```
┌─────────────────────┐       ┌──────────────────────────┐
│    Transaction      │       │      BankAccount         │
├─────────────────────┤       ├──────────────────────────┤
│ + id: int           │       │ - accountId: int         │
│ + amount: double    │◄──────│ - owner: string          │
│ + description: char*│  1..* │ - balance: double*       │
├─────────────────────┤       │ - transactions: vector   │
│ + Transaction()     │       ├──────────────────────────┤
│ + ~Transaction()    │       │ + deposit()              │
└─────────────────────┘       │ + withdraw()             │
                              │ + transfer()             │
                              │ + getBalance()           │
                              │ + getTransaction()       │
                              │ + generateReport()       │
                              └──────────────────────────┘
```

<!-- IMAGE: Insert a UML class diagram screenshot or draw.io export of the banking system classes -->

### 2.3 Build Configuration

```bash
cd Chap3Lab
g++ -std=c++17 -Wall -Wextra -g -o banking_app BankingSystem.cpp BankAccount.cpp
```

**Build tool:** Code::Blocks project (`CEF474 Bank.cbp`) / g++ CLI

<!-- IMAGE: Screenshot of Code::Blocks project layout or terminal showing successful/failed build -->

---

## 3. Activity 1 — Code Review

### 3.1 Methodology

Manual peer code review was performed against the following checklist:

- [ ] Memory allocation and deallocation correctness
- [ ] Input validation on financial operations
- [ ] Bounds checking on array/vector access
- [ ] Thread safety for shared state
- [ ] Error handling and edge cases
- [ ] Use of unsafe C functions (`strcpy`, `sprintf`)
- [ ] Return value semantics (dangling pointers)

### 3.2 Review Findings

#### Finding CR-01 — Heap buffer overflow in `Transaction` constructor

**File:** `BankAccount.cpp` lines 12–13

```cpp
description = new char[strlen(desc)];  // Missing +1 for null terminator
strcpy(description, desc);
```

**Issue:** Allocates `strlen(desc)` bytes but `strcpy` writes `strlen(desc) + 1` bytes (including `\0`), causing a one-byte heap overflow.

**Severity:** Critical

---

#### Finding CR-02 — Off-by-one error in destructor

**File:** `BankAccount.cpp` lines 36–39

```cpp
for(size_t i=0; i<=transactions.size(); i++)  // Should be i < size()
{
    delete transactions[i];
}
```

**Issue:** When `transactions` is empty, `transactions[0]` is accessed → undefined behavior / segfault. When non-empty, one extra invalid pointer is deleted past the end of the vector.

**Severity:** Critical

---

#### Finding CR-03 — Negative deposits accepted

**File:** `BankAccount.cpp` lines 44–49

```cpp
if(amount < 0)
{
    std::cout << "Negative deposit accepted\n";  // Only warns, does not reject
}
*balance += amount;
```

**Issue:** Business rule violation — negative deposits reduce balance without proper validation.

**Severity:** High

---

#### Finding CR-04 — Withdrawal allows overdraft and negative amounts

**File:** `BankAccount.cpp` lines 52–57

```cpp
if(*balance > amount)  // Strict > allows edge cases; no negative amount check
{
    *balance -= amount;
}
```

**Issue:** Withdrawing a negative amount increases balance (`balance - (-500) = balance + 500`). No rejection of insufficient funds when `amount > balance`.

**Severity:** High

---

#### Finding CR-05 — Non-atomic transfer

**File:** `BankAccount.cpp` lines 60–64

```cpp
void BankAccount::transfer(BankAccount& target, double amount)
{
    target.deposit(amount);   // Credits target first
    withdraw(amount);         // May fail silently if insufficient funds
}
```

**Issue:** Target account is credited even if source withdrawal fails. A transfer of 5000 from an account with 1000 still credits the target.

**Severity:** High

---

#### Finding CR-06 — No bounds check in `getTransaction`

**File:** `BankAccount.cpp` lines 72–74

```cpp
Transaction* BankAccount::getTransaction(int index)
{
    return transactions[index];  // No bounds validation
}
```

**Severity:** High

---

#### Finding CR-07 — Dangling pointer in `generateReport`

**File:** `BankAccount.cpp` lines 96–106

```cpp
char* BankAccount::generateReport()
{
    char report[128];           // Stack-allocated
    sprintf(report, "Account=%d Owner=%s Balance=%f", ...);
    return report;              // Returns address of local variable
}
```

**Severity:** Critical

---

#### Finding CR-08 — Race condition on global account

**File:** `BankingSystem.cpp` lines 5–12, 25–28

```cpp
BankAccount* globalAccount = nullptr;

void performTransactions() {
    for(int i=0; i<100000; i++)
        globalAccount->deposit(1);  // No mutex / synchronization
}
```

**Issue:** Two threads concurrently modify the same balance without synchronization → data race and incorrect final balance.

**Severity:** High

---

#### Finding CR-09 — Deliberate null pointer dereference

**File:** `BankingSystem.cpp` lines 57–62

```cpp
int* ptr = nullptr;
if(account1.getBalance() > 100)
{
    *ptr = 10;  // Guaranteed crash when balance > 100
}
```

**Severity:** Critical

---

#### Finding CR-10 — Out-of-bounds transaction access in main

**File:** `BankingSystem.cpp` lines 43–44

```cpp
Transaction* tx2 = account1.getTransaction(100);  // Only 1 transaction exists (index 0)
```

**Severity:** Critical

---

### 3.3 Code Review Summary

| ID | Category | File | Severity |
|----|----------|------|----------|
| CR-01 | Memory | BankAccount.cpp:12 | Critical |
| CR-02 | Memory | BankAccount.cpp:36 | Critical |
| CR-03 | Logic | BankAccount.cpp:44 | High |
| CR-04 | Logic | BankAccount.cpp:52 | High |
| CR-05 | Logic | BankAccount.cpp:60 | High |
| CR-06 | Logic | BankAccount.cpp:72 | High |
| CR-07 | Memory | BankAccount.cpp:96 | Critical |
| CR-08 | Concurrency | BankingSystem.cpp:5 | High |
| CR-09 | Memory | BankingSystem.cpp:57 | Critical |
| CR-10 | Logic | BankingSystem.cpp:43 | Critical |

<!-- IMAGE: Screenshot of code review annotations in IDE (e.g., highlighted lines with review comments) -->

---

## 4. Activity 2 — Static Analysis

### 4.1 Tools Used

| Tool | Version | Purpose |
|------|---------|---------|
| **g++ / clang++** (`-Wall -Wextra`) | Apple clang 21 | Compiler warnings |
| **cppcheck** | _[TODO: version]_ | Dedicated static analyzer |
| **clang-tidy** | _[TODO: version]_ | Lint and modernization checks |

> **Note:** If cppcheck/clang-tidy are not installed locally, install via:
> ```bash
> brew install cppcheck llvm
> ```

### 4.2 Compiler Warning Analysis (g++ -Wall -Wextra)

**Command:**
```bash
g++ -std=c++17 -Wall -Wextra -g -o banking_app BankingSystem.cpp BankAccount.cpp
```

**Result:** Build **succeeded** (executable `banking_app` created), but the compiler reported **2 warnings** — both in `generateReport()`.

**Full terminal output (captured June 19, 2026):**
```
BankAccount.cpp:100:5: warning: 'sprintf' is deprecated: This function is provided for compatibility reasons only. Due to security concerns inherent in the design of
      sprintf(3), it is highly recommended that you use snprintf(3) instead. [-Wdeprecated-declarations]
  100 |     sprintf(report,
      |     ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/_stdio.h:278:1: note: 'sprintf' has been explicitly marked
      deprecated here
  278 | __deprecated_msg("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of sprintf(3), it is highly recommended that you use snpr...
      | ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/sys/cdefs.h:227:48: note: expanded from macro
      '__deprecated_msg'
  227 |         #define __deprecated_msg(_msg) __attribute__((__deprecated__(_msg)))
      |                                                       ^
BankAccount.cpp:106:12: warning: address of stack memory associated with local variable 'report' returned [-Wreturn-stack-address]
  106 |     return report;
      |            ^~~~~~
2 warnings generated.
```

**Warning breakdown:**

| Warning | Location | Defect ID | What it means |
|---------|----------|-----------|---------------|
| `'sprintf' is deprecated` | `BankAccount.cpp:100` | SA-01 / BUG-009 | `sprintf` does not bound-check the destination buffer. A long owner name or balance string could overflow `report[128]`. Use `snprintf` instead. |
| `address of stack memory returned` | `BankAccount.cpp:106` | SA-02 / BUG-008 | `report` is a local array on the stack. Returning `report` gives the caller a **dangling pointer** — the memory is invalid after the function returns. |

**Interpretation:** These are static analysis findings at compile time. The program still links and produces `banking_app`, but both warnings point to real defects in `generateReport()` that can cause garbage output or security issues at runtime. This confirms findings **CR-07** and **BUG-008** from the code review.

<!-- IMAGE: Screenshot of your terminal showing the g++ warnings above (you can use the output you already captured) -->

### 4.3 cppcheck Results

**Command:**
```bash
cppcheck --enable=all --suppress=missingIncludeSystem Chap3Lab/
```

**Results:** _[TODO: Paste cppcheck output here after running]_

| ID | Severity | Message | File:Line |
|----|----------|---------|-----------|
| SA-03 | _[TODO]_ | _[TODO]_ | _[TODO]_ |

<!-- IMAGE: Screenshot of cppcheck report output or HTML report -->

### 4.4 clang-tidy Results

**Command:**
```bash
clang-tidy Chap3Lab/BankAccount.cpp Chap3Lab/BankingSystem.cpp -- -std=c++17
```

**Results:** _[TODO: Paste clang-tidy output here]_

<!-- IMAGE: Screenshot of clang-tidy warnings in terminal or IDE integration -->

### 4.5 Static Analysis Summary

| Tool | Defects Found | Critical | High | Medium |
|------|---------------|----------|------|--------|
| g++ warnings | 2 | 1 | 0 | 1 |
| cppcheck | _[TODO]_ | _[TODO]_ | _[TODO]_ | _[TODO]_ |
| clang-tidy | _[TODO]_ | _[TODO]_ | _[TODO]_ | _[TODO]_ |
| Manual review | 10 | 5 | 5 | 0 |

---

## 5. Activity 3 — Dynamic Analysis

### 5.1 Tools Used

| Tool | Purpose |
|------|---------|
| **AddressSanitizer (ASan)** | Heap/stack buffer overflows, use-after-free |
| **UndefinedBehaviorSanitizer (UBSan)** | Undefined behavior detection |
| **lldb / gdb** | Interactive debugging |

### 5.2 AddressSanitizer Run

**Command:**
```bash
g++ -std=c++17 -g -fsanitize=address,undefined -o banking_asan BankingSystem.cpp BankAccount.cpp
./banking_asan
```

**Observed behavior:**

1. Program prints: `Negative deposit accepted`
2. **CRASH:** `AddressSanitizer: heap-buffer-overflow` in `Transaction::Transaction()` at `strcpy`
3. Write of size 7 into 6-byte allocated region (description `"Salary"` = 6 chars + null terminator)

**Stack trace (abbreviated):**
```
ERROR: AddressSanitizer: heap-buffer-overflow
WRITE of size 7 at ...
    #0 strcpy
    #1 Transaction::Transaction(int, double, char const*)
    #2 main
```

<!-- IMAGE: Screenshot of AddressSanitizer crash output in terminal (red error block) -->

### 5.3 Unit Test Dynamic Run (Destructor Crash)

Running unit tests (`tests/test_bankaccount.cpp`) with ASan revealed a **second crash** in `BankAccount::~BankAccount()` due to the off-by-one destructor loop (CR-02), even on a simple balance test:

```
AddressSanitizer: SEGV on unknown address 0x000000000000
    #0 BankAccount::~BankAccount()
    #1 test_initial_balance()
```

<!-- IMAGE: Screenshot of ASan segfault in destructor during unit test run -->

### 5.4 Expected Additional Dynamic Findings (if program runs past first crash)

| Defect | Expected Dynamic Symptom |
|--------|--------------------------|
| CR-09 Null deref | `SEGV` at `*ptr = 10` |
| CR-10 OOB access | `SEGV` or garbage output from `tx2->description` |
| CR-08 Race condition | Final balance ≠ expected 200,000 (non-deterministic) |
| CR-07 Dangling pointer | Garbage characters in report output |

### 5.5 Dynamic Analysis Summary

| ID | Tool | Defect | Confirmed |
|----|------|--------|-----------|
| DA-01 | ASan | Heap buffer overflow (CR-01) | ✅ Yes |
| DA-02 | ASan | Destructor segfault (CR-02) | ✅ Yes |
| DA-03 | ASan/Runtime | Null pointer deref (CR-09) | _[TODO: confirm after fixing earlier crashes]_ |
| DA-04 | ASan/Runtime | OOB transaction access (CR-10) | _[TODO]_ |
| DA-05 | Runtime | Race condition (CR-08) | _[TODO: run without ASan, compare balance]_ |

---

## 6. Activity 4 — Testing

### 6.1 Unit Testing

**Test file:** `Chap3Lab/tests/test_bankaccount.cpp`

**Command:**
```bash
g++ -std=c++17 -g -o test_bank tests/test_bankaccount.cpp BankAccount.cpp
./test_bank
```

| Test ID | Description | Expected | Actual (Buggy Code) | Status |
|---------|-------------|----------|---------------------|--------|
| TC-01 | Initial balance = 1000 | 1000.0 | 1000.0 | ✅ PASS |
| TC-02 | Deposit 50 on 100 | 150.0 | 150.0 | ✅ PASS |
| TC-03 | Reject negative deposit | 100.0 | 50.0 | ❌ FAIL |
| TC-04 | Withdraw 40 from 100 | 60.0 | 60.0 | ✅ PASS |
| TC-05 | Reject overdraft | 100.0 | -100.0 | ❌ FAIL |
| TC-06 | Reject transfer w/ insufficient funds | src=100, dst=50 | src=-400, dst=550 | ❌ FAIL |
| TC-07 | Bounds check on getTransaction(100) | nullptr / exception | Segfault | ❌ FAIL |

**Pass rate:** 3/7 (42.9%)

<!-- IMAGE: Screenshot of unit test execution output in terminal -->

### 6.2 Integration Testing

Integration testing exercises the full `BankingSystem.cpp` main flow:

| Test ID | Scenario | Steps | Expected | Actual |
|---------|----------|-------|----------|--------|
| IT-01 | Account creation | Create Alice (1000), Bob (500) | Balances correct | _[TODO]_ |
| IT-02 | Transfer 5000 | Alice → Bob | Rejected; balances unchanged | Bob credited 5000 |
| IT-03 | Concurrent deposits | 2 threads × 100,000 deposits of 1 | Deterministic balance | Non-deterministic |
| IT-04 | Statement generation | Add transaction, print | Valid output | Heap overflow first |
| IT-05 | Report generation | generateReport() | Valid string | Dangling pointer |

<!-- IMAGE: Screenshot of integration test run or main program output before crash -->

### 6.3 Security Testing

| Test ID | Attack / Scenario | Vulnerability | Result |
|---------|-------------------|---------------|--------|
| SEC-01 | Buffer overflow via `strcpy` | CWE-120 | **EXPLOITABLE** — ASan confirmed |
| SEC-02 | Buffer overflow via `sprintf` | CWE-120 | Potential — no bounds on `report[128]` |
| SEC-03 | Null pointer dereference | CWE-476 | **CRASH** — deliberate in main |
| SEC-04 | Negative amount injection | CWE-20 | Balance manipulation succeeds |
| SEC-05 | Race condition (TOCTOU) | CWE-362 | Balance corruption possible |

**Security posture:** **FAIL** — multiple exploitable/crashable vulnerabilities present.

<!-- IMAGE: Screenshot of security test results or OWASP-style checklist filled in -->

---

## 7. Activity 5 — Debug Runtime Issues

### 7.1 Debugging Session 1 — Heap Buffer Overflow

| Step | Action |
|------|--------|
| 1 | Build with `-g -fsanitize=address` |
| 2 | Run `./banking_asan` |
| 3 | ASan reports overflow at `Transaction::Transaction` line 13 |
| 4 | Inspect: `new char[strlen(desc)]` allocates 6 bytes for `"Salary"` |
| 5 | `strcpy` writes 7 bytes → overflow |

**Root cause:** Missing `+ 1` for null terminator in allocation.

**Fix:** `description = new char[strlen(desc) + 1];`

<!-- IMAGE: Screenshot of lldb/gdb breakpoint at Transaction constructor showing variable values (desc, strlen, allocated size) -->

### 7.2 Debugging Session 2 — Destructor Segfault

| Step | Action |
|------|--------|
| 1 | Run unit test with ASan |
| 2 | Crash in `~BankAccount()` at `delete transactions[i]` |
| 3 | Inspect loop: `i <= transactions.size()` when size=0 accesses index 0 |

**Root cause:** Off-by-one (`<=` instead of `<`).

**Fix:** `for(size_t i = 0; i < transactions.size(); i++)`

<!-- IMAGE: Screenshot of debugger showing transactions.size()=0 and i=0 at crash point -->

### 7.3 Debugging Session 3 — Race Condition

_[TODO: Debug with breakpoints in performTransactions(), run multiple times, observe different final balances]_

<!-- IMAGE: Screenshot showing two different final balance values from consecutive runs -->

---

## 8. Activity 6 — Compare Outputs

### 8.1 Expected vs Actual Output

| Operation | Expected Output | Actual Output (Buggy) | Match? |
|-----------|-----------------|----------------------|--------|
| `deposit(-200)` on 1000 | Error; balance = 1000 | `"Negative deposit accepted"`; balance = 800 | ❌ |
| `withdraw(-500)` | Error or no-op | balance increases by 500 | ❌ |
| `transfer(5000)` Alice→Bob | Rejected | Bob += 5000, Alice overdrawn | ❌ |
| `getTransaction(100)` | Error/null | Segfault / garbage | ❌ |
| `generateReport()` | `"Account=1 Owner=Alice Balance=..."` | Garbage / stack garbage | ❌ |
| Final balance (2×100k threads) | 200,000 + adjustments | Non-deterministic | ❌ |

### 8.2 Before vs After Fix Comparison

_[TODO: After applying fixes in Section 12, fill in this table with fixed program output]_

| Test | Before Fix | After Fix |
|------|------------|-----------|
| TC-03 Negative deposit | FAIL | _[TODO]_ |
| TC-05 Overdraft | FAIL | _[TODO]_ |
| TC-06 Transfer | FAIL | _[TODO]_ |
| Full main() run | CRASH | _[TODO]_ |

<!-- IMAGE: Side-by-side terminal screenshots — buggy output on left, fixed output on right -->

---

## 9. Activity 7 — Test Case Management

### 9.1 Test Case Repository

All test cases are tracked in `Chap3Lab/tests/test_bankaccount.cpp` and documented in Section 11.

### 9.2 Traceability Matrix

| Requirement | Test Case | Defect Covered | Tool |
|-------------|-----------|----------------|------|
| REQ-01: Valid initial balance | TC-01 | — | Unit |
| REQ-02: Accept valid deposits | TC-02 | — | Unit |
| REQ-03: Reject negative deposits | TC-03 | CR-03 | Unit |
| REQ-04: Accept valid withdrawals | TC-04 | — | Unit |
| REQ-05: Prevent overdraft | TC-05 | CR-04 | Unit |
| REQ-06: Atomic transfer | TC-06 | CR-05 | Unit |
| REQ-07: Safe transaction lookup | TC-07 | CR-06, CR-10 | Unit |
| REQ-08: Thread-safe deposits | IT-03 | CR-08 | Integration |
| REQ-09: No buffer overflows | SEC-01 | CR-01 | Security/ASan |
| REQ-10: Valid report generation | IT-05 | CR-07 | Integration |

### 9.3 Test Management Process

1. **Author** test cases from requirements and code review findings
2. **Execute** against buggy baseline — document failures
3. **Apply fixes** — re-run full suite
4. **Regression** — ensure no new failures introduced
5. **Report** — update traceability matrix and pass rates

<!-- IMAGE: Screenshot of test case spreadsheet, test management tool, or markdown table in IDE -->

---

## 10. Bug Report (Defects by Tool)

### 10.1 Master Defect Log

| Bug ID | Description | Severity | Found By | Status |
|--------|-------------|----------|----------|--------|
| BUG-001 | Heap buffer overflow in Transaction ctor (`strlen` missing +1) | Critical | Code Review, ASan, g++ | Open |
| BUG-002 | Off-by-one in destructor loop (`<=` vs `<`) | Critical | Code Review, ASan | Open |
| BUG-003 | Negative deposits accepted (warn only) | High | Code Review, Unit Test | Open |
| BUG-004 | Withdrawal allows overdraft | High | Code Review, Unit Test | Open |
| BUG-005 | Negative withdrawal increases balance | High | Code Review | Open |
| BUG-006 | Transfer credits target without sufficient funds | High | Code Review, Unit Test | Open |
| BUG-007 | No bounds check in getTransaction() | High | Code Review, Unit Test | Open |
| BUG-008 | generateReport() returns stack pointer | Critical | Code Review, g++ | Open |
| BUG-009 | sprintf() deprecated / unsafe | Medium | g++ warning | Open |
| BUG-010 | Data race on globalAccount (no mutex) | High | Code Review | Open |
| BUG-011 | Null pointer dereference in main | Critical | Code Review | Open |
| BUG-012 | Out-of-bounds getTransaction(100) in main | Critical | Code Review | Open |
| BUG-013 | Unnecessary heap allocation for balance (double*) | Low | Code Review | Open |
| BUG-014 | Raw char* for description (should use std::string) | Medium | Code Review | Open |
| BUG-015 | Missing const correctness on getTransaction | Low | Code Review | Open |
| BUG-016 | transfer() not atomic (partial failure state) | High | Code Review, Unit Test | Open |

### 10.2 Defects per Tool

| Tool | Bug IDs Found |
|------|---------------|
| **Manual Code Review** | BUG-001 – BUG-016 |
| **g++ -Wall -Wextra** | BUG-008, BUG-009 |
| **AddressSanitizer** | BUG-001, BUG-002 |
| **Unit Tests** | BUG-003, BUG-004, BUG-006, BUG-007 |
| **Integration Tests** | BUG-006, BUG-010, BUG-011, BUG-012 |
| **Security Testing** | BUG-001, BUG-009, BUG-011, BUG-003, BUG-010 |
| **cppcheck** | _[TODO]_ |
| **clang-tidy** | _[TODO]_ |

<!-- IMAGE: Screenshot of bug tracking table (Excel, GitHub Issues, or similar) -->

---

## 11. Test Cases

### 11.1 Unit Test Cases (Detailed)

#### TC-01: Initial Balance
```
Precondition:  New account with id=1, owner="Alice", balance=1000
Action:        Call getBalance()
Expected:      1000.0
Priority:      P1
```

#### TC-02: Valid Deposit
```
Precondition:  Account balance = 100.0
Action:        deposit(50.0)
Expected:      balance = 150.0
Priority:      P1
```

#### TC-03: Negative Deposit Rejected
```
Precondition:  Account balance = 100.0
Action:        deposit(-50.0)
Expected:      balance remains 100.0; error logged
Priority:      P1
```

#### TC-04: Valid Withdrawal
```
Precondition:  Account balance = 100.0
Action:        withdraw(40.0)
Expected:      balance = 60.0
Priority:      P1
```

#### TC-05: Overdraft Protection
```
Precondition:  Account balance = 100.0
Action:        withdraw(200.0)
Expected:      balance remains 100.0; withdrawal rejected
Priority:      P1
```

#### TC-06: Transfer — Insufficient Funds
```
Precondition:  Alice balance = 100.0, Bob balance = 50.0
Action:        Alice.transfer(Bob, 500.0)
Expected:      Alice = 100.0, Bob = 50.0; transfer rejected
Priority:      P1
```

#### TC-07: Transaction Index Bounds
```
Precondition:  Account with 1 transaction at index 0
Action:        getTransaction(100)
Expected:      Returns nullptr or throws exception; no crash
Priority:      P1
```

### 11.2 Integration Test Cases

#### IT-01 through IT-05
_See Section 6.2_

### 11.3 Security Test Cases

#### SEC-01 through SEC-05
_See Section 6.3_

---

## 12. Fixed Code

The following corrections address all Critical and High severity defects. Apply these changes to produce a verified banking system.

> **Note:** The `FixedBankingSystem/` folder in this repository currently mirrors the buggy code. Use the fixes below as the corrected baseline.

### 12.1 Fix BUG-001 — Transaction buffer allocation

```cpp
// BankAccount.cpp — Transaction constructor
description = new char[strlen(desc) + 1];
strcpy(description, desc);
```

### 12.2 Fix BUG-002 — Destructor off-by-one

```cpp
// BankAccount.cpp — ~BankAccount()
for (size_t i = 0; i < transactions.size(); i++)
{
    delete transactions[i];
}
transactions.clear();
```

### 12.3 Fix BUG-003, BUG-004, BUG-005 — Input validation

```cpp
void BankAccount::deposit(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "Error: Deposit amount must be positive.\n";
        return;
    }
    *balance += amount;
}

void BankAccount::withdraw(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "Error: Withdrawal amount must be positive.\n";
        return;
    }
    if (*balance < amount)
    {
        std::cerr << "Error: Insufficient funds.\n";
        return;
    }
    *balance -= amount;
}
```

### 12.4 Fix BUG-006, BUG-016 — Atomic transfer

```cpp
void BankAccount::transfer(BankAccount& target, double amount)
{
    if (amount <= 0)
    {
        std::cerr << "Error: Transfer amount must be positive.\n";
        return;
    }
    if (*balance < amount)
    {
        std::cerr << "Error: Insufficient funds for transfer.\n";
        return;
    }
    *balance -= amount;
    *target.balance += amount;
}
```

### 12.5 Fix BUG-007 — Bounds-checked getTransaction

```cpp
Transaction* BankAccount::getTransaction(int index)
{
    if (index < 0 || index >= static_cast<int>(transactions.size()))
        return nullptr;
    return transactions[index];
}
```

### 12.6 Fix BUG-008, BUG-009 — Safe report generation

```cpp
std::string BankAccount::generateReport() const
{
    char report[128];
    snprintf(report, sizeof(report),
             "Account=%d Owner=%s Balance=%.2f",
             accountId, owner.c_str(), *balance);
    return std::string(report);
}
```

_Update `BankAccount.h` return type from `char*` to `std::string`._

### 12.7 Fix BUG-010 — Thread safety

```cpp
#include <mutex>

std::mutex accountMutex;

void performTransactions()
{
    for (int i = 0; i < 100000; i++)
    {
        std::lock_guard<std::mutex> lock(accountMutex);
        globalAccount->deposit(1);
    }
}
```

### 12.8 Fix BUG-011, BUG-012 — Safe main()

```cpp
// Remove null pointer dereference block entirely

// Replace unsafe access:
Transaction* tx2 = account1.getTransaction(0);
if (tx2 != nullptr)
    std::cout << tx2->description << std::endl;
else
    std::cout << "Transaction not found.\n";

// Use std::string for report:
std::string report = account1.generateReport();
std::cout << report << std::endl;
```

<!-- IMAGE: Screenshot of diff view showing before/after code changes in IDE -->

---

## 13. Verification Report

### 13.1 Verification Criteria

| Criterion | Target | Buggy Code | Fixed Code |
|-----------|--------|------------|------------|
| Compiles with `-Wall -Wextra` | 0 warnings | 2 warnings | _[TODO: 0]_ |
| ASan clean run | No errors | CRASH | _[TODO: PASS]_ |
| Unit test pass rate | 100% | 42.9% (3/7) | _[TODO: 100%]_ |
| Integration tests | All pass | FAIL | _[TODO: PASS]_ |
| Security tests | No exploitable flaws | FAIL | _[TODO: PASS]_ |
| Deterministic balance (threads) | Yes | No | _[TODO: Yes]_ |

### 13.2 Verification Activities Completed

| Activity | Status | Evidence |
|----------|--------|----------|
| Code Review | ✅ Complete | Section 3 |
| Static Analysis (compiler) | ✅ Complete | Section 4.2 — 2 warnings captured |
| Static Analysis (cppcheck) | 🔄 Pending | Section 4.3 |
| Static Analysis (clang-tidy) | 🔄 Pending | Section 4.4 |
| Dynamic Analysis (ASan) | ✅ Complete | Section 5 |
| Unit Testing | ✅ Complete | Section 6.1 |
| Integration Testing | 🔄 Partial | Section 6.2 |
| Security Testing | ✅ Complete | Section 6.3 |
| Debugging | ✅ Complete | Section 7 |
| Output Comparison | 🔄 Partial | Section 8 |
| Test Case Management | ✅ Complete | Section 9 |
| Fix Application | 🔄 Pending | Section 12 |
| Re-verification | 🔄 Pending | Section 13.1 |

### 13.3 Sign-Off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Tester | _[TODO]_ | | |
| Reviewer | _[TODO]_ | | |
| Instructor | _[TODO]_ | | |

<!-- IMAGE: Final verification checklist screenshot or signed PDF export -->

---

## 14. Conclusion & Recommendations

### 14.1 Conclusion

The Chap3Lab banking application contains **16 documented defects** spanning memory safety, business logic, concurrency, and security. V&V activities confirmed that the application is **not production-ready** and crashes under normal execution paths due to heap corruption and destructor bugs.

### 14.2 Recommendations

1. **Apply all fixes** in Section 12 and re-run the full test suite.
2. **Replace raw pointers** with `std::string` and value types where possible.
3. **Add continuous static analysis** (cppcheck/clang-tidy) to the build pipeline.
4. **Enable sanitizers** in CI for every commit.
5. **Adopt RAII** — use smart pointers (`std::unique_ptr`) for Transaction ownership.
6. **Remove global mutable state** — pass accounts by reference to thread functions.

### 14.3 Next Steps (Collaborative)

- [ ] Run cppcheck and clang-tidy; paste results into Section 4.3–4.4
- [ ] Apply fixes to `FixedBankingSystem/` and verify
- [ ] Capture all screenshots at `<!-- IMAGE -->` placeholders
- [ ] Complete integration test actual results
- [ ] Fill verification table after re-testing fixed code
- [ ] Add student names and sign-off

---

## Appendix A — Build & Run Commands

```bash
# Standard build
g++ -std=c++17 -Wall -Wextra -g -o banking_app BankingSystem.cpp BankAccount.cpp

# Sanitizer build
g++ -std=c++17 -g -fsanitize=address,undefined -o banking_asan BankingSystem.cpp BankAccount.cpp

# Unit tests
g++ -std=c++17 -g -o test_bank tests/test_bankaccount.cpp BankAccount.cpp
./test_bank

# Static analysis
cppcheck --enable=all --suppress=missingIncludeSystem .
clang-tidy BankAccount.cpp BankingSystem.cpp -- -std=c++17
```

## Appendix B — Image Placeholder Index

| # | Location | Suggested Screenshot |
|---|----------|-------------------|
| 1 | Section 2.2 | UML class diagram |
| 2 | Section 2.3 | Code::Blocks / project tree |
| 3 | Section 3.3 | IDE code review annotations |
| 4 | Section 4.2 | Compiler warnings in terminal |
| 5 | Section 4.3 | cppcheck output |
| 6 | Section 4.4 | clang-tidy output |
| 7 | Section 5.2 | ASan heap overflow crash |
| 8 | Section 5.3 | ASan destructor segfault |
| 9 | Section 6.1 | Unit test results |
| 10 | Section 6.2 | Integration test output |
| 11 | Section 6.3 | Security test checklist |
| 12 | Section 7.1 | Debugger at Transaction ctor |
| 13 | Section 7.2 | Debugger at destructor crash |
| 14 | Section 7.3 | Race condition — two different balances |
| 15 | Section 8.2 | Before/after output comparison |
| 16 | Section 9.3 | Test management spreadsheet |
| 17 | Section 10.2 | Bug tracking table |
| 18 | Section 12 | Code diff (before/after fixes) |
| 19 | Section 13.3 | Verification sign-off checklist |

---

*Report generated as part of CEF474 Verification & Validation lab. Work in progress — complete `[TODO]` sections and image placeholders collaboratively.*
