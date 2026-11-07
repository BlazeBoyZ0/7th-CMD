# 7th cmd

**7th cmd** is a lightweight, local-first command intelligence and computer-automation system. It interprets structured natural-language commands, resolves their targets, plans operations, executes only registered and permitted capabilities, verifies outcomes, and reports results.

## Project principles

- No LLM, SLM, cloud inference, or required network service.
- C++20 baseline; standard library first.
- Linux first, with a platform abstraction intended to support Windows later.
- Explicit capabilities, permission checks, bounded reasoning, and verifiable execution.
- User preferences and approved static knowledge may persist. Live operating-system facts must be queried from the OS and kept only in temporary caches or active task context.
- No unrestricted natural-language-to-shell execution.
- Architecture and implementation status are documented in `docs/`.

## Current status

**Architecture and repository scaffold stage.** Source and header files are intentionally empty placeholders. The first implementation milestone is a small end-to-end vertical slice, not simultaneous implementation of every subsystem.

## Build baseline

- C++20
- CMake 3.16 or newer
- Linux Mint / Linux first

The initial CMake project records the language and project configuration. Build targets will be added with the first implementation slice.

## Documentation

Start with:

1. [Architecture](docs/ARCHITECTURE.md)
2. [System design](docs/SYSTEM_DESIGN.md)
3. [Module registry](docs/MODULE_REGISTRY.md)
4. [Communication protocol](docs/COMMUNICATION_PROTOCOL.md)
5. [Task lifecycle](docs/TASK_LIFECYCLE.md)
6. [Security and permissions](docs/SECURITY_AND_PERMISSIONS.md)
7. [Roadmap](docs/DEVELOPMENT_ROADMAP.md)
8. [Implementation status](docs/IMPLEMENTATION_STATUS.md)

## Repository status

The directory structure is a planned architecture map, not a claim that every listed subsystem is implemented. Empty C++ files are placeholders until their contracts and tests are implemented.
