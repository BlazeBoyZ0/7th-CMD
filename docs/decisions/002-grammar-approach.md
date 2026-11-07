# ADR 002 — Grammar Approach

- **Status:** Accepted
- **Decision:** Use a hybrid grammar engine: a controlled base grammar that can be expanded through declarative JSON vocabulary, synonyms, spelling knowledge, and patterns.
- **Reason:** Predictable behavior and low computational cost while allowing gradual coverage growth.
- **Consequences:** Grammar emits CIR and requirements; it does not decide subsystem routing or final entity identity. Entity Resolution handles actual targets.
