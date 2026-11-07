# Module Registry

This file is the quick index of planned modules. The architecture documents define their contracts. Placeholder files are intentionally empty until implementation begins.

## Core
- **types / identifiers:** shared primitive domain types and unique identifiers.
- **subsystem / subsystem_request / subsystem_result:** common subsystem contract and messages.
- **status / errors / constants:** shared statuses, error model, and stable constants.

## Orchestrator
- **orchestrator:** top-level coordination facade.
- **request_manager:** accepts and tracks user requests.
- **subsystem_router:** matches requirements to handlers.
- **workflow_controller:** coordinates the sequence of subsystem calls.
- **task_state_manager:** owns task state and valid transitions.
- **interaction_coordinator:** coordinates clarification and permission interactions.
- **subsystem_dispatcher:** invokes registered subsystem handlers.
- **result_handler:** validates and interprets subsystem results.
- **pause_resume_manager / cancellation_handler:** pause, resume, and cancel workflows.

## Engines
### Grammar
- **grammar_engine:** grammar pipeline facade.
- **input_processor:** accepts and prepares raw user input.
- **tokenizer:** splits input into tokens.
- **normalizer:** normalizes spelling, case, and supported variants.
- **grammar_parser:** parses supported grammar patterns.
- **ast_generator:** creates a syntax tree.
- **semantic_analyzer:** checks semantic structure and required arguments.
- **cir_generator:** emits Canonical Intent Representation.

### Reasoning
- **reasoning_engine:** reasoning facade and request boundary.
- **fact_resolver:** retrieves and resolves facts.
- **rule_evaluator:** evaluates validated declarative rules.
- **truth_evaluator:** implements TRUE/FALSE/UNKNOWN logic.
- **uncertainty_handler:** coordinates unresolved evidence.
- **contradiction_handler:** checks and resolves conflicting evidence.
- **confidence_evaluator:** computes deterministic evidence scores.
- **bounded_search:** enforces candidate, depth, and work budgets.
- **reasoning_trace:** records structured reasoning events.

### Planning
- **planning_engine:** planning facade.
- **goal_decomposer:** decomposes the requested goal.
- **task_breakdown:** creates actionable task units.
- **dependency_analyzer:** identifies prerequisites and dependencies.
- **operation_orderer:** orders operations.
- **plan_validator:** validates preconditions and constraints.
- **plan_optimizer:** removes unnecessary work without changing meaning or safety.
- **plan_state_manager:** tracks plan progress.

### Execution
- **execution_engine:** execution facade.
- **action_dispatcher:** routes approved actions to handlers.
- **operation_executor:** invokes a typed operation.
- **execution_tracker:** records operation outcomes and partial progress.

## Supporting systems
- **knowledge:** persistent knowledge interfaces, retrieval, updates, and task/session context.
- **information:** source providers, acquisition, parsing, normalization, typing, operations, and result construction.
- **entity_resolution:** candidate discovery, classification, matching, context resolution, and ambiguity handling.
- **capabilities:** definitions and registry of supported actions.
- **permissions:** authorization, scope checks, and confirmation.
- **recovery:** structured error and recovery handling.
- **verification:** postcondition verification.
- **response:** user-facing result generation and formatting.

## Platform
- **platform_interface:** common platform boundary.
- **filesystem / applications / processes / system interfaces:** portable contracts for OS providers.
- **linux:** first implementation target.
- **windows:** future implementation target; placeholders only.

## CLI and utilities
- **cli:** command-line input and output.
- **utilities:** JSON loading, logging, string helpers, and path helpers.

## Ownership rules
- Orchestrator owns task lifecycle.
- Grammar owns parsing into CIR.
- Reasoning owns logical evaluation, not planning.
- Planning owns task decomposition and ordering, not fact invention.
- Execution owns invoking approved handlers, not permission grants.
- Verification owns independent postcondition checks.
