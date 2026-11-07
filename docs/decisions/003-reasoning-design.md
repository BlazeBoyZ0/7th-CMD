# ADR 003 — Reasoning Design

- **Status:** Accepted
- **Decision:** Use structured facts, declarative JSON rules evaluated in C++, three-valued truth, explicit uncertainty/contradiction handling, deterministic evidence-based confidence, bounded search, and structured traces.
- **Reason:** Predictability, auditability, and low resource requirements without a language model.
- **Consequences:** UNKNOWN remains distinct from FALSE and FAILURE. Search exhaustion without a conclusion returns UNKNOWN. Reasoning does not execute OS actions.
