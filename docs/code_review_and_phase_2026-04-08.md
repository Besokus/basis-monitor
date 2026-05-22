# Basis Monitor Code Review And Phase Update (2026-04-08)

## Current Phase

The project has completed the first critical milestone for live CTP market data:

- formal Linux CTP market-data API is linked and running
- formal Linux data-collect library is linked and observable at runtime
- CTP live connect/login/subscribe/first-tick has been validated in the field
- local reference-data filtering and SFTP upload flow is already available
- dual-session architecture for `CTP + XTP` exists in code

The project is now in:

**Phase A complete, Phase B in progress**

- Phase A: CTP live market-data access and local reference-data sync
- Phase B: stabilize dual-session behavior and validate XTP data reception

The annualized-basis calculation is still based on:

- CTP real-time futures price
- previous-day index close from reference CSV

The system has **not** yet entered the next calculation phase:

- CTP future last price + XTP real-time index last price

## Code Review Findings

### Finding 1

`CTP + XTP` dual-session startup still treats XTP as a hard dependency, which conflicts with the current field-validation strategy.

- File: `src/market_data/market_data_session_factory.cpp`
- Risk:
  - if CTP is healthy but XTP login or subscribe fails, the whole process returns startup failure
  - this makes it hard to keep CTP production monitoring alive while XTP is still under validation
- Current behavior:
  - `DualMarketDataSession::Start()` returns `false` immediately when `xtp_session_->Start()` fails
  - this propagates to `main.cpp`, which exits on `!session->Start()`
- Recommendation:
  - split dual-session startup policy into:
    - `CTP required`
    - `XTP optional during validation`
  - keep CTP as the hard gate
  - downgrade XTP startup failure to warning during the current phase

### Finding 2

Market-data health tracking still only records futures ticks, so the health model is not yet compatible with the new dual-provider architecture.

- File: `app/main.cpp`
- Risk:
  - `Index` ticks return before `health_tracker_.RecordTick(...)`
  - when running XTP-only validation, the process can falsely report stale data even while XTP ticks are arriving
  - in later mixed-mode work, the health signal will still reflect only CTP freshness
- Recommendation:
  - split health tracking into independent CTP and XTP channels
  - for the current phase, at minimum log XTP freshness separately

### Finding 3

CTP startup still waits with `INFINITE` on connect/login/subscribe events, so the timeout failure messages in `MdApiSession` are not actually enforceable.

- File: `src/ctp/md_api_session.cpp`
- Risk:
  - if the front becomes unreachable or the callback chain stalls, the process can block indefinitely
  - operators will not get a bounded startup failure window
- Recommendation:
  - replace `INFINITE` waits with configurable startup timeouts
  - log stage-specific timeout values for connect, login, and subscribe

## What Is Working Now

The following path has already been proven by field logs:

1. load formal MdApi and LinuxDataCollect
2. connect to live CTP front
3. login successfully
4. subscribe monitored futures contracts
5. receive first market-data tick
6. run basis calculation and trigger alert logic

This means the current CTP objective can be marked as:

**Completed with follow-up hardening still required**

## Newly Completed In This Iteration

The runtime subscription model has now been tightened so that:

- CTP subscribes only the Top-4 monitored futures contracts selected from staging reference data
- XTP derives its runtime index subscription list from those same monitored contracts
- static `IndexInstrumentID` configuration is retained only as a fallback
- XTP subscription normalization now supports staged index codes such as `000300.XSHG` and `399905.XSHE`

This closes an important gap between the local reference-data filtering pipeline and the market-data runtime:

- the same staged dataset now drives monitored-contract selection
- the monitored-contract selection now drives both CTP futures subscriptions and XTP index subscriptions

## Development Flow From This Point

Recommended execution order:

1. keep CTP as the only production-critical path
2. harden startup policy so XTP failure does not take down CTP during validation
3. split health tracking for CTP and XTP
4. validate XTP reception and instrument mapping in the field
5. only after XTP is stable, switch calculation inputs to real-time XTP index prices

## Stage Exit Criteria

### Phase A Exit

Completed:

- CTP live connect/login/subscribe/first-tick verified
- basis monitoring continues to compute from CTP futures prices

### Phase B Exit

Required next:

- XTP can connect and continuously receive target index ticks
- dual-session startup no longer fails just because XTP is unstable
- CTP and XTP health are independently observable

### Phase C Entry

Only start after Phase B is stable:

- introduce real-time index cache
- switch annualized-basis calculation to `CTP future price + XTP index price`
- revalidate alerting and report outputs
