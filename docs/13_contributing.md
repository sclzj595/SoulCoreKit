# SoulCoreKit Contributing Guide

## Overview

This document defines the contribution workflow and standards for SoulCoreKit.

---

## Code of Conduct

### Respect
- Be respectful to all contributors
- Listen to different opinions
- Provide constructive feedback

### Quality
- Maintain high code quality
- Follow existing patterns
- Write tests for new code

### Transparency
- Document your changes
- Explain your reasoning
- Follow the review process

---

## Getting Started

### Prerequisites

- Qt 6.5+
- CMake 3.16+
- C++17 compatible compiler
- Git

### Fork the Repository

```bash
git clone https://github.com/yourusername/SoulCoreKit.git
cd SoulCoreKit
```

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Contribution Workflow

### 1. Find an Issue

- Check issues labeled "good first issue"
- Check issues labeled "help wanted"
- Discuss your plan in the issue

### 2. Create a Branch

```bash
git checkout -b feature/your-feature-name
```

### 3. Make Changes

- Follow coding style guidelines
- Write tests
- Update documentation

### 4. Commit

#### Commit Message Format

```
<type>(<scope>): <description>

<optional body>

<optional footer>
```

#### Types

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation changes |
| `style` | Code style changes |
| `refactor` | Refactoring |
| `test` | Test changes |
| `chore` | Build/CI changes |

#### Examples

```
feat(ui): add breathing animation to Button

- Implement sine-based breathing animation
- Add animation cache to prevent stacking
- Update Button QSS for glow effect

Closes #123
```

### 5. Push

```bash
git push origin feature/your-feature-name
```

### 6. Create Pull Request

- Title: Same as commit message
- Description: Explain the changes
- Reference related issues
- Request review from maintainers

### 7. Review Process

- Address review comments
- Update tests if needed
- Wait for approval

### 8. Merge

- Squash and merge
- Delete the branch

---

## Code Review Guidelines

### Reviewers

- Check for code quality
- Verify test coverage
- Ensure API compatibility
- Follow design principles

### Contributor

- Respond to comments
- Make requested changes
- Rebase if necessary

---

## Testing

### Run Tests

```bash
cd build
ctest -v
```

### Test Requirements

- New code must have unit tests
- Tests must pass on all platforms
- Coverage must meet targets

---

## Documentation

### Doxygen

- All public API must have Doxygen comments
- Follow documentation standards
- Build docs locally to verify

### Update Docs

- Update relevant documentation files
- Add usage examples
- Update changelog

---

## License

### License

SoulCoreKit is licensed under MIT License.

### Contributor License Agreement

By contributing, you agree that your contributions will be licensed under the same license.

---

## Questions

- Create an issue for questions
- Join discussions
- Ask maintainers for help

---

## Contribution Review Checklist

- ☑ Branch follows naming convention
- ☑ Commit messages follow format
- ☑ Code follows coding style
- ☑ Tests are added/updated
- ☑ Documentation is updated
- ☑ Changelog is updated
- ☑ All tests pass
- ☑ API compatibility verified
- ☑ Pull request description is clear
- ☑ Review comments addressed