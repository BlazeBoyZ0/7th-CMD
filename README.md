# 7th cmd

7th cmd is a lightweight, local-first command intelligence and computer-automation system. It interprets user instructions, resolves their targets, determines a valid course of action, checks authorization, performs supported operations, verifies outcomes, and reports results.

## Design principles

- Local-first operation; no required LLM, SLM, cloud inference, or network service.
- C++20 baseline, with a standard-library-first implementation.
- Linux as the initial platform, with platform abstractions for future portability.
- Explicit separation between understanding, reasoning, planning, authorization, execution, and verification.
- Bounded processing and retrieval.
- User preferences and static knowledge may persist; live operating-system information is retrieved from the OS and kept temporary.
- System actions are exposed through defined capabilities and permission checks, not unrestricted shell execution.

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the system architecture and subsystem relationships.

## Build configuration

The project uses CMake and requires C++20. Build targets will be added as the application implementation takes shape.
