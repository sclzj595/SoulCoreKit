# Architecture Decision Records

This directory contains Architecture Decision Records (ADRs) for the SoulCoreKit project. Each ADR documents a significant architectural decision, its rationale, and its consequences.

## Index

| Number | Title | Status | Date |
|--------|-------|--------|------|
| [ADR-001](001-error-handling-boundary.md) | Error Handling Boundary Rules | Accepted | 2026-07-25 |
| [ADR-002](002-module-dependency-rules.md) | Module Dependency Rules | Accepted | 2026-07-25 |
| [ADR-003](003-memory-management.md) | Memory Management Policy | Accepted | 2026-07-25 |
| [ADR-004](004-or-multi-database.md) | ORM Multi-Database Architecture | Accepted | 2026-07-25 |
| [ADR-005](005-thread-safety-policy.md) | Thread Safety Policy | Accepted | 2026-07-25 |

## Purpose

ADRs are lightweight documents that record important architectural decisions. They serve as:

- **Decision history**: A chronological log of significant technical decisions
- **Context preservation**: Future maintainers understand *why* the architecture is the way it is
- **Change catalyst**: When a decision is challenged, the ADR provides a starting point for discussion
- **Learning aid**: New team members can rapidly understand the architectural boundaries

## Format

Each ADR follows this structure:

- **Title**: A clear, descriptive name for the decision
- **Status**: Accepted, Deprecated, Superseded, or Proposed
- **Context**: The forces at play that led to the decision
- **Decision**: The full description of the architectural decision
- **Consequences**: The impact — both positive and negative — of the decision
- **Examples**: Concrete code examples demonstrating the pattern
- **Validation**: How compliance with this decision is enforced or verified

## Maintenance

When a new architectural decision is made, create a new file with the next sequential number and update this index. Decisions should be reviewed during code reviews and updated when they are superseded by newer decisions.

## Related Documentation

- [Architecture Overview](../01_architecture.md)
- [Design Principles](../02_design_principles.md)
- [Module Specification](../03_module_specification.md)
- [API Design](../06_api_design.md)
- [Threading](../07_threading.md)
- [Error Handling](../08_error_handling.md)
- [Memory Management](../09_memory_management.md)