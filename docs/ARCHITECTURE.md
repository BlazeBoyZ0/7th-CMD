# 7th cmd — Architecture

## 1. System purpose

7th cmd is a lightweight, local-first command intelligence system for operating a computer through natural-language instructions. It transforms a request into a structured intent, gathers the information needed to understand it, determines a valid plan, checks authorization, performs supported operations, verifies their effects, and communicates the outcome.

The system is designed to work without an LLM, SLM, hosted inference API, or required cloud service.

## 2. High-level structure

The architecture has four engines, a coordinating orchestrator, and supporting systems.

`User Input → Grammar Engine → Task Orchestrator → Reasoning / Planning → Execution Engine → Verification → Response`

Information retrieval, entity resolution, capability checks, and permission checks are invoked wherever the request requires them. The path is conditional: a request should activate only the components needed to handle it.

## 3. Core engines

### Grammar Engine

Interprets the user's wording and converts it into a structured Canonical Intent Representation (CIR). It handles tokenization, normalization, grammar parsing, semantic analysis, and intent construction.

The grammar is hybrid: a controlled base grammar can be extended through declarative vocabulary, synonym, spelling, and pattern data. Grammar identifies the requested intent; it does not decide which subsystem executes it or independently determine the identity of a real-world computer resource.

### Reasoning Engine

Evaluates facts and declarative rules to establish what is known, what follows logically, and what remains uncertain. It uses three-valued truth—TRUE, FALSE, and UNKNOWN—so missing evidence is not mistaken for falsehood.

It handles evidence conflicts, uncertainty, confidence assessment, and bounded search. Confidence is an evidence score, not a probability, and cannot grant permission or override capability restrictions. Reasoning does not perform operating-system actions.

### Planning Engine

Turns an established goal into an ordered set of operations. It decomposes goals, identifies dependencies, orders work, checks prerequisites and constraints, and tracks plan progress.

Planning uses established facts; it does not invent facts or replace the Reasoning Engine. If a prerequisite is unknown, dependent operations cannot proceed until that uncertainty is resolved.

### Execution Engine

Carries out approved, structured operations through registered handlers. It dispatches actions and tracks their results, but does not decide whether an action is permitted. It does not translate arbitrary natural-language input into unrestricted shell commands.

## 4. Task Orchestrator

The Task Orchestrator is the system-level coordination layer, not another reasoning or execution engine. It receives structured intent, determines which subsystems are required, dispatches work in dependency order, tracks the task lifecycle, and decides what happens after each subsystem result.

It coordinates clarification, permission requests, retries, recovery, pause/resume, cancellation, and completion. Subsystems return structured results to the orchestrator; they do not independently control the overall task.

Routing is capability- and requirement-driven. The orchestrator matches the needs expressed by the structured intent against available subsystem handlers, checks dependencies, avoids duplicate work, and can revise the route when new requirements appear.

## 5. Supporting systems

- **Knowledge & Context System:** provides persistent static knowledge and user preferences, plus temporary task and session context.
- **Information Processor:** acquires, parses, normalizes, types, and operates on information from registered sources.
- **Entity Resolution System:** identifies the actual application, file, folder, process, or other resource referred to by the user.
- **Capability Registry:** describes which operations the system supports and the requirements of each operation.
- **Permission Manager:** determines whether a supported operation is authorized in the current context.
- **Error & Recovery Manager:** coordinates structured failure handling and safe recovery.
- **Verification System:** checks whether an operation produced its intended observable result.
- **Response Generation System:** turns structured outcomes into clear user-facing responses.

## 6. Knowledge and information boundaries

Different kinds of information have different sources and lifetimes:

| Information | Source | Lifetime |
|---|---|---|
| Grammar and language data | Declarative configuration | Persistent |
| User preferences | User-managed configuration | Persistent |
| General static knowledge | Knowledge data | Persistent |
| Live OS information—apps, files, folders, processes, versions, settings, and resources | Operating-system providers | Temporary cache or active task |
| Current task facts and references | Task context | Current task |

Live operating-system information is never stored as long-term knowledge. It is queried from the operating system when needed and may be cached temporarily with freshness and invalidation handling.

The Information Processor provides structured data to other systems. The Knowledge & Context System manages the appropriate sources and context; neither should turn transient OS observations into permanent facts.

## 7. Entity, capability, and permission separation

These are distinct questions:

- **Entity Resolution:** Which actual resource does the user mean?
- **Capability Registry:** Does 7th cmd have a handler for the requested operation?
- **Permission Manager:** Is that operation allowed now?

Knowing that an application or file exists does not mean the system can modify it. Having an implementation for an operation does not mean it is authorized. These checks remain separate from reasoning confidence.

## 8. Platform architecture

Platform-specific access is isolated behind common interfaces for filesystem, applications, processes, and system information. Linux is the initial implementation target. Other operating systems can provide their own implementations without changing the higher-level engines and supporting systems.

## 9. Safety and execution boundaries

All computer actions pass through defined capabilities and permission checks. Operations validate their arguments and target paths. Read access and modification access are distinct. Destructive operations require appropriate confirmation. The system does not silently escalate privileges or bypass operating-system permissions.

After an operation, verification checks its observable postconditions. A response must distinguish success, partial completion, uncertainty, permission denial, and failure rather than claiming an action succeeded merely because execution was attempted.

## 10. Resource model

The design prioritizes low computational and memory overhead:
- C++20 and standard-library-first components.
- Declarative configuration for grammar and static rules.
- On-demand knowledge loading and targeted information retrieval.
- Bounded candidate search and reasoning work.
- Temporary caches for live OS information.
- No mandatory background service, cloud inference, or heavyweight runtime.
