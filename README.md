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

## Build

Requirements:

- CMake 3.16 or newer
- A C++20-compatible compiler
- Linux for the current platform backend

Configure and build from the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel 1
```

Run the CLI:

```bash
./build/7thcmd
```

Use `./build/7thcmd --help` for command-line options and `./build/7thcmd --version` to print the version.

The grammar configuration is loaded from `config/grammar` when running from the repository root. Installed builds also check `/usr/local/share/7thcmd/config/grammar` and `/usr/share/7thcmd/config/grammar`.

## Current command scope

Examples:

```text
Create a folder named Projects on Desktop
Create folder Projects on Desktop then create folder Games in Projects
Open browser
Open testfolder
Search files named report in Desktop
Rename file old.txt to new.txt
Move file report.txt to Documents
Copy file report.txt to Desktop
Delete file report.txt
Count words in file report.txt
Count lines in file report.txt
Count characters in file report.txt
```

Current boundaries:

- Filesystem modifications and reads are restricted to the user's home directory.
- Search examines one directory level and displays at most 50 matching entries.
- Copy currently supports files, not directory trees.
- Delete requires explicit confirmation and only removes files or empty directories.
- Text counting rejects files containing NUL bytes and reads at most 4 MiB.
- Multi-step execution currently supports dependent folder-creation sequences.
- The Linux backend is the only active platform backend. Windows support is not part of the current v0.1 command runtime.

7th cmd does not execute arbitrary shell commands from natural-language input.
