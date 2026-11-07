# Architecture

## Purpose

7th cmd is a local, bounded command-intelligence system. It converts a user's request into a structured intent, resolves required entities and information, evaluates relevant facts and rules, plans operations, checks capabilities and permissions, executes approved actions, verifies outcomes, and communicates the result.

It is not a general-purpose conversational model. It does not use an LLM, SLM, hosted inference API, or required cloud service.

## Architectural layers

1. **Input and grammar** — normalize text and convert it into a Canonical Intent Representation (CIR).
2. **Task orchestration** — coordinate subsystem calls and own the task lifecycle.
3. **Reasoning and planning** — evaluate facts/rules and produce a validated operation plan.
4. **Information and knowledge** — retrieve facts from the correct source and provide typed information operations.
5. **Entity resolution** — identify the actual app, file, folder, process, or other target.
6. **Capabilities and permissions** — describe available actions and decide whether they are currently authorized.
7. **Execution and platform adapters** — perform narrowly defined operations through OS-specific interfaces.
8. **Verification and response** — check results and produce a truthful user-facing report.

## Four engines

- **Grammar Engine:** produces structured intent. It does not select subsystems or execute actions.
- **Reasoning Engine:** evaluates facts, rules, truth, uncertainty, contradictions, and bounded search. It does not plan or perform OS actions.
- **Planning Engine:** decomposes goals, builds dependencies, orders operations, validates and tracks plans. It does not invent facts.
- **Execution Engine:** dispatches approved operations and tracks their outcomes. It does not independently grant permissions.

## Supporting systems

Knowledge & Context, Information Processor, Entity Resolution, Capability Registry, Permission Manager, Error & Recovery, Verification, and Response Generation support the engines and orchestrator.

The **Task Orchestrator** is the coordination layer, not a fifth engine. It chooses which subsystem to invoke based on structured requirements, available handlers, and dependencies. It does not perform grammar parsing, reasoning, planning, or OS execution itself.

## Core boundaries

- Grammar describes what the user asked for; orchestration decides who handles each requirement.
- Knowledge represents what is known; the capability registry represents what the program can do; the permission manager represents what is allowed now.
- Reasoning establishes logical conclusions; planning creates an operation sequence.
- Execution only receives validated, authorized operations.
- Verification independently checks whether the intended result occurred.
- Subsystems return structured results to the orchestrator. They do not directly control the overall task lifecycle.

## Data lifetime

| Data | Source | Lifetime |
|---|---|---|
| Grammar vocabulary and patterns | Versioned JSON | Persistent |
| User preferences explicitly supplied/approved | User-managed JSON | Persistent |
| General static domain knowledge | Modular JSON | Persistent |
| Installed apps, files, processes, OS settings and resource state | Live OS providers | Temporary cache / active task only |
| Current task facts and references | Task context | Current task |
| Diagnostic traces | RAM by default; optional local logs | User-configured |

Live OS inventory must never be written into persistent knowledge. Cached OS facts require freshness metadata and invalidation.

## Routing model

1. Grammar emits a CIR and explicit requirements.
2. The orchestrator extracts requirements and matches them against registered subsystem handlers.
3. Dependencies are resolved and duplicate work is removed.
4. Required subsystems are dispatched in a valid order.
5. Results may introduce new requirements, missing information, permission requests, uncertainty, or failure.
6. The orchestrator updates the workflow and decides whether to continue, clarify, request permission, recover, complete, or stop.

Routing begins with deterministic requirement-to-handler matching using ordinary C++ data structures and declarative subsystem profiles.

## Initial implementation strategy

Build one end-to-end vertical slice before implementing every subsystem deeply:

`Create a folder named Projects on Desktop`

The first slice should cover input, grammar, orchestration, target resolution, plan validation, permission handling, execution, verification, and response. Expand capabilities only after this path is testable.
