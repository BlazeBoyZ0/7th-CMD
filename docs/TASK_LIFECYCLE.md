# Task Lifecycle

## States

- `NEW`
- `UNDERSTANDING`
- `ROUTING`
- `PROCESSING`
- `WAITING_FOR_USER`
- `WAITING_FOR_PERMISSION`
- `RECOVERY`
- `COMPLETED`
- `FAILED`
- `CANCELLED`

## Typical flow

`NEW → UNDERSTANDING → ROUTING → PROCESSING`

From PROCESSING, the orchestrator may:
- Continue with another required subsystem.
- Return to ROUTING when new requirements emerge.
- Enter WAITING_FOR_USER for clarification.
- Enter WAITING_FOR_PERMISSION for authorization.
- Enter RECOVERY after a recoverable failure.
- Enter COMPLETED after successful verification.
- Enter FAILED when the task cannot safely or meaningfully continue.
- Enter CANCELLED when the user cancels.

Waiting states resume through the orchestrator after the required input or authorization is received. Recovery must not blindly repeat already completed operations.

## Lifecycle invariants

- Every task has a unique task ID.
- Every subsystem request has a request ID and owning task ID.
- State changes pass through a transition controller and are validated.
- Subsystems report outcomes; they do not mutate global task state directly.
- Clarification pauses the task, not the entire application.
- Completed operations are tracked to prevent accidental duplicate execution.
- Cancellation is checked at safe operation boundaries.
- Partial completion is represented explicitly.
- A task is not COMPLETED until required postconditions have been verified.

## Task state versus plan state

Task state describes the overall user request. Plan state describes the progress of the operation sequence. Keep these related but distinct so that plan replanning does not corrupt the task lifecycle.
