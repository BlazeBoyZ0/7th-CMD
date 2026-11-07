# ADR 004 — Task Orchestrator

- **Status:** Accepted
- **Decision:** Use a dedicated orchestrator as the system-level coordination layer, not as a fifth engine.
- **Reason:** One component must own subsystem routing, task lifecycle, interaction, dispatch, result handling, pause/resume, and cancellation.
- **Consequences:** Subsystems return structured results and may report new requirements, but cannot directly dispatch other subsystems or control the overall task lifecycle.
