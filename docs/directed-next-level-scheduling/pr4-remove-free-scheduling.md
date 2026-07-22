# PR4 Plan: Remove Legacy Free Scheduling

## Objective

Delete the unconstrained NEXT_LEVEL scheduling implementation and leave one
minimal, internally consistent directed-dispatch design. Update all maintained
documentation and run the complete regression set for the changed subsystem.

PR1 through PR3 are prerequisites.

## Scope

- Rename affinity-oriented slot state to exact-target terminology.
- Delete NEXT_LEVEL idle-pool selection and unconstrained fallback helpers.
- Delete the old shared NEXT_LEVEL ready-queue path.
- Retain endpoint eligibility only as submit-time target validation.
- Keep the SUB shared queue and existing SUB free scheduling unchanged.
- Remove ReadyQueue methods and state that have no remaining callers.
- Update architecture and API documentation to match the final design.
- Add or adjust simulation integration coverage for directed single/group
  execution and dependency release.

## Explicit Non-Goals

- Do not add SUB worker selection.
- Do not change heap-buffer allocation or binding.
- Do not change TensorMap dependency semantics.
- Do not add a compatibility mode for unconstrained NEXT_LEVEL submit.
- Do not add advanced group/single resource arbitration.

## Planned Dead-Code Audit

Search production code, tests, examples, docs, and repository rules for:

- `affinity`, `affinities`, and `get_affinity` in hierarchical scheduling.
- `worker=-1`, `workers=None`, and empty-worker unconstrained semantics.
- `pick_idle`, `pick_n_idle`, `pick_idle_excluding`, and
  `pick_idle_excluding_eligible`.
- the old shared `ready_next_level_queue`.
- `ReadyQueue::wait_pop`, shutdown state, and condition variables.
- comments that describe Scheduler-selected NEXT_LEVEL workers.

Every match must be removed, renamed, or justified as a SUB-only behavior.

## Planned File Changes

- `src/common/hierarchical/types.h` and `types.cpp`
  - Rename target state and remove unused queue operations.
- `src/common/hierarchical/orchestrator.*`
  - Remove optional-target branches and legacy queue wiring.
- `src/common/hierarchical/scheduler.*`
  - Remove NEXT_LEVEL free-selection and legacy dispatch branches.
- `src/common/hierarchical/worker_manager.*`
  - Delete selection helpers with no SUB or control-path callers.
- `src/common/hierarchical/worker.*`
  - Delete ownership of the old NEXT_LEVEL queue.
- Python bindings and facade
  - Remove transitional terminology and unreachable compatibility handling.
- Tests and examples
  - Delete tests whose only purpose was unconstrained NEXT_LEVEL selection.
  - Keep or rewrite tests that still prove eligibility and exact placement.
- Maintained design documents
  - Describe exact placement, per-worker queues, the group queue, and the
    intentionally simple arbitration contract.

## Validation

- Run focused Python API and callable-identity tests.
- Build hierarchical C++ tests and run the relevant scheduler, orchestrator,
  worker-manager, remote-endpoint, and ring tests.
- Run the L3 dependency and group simulation scene tests.
- Run both a2a3 and a5 compile coverage for common hierarchical code where the
  local environment supports it.
- Run formatting, markdown lint, and repository pre-commit hooks.
- Use `rg` and compiler warnings to prove removed helpers have no callers.

## Size Budget

- Production code should have net deletions.
- New or rewritten tests: at most 350 added lines.
- Documentation: at most 250 added lines.
- Total target: at most 650 added lines.

## Completion Criteria

- No public or internal unconstrained NEXT_LEVEL submit path remains.
- No dead NEXT_LEVEL queue, fallback selector, field, helper, test, comment,
  or document remains.
- SUB behavior is unchanged and explicitly identifiable as separate.
- All relevant tests pass from a project-local environment.
- The final diff contains only code required by the documented contract.
