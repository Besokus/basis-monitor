# Local Trading-Time Error (CN 08:30-15:30)

## Summary
Add a local pre-check for order submission: if the current time in China (UTC+8) is outside the trading window 08:30-15:30, the system will reject the order locally and print a deterministic error code/message to the terminal. This satisfies the requirement to return a clear error code even when CTP does not respond during non-trading periods.

## Goals
- Always emit a local error code when users attempt to submit orders outside 08:30-15:30 (China timezone).
- Do not rely on CTP responses for this specific case.
- Keep behavior unchanged during trading time.

## Non-Goals
- No holiday/weekend calendar logic.
- No night session handling.
- No per-exchange or per-instrument session rules.

## Requirements
- Use China timezone (UTC+8) regardless of system locale.
- Trading window is inclusive: 08:30:00 through 15:30:00.
- When outside the window, do not call `ReqOrderInsert` and instead log a local error.
- Error delivery mechanism is terminal logging only (no CTP callbacks, no API return code changes).
- Use a stable, documented local error code and message printed to terminal.
- Log format should match existing `[API_ERROR]` style used in the demo.
- Minimal changes; keep existing behavior when within the window.

## Error Code & Message
- ErrorID: `90001`
- ErrorCode: `LOCAL_NOT_TRADING_TIME`
- Prompt: `LOCAL: Not in trading time (CN 08:30-15:30)`

## Data Flow
1. User triggers normal order flow (e.g., `ReqOrderInsert_Ordinary`).
2. `ReqOrderInsert_Ordinary_Checked` runs the new local time check.
3. If outside window: log local error (same format as existing `[API_ERROR]`) and return without sending to CTP.
4. If inside window: continue with existing behavior (send order).

## Implementation Notes
- Implement a helper that returns China local time (UTC+8) using `std::chrono::system_clock` and `gmtime_r/gmtime_s` + 8-hour offset.
- Add a simple time-window predicate (08:30-15:30 inclusive).
- Place the check at the start of `ReqOrderInsert_Ordinary_Checked` to avoid sending orders outside trading time.
- Reuse existing error logging helper to keep output consistent (`LogTraderErrorDetail` or unified `[API_ERROR]` formatting).

## Test Plan
- Run during actual trading hours or set system clock to 09:00 China time: order should proceed (no local error).
- Set system clock to 20:00 China time: order should be rejected locally with ErrorID `90001` and `LOCAL_NOT_TRADING_TIME`.
- Verify no call to `ReqOrderInsert` occurs when outside window (by absence of `<ReqOrderInsert>` in log).

## Risks
- Since holiday/weekend logic is out of scope, orders could still be rejected by exchange even during the time window.
- If system time is incorrect, local check will be wrong (acceptable for this simple rule).
