# ADR 001 — Core Architecture

- **Status:** Accepted
- **Decision:** Use a lightweight local-first architecture with four engines—Grammar, Reasoning, Planning, and Execution—coordinated by a Task Orchestrator and supported by dedicated systems.
- **Reason:** Clear separation of concerns, bounded resource use, testability, and explicit control over OS actions.
- **Consequences:** Subsystems communicate through structured requests/results. The orchestrator owns task lifecycle. No LLM or cloud inference is required.
