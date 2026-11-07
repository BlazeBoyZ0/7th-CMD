# System Design

## Runtime flow

`User Input → Grammar → Orchestrator → Information / Entity Resolution → Reasoning → Planning → Capability & Permission Checks → Execution → Verification → Response`

The exact path is requirement-driven. A simple request must not activate every subsystem. The orchestrator should retrieve only the information needed for the current task and reuse task-local facts only while they remain valid.

## Grammar Engine

The grammar engine uses a hybrid grammar approach: a controlled base grammar that can expand through declarative data files.

Main stages:
- Input processing and tokenization.
- Text normalization and spelling/synonym handling.
- Grammar parsing and AST construction.
- Semantic analysis.
- CIR generation.

Grammar data is declarative JSON. Spelling correction can propose alternatives, but it must not decide entity identity. For example, whether a phrase refers to an application, website, file, or folder is resolved by Entity Resolution using context and OS evidence.

## Knowledge & Context

Storage is distributed by data type:
- Preferences: user-managed JSON.
- Static concepts, relationships, and rules: modular JSON.
- Grammar: vocabulary, synonyms, spelling data, and patterns in JSON.
- Live OS facts: queried from OS providers and retained only temporarily.
- Task context: in-memory task-scoped structures.

Use a common logical fact representation with source, freshness/timestamp, and status metadata while allowing each source to retain its appropriate storage. Retrieval is read-only by default. Persistent changes require explicit user approval. User corrections can update the current task immediately without silently becoming permanent knowledge.

Context supports session/task scope, entity references, pronouns and demonstratives, parent-child relations, and clarification when a missing argument blocks safe progress.

## Information Processor

The Information Processor acquires, parses, normalizes, types, and operates on information. It supports reasoning and response generation but is not itself the reasoning engine.

Initial provider families:
- System information.
- Applications.
- Filesystem and documents.
- Processes.

Reusable operation families:
- Collections: count, filter, sort, find, group.
- Numbers: add, subtract, compare, average, minimum, maximum.
- Text: tokenize, word/character/line count, search, extract.
- Records: get property, select fields, compare records.

Use specialized providers and parsers rather than pretending every data source has one universal format.

## Entity Resolution

Entity Resolution identifies the real target of a reference. It discovers candidates, classifies candidate types, matches names and context, checks ambiguity, and uses platform sources where appropriate.

Linux-first sources may include desktop application metadata, XDG configuration, filesystem metadata, and process information. Platform-specific discovery belongs behind interfaces. A phrase such as `open browser` may resolve to the configured default browser when no specifically named app is requested; a phrase such as `open testfolder` should search for a folder candidate rather than assume it is an application.

## Reasoning Engine

Reasoning uses:
- Unified logical facts with source and freshness metadata.
- Declarative, validated JSON rules evaluated in C++.
- Three-valued truth: TRUE, FALSE, UNKNOWN.
- Kleene-style three-valued logical operations.
- Explicit uncertainty and contradiction handling.
- Deterministic evidence-based confidence scores (not probabilities).
- Bounded search budgets and cycle detection.
- A structured reasoning trace.

UNKNOWN must never silently become FALSE. Confidence does not override truth status, permissions, capabilities, or safety constraints. Reasoning has no OS-action side effects; it may request information through the Information Processor or knowledge retrieval interfaces.

## Planning Engine

Planning decomposes a goal into tasks, identifies dependencies, orders operations, validates preconditions and constraints, and manages plan state. It may ask Reasoning to evaluate a logical question but does not independently establish facts.

An unresolved prerequisite blocks dependent operations, while independent safe subtasks may remain eligible.

## Task Orchestrator

The orchestrator owns request tracking, subsystem routing, workflow control, task state, interaction/clarification coordination, dispatch, result handling, pause/resume, and cancellation.

It uses explicit state transitions rather than a large nested conditional. Subsystems return results; only the orchestrator decides the next overall workflow step.

## Execution and platform

The execution engine dispatches structured operations to registered capability handlers. Platform interfaces isolate Linux-specific implementation from future Windows implementation. No arbitrary natural-language-to-shell conversion is allowed. Each operation validates arguments and paths and returns a structured result.

## Verification and response

Verification checks observable postconditions independently of the executor's claim. Response generation formats verified results, partial completion, unresolved information, permission requirements, and errors without overstating success.

## Resource strategy

- C++20, standard library first.
- Avoid heavyweight frameworks and mandatory background services.
- Load knowledge/configuration on demand.
- Use bounded candidate sets and retrieval budgets.
- Cache only temporary OS information with freshness/invalidation.
- Add concurrency only when a measured requirement justifies it.
