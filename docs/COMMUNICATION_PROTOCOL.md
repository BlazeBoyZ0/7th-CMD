# Subsystem Communication Protocol

## Subsystem Request

Every subsystem invocation uses a structured request:

| Field | Meaning |
|---|---|
| `request_id` | Unique identifier for this subsystem call |
| `task_id` | Identifier of the owning user task |
| `operation` | Requested subsystem operation |
| `input_data` | Typed operation-specific input |
| `context` | Relevant task/session context |
| `constraints` | Applicable limits and requirements |
| `metadata` | Version, source, tracing, and routing metadata |

## Subsystem Result

| Field | Meaning |
|---|---|
| `request_id` | Correlates result with the request |
| `status` | One of the result statuses below |
| `result_data` | Structured output |
| `new_requirements` | Additional work discovered during processing |
| `missing_information` | Information needed to proceed |
| `error` | Structured error details, if any |
| `metadata` | Provenance, timing, trace and diagnostic data |

## Status values

- `SUCCESS` — requested subsystem operation completed.
- `NEEDS_INFORMATION` — additional information is required.
- `NEEDS_PERMISSION` — an authorization decision is required.
- `NEW_REQUIREMENTS` — processing revealed additional requirements.
- `UNCERTAIN` — available evidence does not support a reliable conclusion.
- `FAILURE` — the subsystem operation failed.

`UNCERTAIN` is not equivalent to `FAILURE`. A subsystem may successfully evaluate the available evidence and correctly return uncertainty.

## Rules

1. Subsystems never directly control the overall task lifecycle.
2. Results always return to the orchestrator.
3. The orchestrator decides whether to continue, clarify, request permission, recover, pause, fail, cancel, or complete.
4. A subsystem may report new requirements but cannot independently dispatch another subsystem.
5. Requests and results are validated at subsystem boundaries.
6. No subsystem may bypass capability registration, permissions, or execution constraints.
7. Every request is associated with a task ID; every task has a unique ID.
8. Results must preserve useful provenance and freshness metadata.

## Initial implementation

Use in-process C++ structures: `struct` for requests/results, `enum class` for statuses, and standard containers for requirements and missing information. JSON is used for configuration and persistent declarative knowledge, not as a runtime message broker. No network transport or message queue is required.
