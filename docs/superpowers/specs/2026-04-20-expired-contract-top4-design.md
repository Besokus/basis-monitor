# Expired Contract Filtering for Top4 Selection

## Background

The current monitoring chain selects up to 4 contracts per product group from the previous trading day's futures EOD data. The selection logic lives in `src/data/contract_selector.cpp` and is used by both:

- the local reference subset preparation flow (`app/prepare_reference_subset.cpp` via `scripts/push_reference_data_to_zhongtai.sh`)
- the Zhongtai runtime startup flow (`app/main.cpp`)

Today, the business requirement is:

- expired contracts must be excluded before Top4 is computed
- a contract whose `maturity_date` is equal to the current trading date should still be kept
- after expiration filtering, select the Top4 remaining contracts by yesterday turnover

This is needed to keep uploaded monitoring contracts and runtime monitored contracts consistent.

## Goals

- Exclude already expired contracts before Top4 selection.
- Keep contracts whose `maturity_date == trading_date`.
- Preserve the existing Top4 ranking rule by `yesterday_turnover` with instrument ID tie-break.
- Apply the same rule to both local upload preparation and Zhongtai runtime selection.
- Keep the change surgical and easy to verify.

## Non-goals

- Do not change the report formatting flow.
- Do not change alert suppression for current-month contracts.
- Do not add a new fallback policy to force a full 4 contracts when fewer than 4 valid candidates exist.

## Proposed Design

### Rule

Use the current trading date as the reference day. For each candidate contract:

- if `maturity_date < trading_date`, skip it
- if `maturity_date == trading_date`, keep it
- if `maturity_date > trading_date`, keep it

### Placement

Add the expiry filter inside `SelectPerProductTop4()` in `src/data/contract_selector.cpp`, before the sort and resize steps.

This keeps the selection rule centralized and ensures both the local preparation flow and Zhongtai runtime use the same logic.

### Data Flow

1. Load reference data.
2. Filter out expired contracts using `maturity_date` and `trading_date`.
3. Group remaining contracts by report group.
4. Sort each group by:
   - `yesterday_turnover` descending
   - `instrument_id` ascending as a tie-breaker
5. Keep at most 4 contracts per group.

### Expected Behavior

- If IC2604 is expired, it is excluded before Top4 ranking.
- If an on-the-day-expiry contract exists, it remains eligible.
- If a group has fewer than 4 non-expired candidates, the result contains fewer than 4 contracts.

## Implementation Notes

- Prefer a small helper such as `IsExpiredContract(maturity_date, trading_date)`.
- Use the current trading date already available in `app/main.cpp` and pass it through to the selection function if needed.
- Keep the comparison format stable by relying on the `YYYY-MM-DD` string format already used in the reference data.

## Testing

Add or update tests to cover:

- expired contract is excluded before ranking
- contract with `maturity_date == trading_date` remains eligible
- Top4 still sorts by turnover after filtering
- groups with fewer than 4 valid candidates return the actual count without padding

## Acceptance Criteria

- Local reference subset upload no longer includes expired contracts.
- Zhongtai runtime selection no longer includes expired contracts.
- Current-day expiry contracts are still accepted.
- Upload-side and runtime-side selected contract sets stay consistent.

## Risks

- If the trading date source and reference data date diverge, the expiry filter could be applied against the wrong day. The implementation should use the same trading date concept already used by runtime startup.
- If a future business rule wants to treat `maturity_date == trading_date` as expired, only the comparison rule needs to change.

