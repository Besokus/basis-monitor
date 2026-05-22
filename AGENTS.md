# AGENTS.md

**Always answer me in Chinese**
**XTP运行在中泰服务器中，只能上传编译好后的可执行文件和相关数据文件，不可以上传源代码**

## Purpose
This repository prioritizes correctness, stability, traceability, and minimal-risk delivery.
When working in this repo, always optimize for:
1. correctness
2. minimal diffs
3. maintainability
4. reproducibility
5. easy review and rollback

Do not optimize for cleverness, unnecessary abstraction, or broad refactoring unless explicitly requested.

---

## Knowledge Base Maintenance

The following project knowledge files are long-lived assets and should be kept current whenever major implementation, validation, deployment, or architecture decisions change:

- `architecture_decisions.md`
- `resume_highlights.md`
- `interview_stories.md`
- `metrics_baseline.md`

When a task materially changes system behavior, deployment boundaries, runtime validation status, or the project's strongest engineering stories:

1. update the relevant knowledge files
2. distinguish confirmed facts from pending evidence
3. avoid exaggeration
4. keep the files readable as standalone project context for future handoff, review, resume writing, and interview prep

---

## Global Working Rules

### 1. Plan before changing
For any non-trivial task, do not modify code immediately.
First provide:
- current behavior
- likely root cause
- proposed minimal solution
- affected files
- risks
- validation plan

Only start editing after the approach is clear.

### 2. Prefer minimal diffs
Make the smallest possible change that solves the problem.
Avoid unrelated cleanup, renaming, formatting churn, or architectural refactors unless explicitly requested.

### 3. One concern at a time
Do not combine multiple independent changes in one step.
Split work into small, reviewable units.

### 4. Preserve public behavior by default
Unless explicitly requested:
- do not change public APIs
- do not change request/response formats
- do not change database schema
- do not change config semantics
- do not change deployment behavior

### 5. Explain reasoning, not just code
After each meaningful change, summarize:
- what changed
- why it changed
- what alternatives were considered
- what risks remain
- how to verify it

### 6. Ask for confirmation on risky changes
Stop and ask before:
- deleting files
- renaming files or directories
- changing public interfaces
- changing database schema
- modifying CI/CD
- changing infra/config/deployment
- editing security-sensitive logic
- modifying auth, permissions, billing, payments, persistence, or production scripts

### 7. Keep style consistent
Follow existing project conventions for:
- naming
- file layout
- logging
- error handling
- test organization
- comments
- dependency usage

Do not introduce a new style unless explicitly requested.

### 8. Never fabricate understanding
If repository context is incomplete, state assumptions clearly.
Do not invent behavior, APIs, or file relationships.

---

## Development Process

For each task, follow this workflow:

### Step 1: Understand
Read the smallest necessary set of files and explain:
- entry point
- call chain
- key data structures
- control flow
- current constraints

### Step 2: Propose
Before editing, provide:
- root cause or goal understanding
- minimal change proposal
- files to edit
- possible side effects
- validation steps

### Step 3: Implement
Implement only the approved or clearly implied minimal scope.

### Step 4: Verify
Whenever possible:
- run the smallest relevant test first
- run lint/format only where needed
- avoid expensive full-project operations unless necessary
- report exact verification status

### Step 5: Review
After editing, provide a concise change summary and list:
- modified files
- key logic changes
- verification performed
- remaining uncertainties

---

## Code Change Boundaries

### Allowed by default
- small bug fixes
- targeted feature additions
- local refactors required for clarity
- tests directly related to the task
- focused logging improvements
- defensive error handling
- small documentation updates

### Not allowed by default
- broad refactors
- changing architecture
- introducing new frameworks/libraries
- changing package managers/toolchains
- changing build system
- changing persistence model
- changing external behavior
- changing runtime environment
- changing deployment files
- changing secrets/auth flows

---

## File Editing Rules

### Prefer precise edits
When editing:
- modify the smallest relevant function/class/module
- do not rewrite entire files unless necessary
- do not reorder code without reason
- do not touch unrelated imports/includes
- do not mass-format unrelated sections

### Comments
Add comments only when they improve maintainability.
Do not add obvious comments that restate the code.

### Logging
Prefer structured, actionable logs.
Do not add noisy logs in hot paths unless explicitly justified.

### Error handling
Prefer explicit, debuggable failure paths.
Do not swallow errors silently.

---

## Testing and Validation Rules

When making changes, validation priority is:

1. targeted unit or local test
2. module-level verification
3. integration verification if directly relevant
4. full build/test only if needed

When reporting validation, always distinguish:
- not run
- ran and passed
- ran and failed
- unable to run

Never claim code is verified if tests were not executed.

---

## Performance and Reliability Rules

This repository values stable engineering over speculative optimization.

### Performance
Do not introduce complexity for hypothetical performance gains.
For performance-related changes:
- identify the bottleneck first
- explain why the change helps
- mention tradeoffs
- avoid premature optimization

### Concurrency / async / low-latency code
When touching concurrent or latency-sensitive code:
- preserve thread-safety assumptions
- identify shared state explicitly
- mention lock/order/ownership implications
- avoid changing timing-sensitive behavior without explanation

### Reliability
Prefer predictable behavior over hidden magic.
Add safeguards where useful:
- input validation
- timeout handling
- null/empty checks
- fallback paths
- clear error propagation

---

## Dependency Rules

Do not add new dependencies unless explicitly justified.
If a new dependency is necessary, explain:
- why existing code cannot solve it reasonably
- package size / maintenance impact
- security / operational implications
- alternatives considered

Prefer standard library and existing repo dependencies.

---

## Git and Review Expectations

Keep changes easy to review.
A good change should have:
- clear intent
- limited scope
- minimal unrelated edits
- easy rollback path

If a task grows in scope, stop and explicitly say so before continuing.

---

## Response Format for Non-Trivial Tasks

For non-trivial tasks, respond in this structure before editing:

### Understanding
- current behavior
- relevant files
- likely root cause / implementation target

### Proposed change
- minimal plan
- files to modify
- expected impact
- risks

### Validation
- tests/checks to run

After editing, respond in this structure:

### What changed
- concise summary

### Why
- reasoning

### Validation status
- exact checks run and results

### Remaining risks
- open questions or edge cases

---

## Repo Safety Rules

Treat these areas as high-risk and require confirmation before changes:
- database migrations
- production config
- CI/CD pipelines
- deployment scripts
- auth/security logic
- billing/payment logic
- destructive scripts
- secrets/credentials handling

---

## Preferred Behavior by Task Type

### Bug fix
Prefer root-cause fix over superficial patch.
Do not broaden scope unless needed to prevent recurrence.

### New feature
Implement the smallest vertical slice that satisfies the requirement.
Do not over-generalize.

### Refactor
Only refactor when necessary for the requested task or correctness.
Keep behavior unchanged unless explicitly requested.

### Debugging
First identify:
- expected behavior
- actual behavior
- likely failure point
- evidence needed
Do not guess blindly.

---

## If Context Is Missing
If the repository context is insufficient:
- state assumptions explicitly
- avoid irreversible edits
- prefer analysis first
- ask for confirmation only when risk is material

---

## Default Operating Mode
Default to:
- analysis first
- minimal changes
- high explainability
- high reviewability
- low-risk implementation

The goal is to act like a careful senior engineer working in an unfamiliar but important production codebase.
