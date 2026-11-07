# Development Roadmap

This roadmap is staged. The existence of placeholder files does not mean a stage is implemented.

## Stage 0 — Architecture and scaffold

- [x] Agree on the high-level architecture and subsystem boundaries.
- [x] Record communication and task lifecycle contracts.
- [x] Establish C++20 and lightweight project constraints.
- [ ] Review scaffold and module registry.
- [ ] Add the first buildable executable and test harness.

## Stage 1 — First vertical slice

Target command:

`Create a folder named Projects on Desktop`

Implement the smallest connected path:
1. CLI input.
2. Minimal grammar and CIR.
3. Orchestrator request and task state.
4. Desktop/folder entity resolution.
5. Basic plan and dependency validation.
6. Capability and permission check.
7. Linux folder creation handler.
8. Verification.
9. Clear response generation.

The vertical slice should be end-to-end, testable, and safe before adding broad language coverage.

## Stage 2 — Basic filesystem capabilities

- Create, rename, move, and copy files/folders.
- Search files and folders within approved locations.
- Clarify ambiguous targets.
- Confirm destructive operations.
- Verify results and report partial completion.

## Stage 3 — Application and information capabilities

- Discover and open installed applications.
- Resolve default applications when appropriate.
- Read basic text-file information.
- Count words, characters, and lines.
- Add reusable typed information operations.

## Stage 4 — Multi-step workflows and reliability

- Dependency-aware multi-operation plans.
- Pause/resume and cancellation.
- Structured recovery.
- Freshness-aware OS information caches.
- Expanded reasoning rules and bounded search.
- Diagnostic trace support, opt-in.

## Stage 5 — Expansion

- Additional grammar patterns and user-editable vocabulary.
- More platform providers.
- Windows implementation behind the platform interfaces.
- Additional capabilities based on demonstrated need.

## Explicitly out of initial scope

- LLM/SLM or cloud inference.
- Unrestricted shell execution.
- Unrestricted natural-language coverage.
- Autonomous permanent learning.
- Windows implementation in the first Linux prototype.
- Complex autonomous multi-application workflows.
