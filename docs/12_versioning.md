# SoulCoreKit Versioning Policy

## Overview

This document defines the versioning strategy for SoulCoreKit, based on Semantic Versioning (SemVer) 2.0.

---

## Semantic Versioning

### Format

```
MAJOR.MINOR.PATCH
```

### MAJOR Version

- **Increment**: Breaking API changes
- **Compatibility**: Not backward compatible
- **Examples**:
  - Remove public API
  - Change method signatures
  - Rename classes or functions
  - Remove enum values

### MINOR Version

- **Increment**: New features, backward compatible
- **Compatibility**: Fully backward compatible
- **Examples**:
  - Add new public classes
  - Add new methods to existing classes
  - Add new enum values
  - Add new parameters with default values
  - Deprecate (but not remove) existing API

### PATCH Version

- **Increment**: Bug fixes only
- **Compatibility**: Fully backward compatible
- **Examples**:
  - Fix bugs
  - Improve performance
  - Fix documentation

---

## API Stability

### Public API

**Definition**: All classes, functions, and constants exposed in the `include/soul/` directory.

**Guarantee**: Public APIs remain compatible across **minor versions**.

**Example**:
```
1.0 → 1.1 → 1.2: Backward compatible
1.2 → 2.0: Breaking changes allowed
```

### ABI Stability

**Definition**: Binary interface for dynamically linked libraries.

**Guarantee**: ABI remains stable across **minor versions**.

**Prohibited Changes**:
- Reorder class members
- Change virtual table layout
- Change size of exported classes
- Remove export symbols

---

## Version Query API

### Runtime Version Check

```cpp
#include "soul/core/version.h"

int major = sc::Version::major();      // 1
int minor = sc::Version::minor();      // 0
int patch = sc::Version::patch();      // 0
QString version = sc::Version::toString(); // "1.0.0"
```

### Compile-Time Version Check

```cpp
#if SOUL_CORE_VERSION >= SOUL_CORE_VERSION_CHECK(1, 1, 0)
    // Use new feature
#endif
```

---

## Release Process

### Pre-Release Checklist

- [ ] All tests pass
- [ ] Documentation updated
- [ ] API compatibility verified
- [ ] Version number updated in `version.h`
- [ ] Changelog updated

### Release Branch

```bash
git checkout -b release/1.0.0
```

### Tagging

```bash
git tag -a v1.0.0 -m "Version 1.0.0"
git push origin v1.0.0
```

### Changelog

Each release must have a changelog entry:

```markdown
## 1.0.0 (2024-01-01)

### Added
- Initial release
- Core module with Result<T> and Error types
- UI module with glassmorphism widgets
- Network module with HTTP client

### Fixed
- None

### Deprecated
- None
```

---

## Deprecation Policy

### Marking Deprecated

- Use `[[deprecated]]` attribute
- Add `@deprecated` Doxygen tag
- Provide migration guide

**Example**:
```cpp
/**
 * @deprecated Use newMethod() instead
 */
[[deprecated("Use newMethod() instead")]]
void oldMethod();
```

### Removal

- Deprecated API must remain for at least one major version
- Removal must be announced in release notes
- Migration guide must be provided

---

## Compatibility Matrix

| Version | Qt 6.5 | Qt 6.6 | Qt 6.7 |
|---------|--------|--------|--------|
| 1.0.x | ✅ | ✅ | ✅ |
| 1.1.x | ✅ | ✅ | ✅ |
| 2.0.x | ❌ | ✅ | ✅ |

---

## Versioning Review Checklist

- ☑ Version follows SemVer format
- ☑ MAJOR version incremented for breaking changes
- ☑ MINOR version incremented for new features
- ☑ PATCH version incremented for bug fixes
- ☑ Public API remains compatible across minor versions
- ☑ ABI remains stable across minor versions
- ☑ Version API is up-to-date
- ☑ Changelog is updated
- ☑ Deprecated API is properly marked
- ☑ Release process is followed