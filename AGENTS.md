# Repository working rules

- Treat the documentation in `docs/` as the architecture source of truth.
- C++20 is the minimum required language standard.
- Keep the first implementation lightweight and standard-library-first.
- Do not add an LLM, SLM, cloud inference, or network dependency.
- Do not persist live operating-system inventory or transient OS facts.
- Route all system actions through registered capabilities and permission checks.
- Never translate arbitrary natural-language text directly into unrestricted shell execution.
- Validate paths and arguments; require confirmation for destructive operations.
- Keep subsystem boundaries and request/result contracts explicit.
- Implement one tested vertical slice at a time; update documentation when architecture changes.
- Do not claim a module is implemented merely because its placeholder files exist.
