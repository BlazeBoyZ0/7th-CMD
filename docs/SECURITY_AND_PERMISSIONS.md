# Security and Permissions

## Security posture

7th cmd is a local automation tool and must treat OS access as a privileged boundary. Capability availability does not imply permission, and a successful logical conclusion does not authorize execution.

## Capability registry

Each capability definition should describe:
- Stable capability ID and human-readable name.
- Supported operation and argument schema.
- Required information and preconditions.
- Platform availability.
- Risk classification and whether confirmation is required.
- Verification strategy.
- Enabled/disabled state.

The registry describes what the application can do. It does not grant authorization.

## Permission manager

The permission manager determines whether an operation is currently allowed. It checks operation scope, user authorization, confirmation requirements, and applicable constraints.

Keep read access distinct from modification access. User authorization must be explicit and scoped to the requested action. Do not infer broad permission from a previous unrelated approval.

## Execution safeguards

- Never execute arbitrary natural-language text as a shell command.
- Use dedicated, typed operation handlers.
- Validate and normalize paths; guard against traversal and unintended locations.
- Keep filesystem search boundaries configurable.
- Require confirmation for destructive or high-impact changes.
- Do not request root privileges by default or silently escalate privileges.
- Do not bypass OS permissions.
- Report partial completion accurately.
- Verify observable postconditions after changes.
- Preserve enough operation metadata to support safe recovery without repeating completed work.

## Live OS information

Installed applications, files, folders, processes, versions, resource state, defaults, and system configuration are live OS facts. Retrieve them from platform providers when needed. They may be held in temporary caches or task context with freshness and invalidation handling, but must never be written into persistent knowledge files.

## Privacy and diagnostics

No data collection or cloud telemetry is required. Reasoning traces remain in memory by default. If local diagnostic logging is later added, it must be opt-in, local, configurable, and sanitized to avoid unnecessary exposure of sensitive paths or file contents.

## Failure behavior

When a permission check fails, stop the affected operation and explain the boundary. Do not retry by switching to an unapproved mechanism. When a safety-critical prerequisite is unknown, do not execute the dependent operation.
