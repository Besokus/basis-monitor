# Local Trading-Time Error Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reject order submissions outside CN 08:30?15:30 and print a deterministic local error code in the terminal.

**Architecture:** Add a small time-check helper (China timezone) and call it at the start of `ReqOrderInsert_Ordinary_Checked`. If outside the window, log a local error in the existing `[API_ERROR]` style and return without sending to CTP.

**Tech Stack:** C++17, existing demo logging and CTP adapter.

---

## File Structure
- Modify: `6.7.11apidemo/6.6.5_demo/main.h` (order insert pre-check and logging)
- Modify: `6.7.11apidemo/6.6.5_demo/linux_compat.h` (no changes needed for this feature)
- Tests: none (project has no test harness)

### Task 1: Add China Time Helper and Trading Window Check

**Files:**
- Modify: `6.7.11apidemo/6.6.5_demo/main.h`

- [ ] **Step 1: Add helper to get China local time**

```cpp
static bool GetChinaLocalTime(int& hour, int& minute)
{
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm gmt = {};
#ifdef _WIN32
    gmtime_s(&gmt, &tt);
#else
    gmtime_r(&tt, &gmt);
#endif
    // UTC+8
    int h = gmt.tm_hour + 8;
    int day_adjust = 0;
    if (h >= 24) { h -= 24; day_adjust = 1; }
    if (h < 0) { h += 24; day_adjust = -1; }
    hour = h;
    minute = gmt.tm_min;
    return true;
}
```

- [ ] **Step 2: Add trading window predicate**

```cpp
static bool IsWithinChinaTradingWindow()
{
    int h = 0, m = 0;
    if (!GetChinaLocalTime(h, m)) return true; // fail-open
    int minutes = h * 60 + m;
    return minutes >= (8 * 60 + 30) && minutes <= (15 * 60 + 30);
}
```

### Task 2: Emit Local Error and Stop Order Submission

**Files:**
- Modify: `6.7.11apidemo/6.6.5_demo/main.h`

- [ ] **Step 1: Add local error constants**

```cpp
const int kLocalNotTradingTimeId = 90001;
const char* kLocalNotTradingTimeCode = "LOCAL_NOT_TRADING_TIME";
const char* kLocalNotTradingTimePrompt = "LOCAL: Not in trading time (CN 08:30-15:30)";
```

- [ ] **Step 2: Add log helper for local errors**

```cpp
static void LogLocalError(const char* callback, int code, const char* id, const char* prompt)
{
    LOG("[API_ERROR] callback=[%s], code=[%d], id=[%s], prompt=[%s] (local)\n", callback, code, id, prompt);
}
```

- [ ] **Step 3: Insert guard at top of ReqOrderInsert_Ordinary_Checked**

```cpp
if (!IsWithinChinaTradingWindow()) {
    LogLocalError("ReqOrderInsert_Ordinary", kLocalNotTradingTimeId, kLocalNotTradingTimeCode, kLocalNotTradingTimePrompt);
    return;
}
```

### Task 3: Manual Verification

**Files:**
- None

- [ ] **Step 1: Build**

Run: `make -j`
Expected: build completes.

- [ ] **Step 2: Runtime check (outside 08:30?15:30 China time)**

Expected terminal output (before any CTP call):
`[API_ERROR] callback=[ReqOrderInsert_Ordinary], code=[90001], id=[LOCAL_NOT_TRADING_TIME], prompt=[LOCAL: Not in trading time (CN 08:30-15:30)] (local)`

- [ ] **Step 3: Runtime check (inside 08:30?15:30 China time)**

Expected: no local error; normal CTP order flow continues.
