# SoulCoreKit Design Principles

## Overview

These principles define the fundamental philosophy guiding all design and implementation decisions in SoulCoreKit. They are ordered by priority—lower priority principles must not violate higher priority ones.

---

## Principle 1: Architecture First

**Core Spirit**: No code change shall violate the existing architecture.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Achieve decoupling through interface abstraction (e.g., `IGlassEffect`) | Directly depend on concrete implementation classes |
| Follow existing module boundaries (core/base/ui/network, etc.) | Arbitrary cross-module calls or file movement |
| Add new functionality through extension, not modification of public interfaces | Modify existing public interface signatures |
| Maintain unidirectional dependency flow | Introduce circular dependencies |

**Consequences of Violation**: Reduced modularity, increased coupling, impeded future maintenance and extensibility.

**Verification**: Review new code for adherence to existing architecture patterns and check for new cross-layer dependencies.

---

## Principle 2: Component First

**Core Spirit**: All UI must prioritize reuse of existing components; no reinventing the wheel.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Use standard components from `soul/ui/soul_ui.h` | Copy identical UI code across multiple pages |
| Abstract UI used twice or more into public components | Create page-specific UI code |
| Extend component capabilities through inheritance or composition | Modify component source code for specific scenarios |
| Contribute improvements to the public component library | Implement similar functionality in business code |

**Consequences of Violation**: Code redundancy, increased maintenance cost, visual inconsistency.

**Verification**: Search the codebase for duplicate UI implementation patterns.

---

## Principle 3: Single Responsibility

**Core Spirit**: Each class is responsible for exactly one thing; no mixing of concerns.

**Class Responsibility Matrix**:

| Class | Responsibility | Forbidden |
|-------|---------------|-----------|
| `HomePage` | Page layout | Business logic, network requests |
| `MusicCard` | Song card display | Data fetching, state management |
| `SearchBar` | Search input UI | Search business logic |
| `PlayerController` | Playback control UI | Media decoding, network playback |
| `ThemeManager` | Theme management | UI rendering, business logic |
| `Animation` | Animation effects | Business logic, UI layout |
| `IconManager` | Icon management | Business logic, network requests |

**Consequences of Violation**: Class bloat, difficulty testing, high coupling.

**Verification**: Review class methods and member variables to identify responsibilities beyond the single purpose.

---

## Principle 4: Unified Design Language

**Core Spirit**: The entire UI component library must maintain consistent visual and interactive experiences.

**Design Token Specification**:

| Design Dimension | Unified Method | Corresponding Class |
|------------------|---------------|---------------------|
| **Color** | Managed via `ColorRole` enum | `Style::color(ColorRole)` |
| **Font** | Unified return via `Style::font()` | `Style` |
| **Corner Radius** | Managed via `CornerRadius` enum | `Style::cornerRadius(CornerRadius)` |
| **Spacing** | Managed via `Spacing` enum | `Style::spacing(Spacing)` |
| **Shadow** | Generated from theme colors | `Theme` |
| **Animation** | Applied via `Animation` static methods | `Animation` |
| **Icon** | Managed via `IconManager` singleton | `IconManager` |
| **Interaction** | Unified hover/click/focus feedback patterns | All components |

**Extended Color Roles**:

| Color Role | Semantic | Light Theme | Dark Theme |
|------------|----------|-------------|------------|
| `Success` | Success state | `#10b981` | `#34d399` |
| `GlassBackground` | Glassmorphism background | `#ffffff` | `#1e293b` |
| `GlassTint` | Glassmorphism tint layer | `rgba(28,28,30,65)` | `rgba(255,255,255,40)` |
| `GlowColor` | Glow effect color | `#6366f1` | `#818cf8` |

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Get design tokens via `Theme::instance().style()` | Hardcode color values, pixel values |
| Follow unified animation patterns (breathing/glow/bounce) | Use independent animation patterns per component |
| Use default styles of standard components | Create independent design styles for specific pages |
| Implement dark/light mode via theme switching | Branch on colors in code |

**Consequences of Violation**: Visual chaos, inconsistent user experience, increased maintenance difficulty.

**Verification**: Check code for hardcoded colors, fonts, spacing, and other design values.

---

## Principle 5: Minimal Change

**Core Spirit**: Prioritize reuse of existing architecture and components when meeting requirements; no over-engineering.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Extend functionality based on existing architecture | Large-scale refactoring for local feature development |
| Reuse existing components and utility classes | Arbitrarily introduce new design patterns |
| Maintain backward compatibility of public interfaces | Modify public interfaces causing caller crashes |
| Add code in existing files | Arbitrarily move files or rename modules |

**Architecture Adjustment Process**:

```
Analyze existing implementation → Explain modification rationale → Assess impact scope → Provide compatibility plan → Implement changes → Regression testing
```

**Consequences of Violation**: Unnecessary complexity, increased testing cost, broken compatibility.

**Verification**: Evaluate whether each modification is the minimally necessary change.

---

## Principle 6: Interface First

**Core Spirit**: Public components must define contracts through pure virtual function interfaces; separate implementation from interface.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Define pure virtual function interfaces (e.g., `IGlassEffect`) | Use concrete classes directly as parameter types |
| Declare interface methods with `= 0` | Provide default implementations in interfaces |
| Implementation classes inherit interfaces and implement all methods | Skip implementing certain interface methods |
| Separate interface and implementation in different files | Mix interface and implementation in the same file |

**SoulCoreKit Practice**:
- `IGlassEffect` defines the standard interface for glassmorphism effects
- All components follow the interface-first principle for easy future replacement

**Consequences of Violation**: Difficulty extending and replacing implementations, violation of dependency inversion principle.

---

## Principle 7: Qt6 Compatibility

**Core Spirit**: Code must be compatible with Qt6; no deprecated APIs.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| `event->globalPosition().toPoint()` | `event->globalPos()` |
| `QPropertyAnimation` for animation properties | `QWidget::setTransform()` / `setScale()` |
| `QPainterPath` for drawing paths | Directly manipulate low-level graphics APIs |
| `QGraphicsBlurEffect` for blur effects | Manual pixel-level blur |

**SoulCoreKit Practice**:
- `Window::mousePressEvent()` migrated from `globalPos()` to `globalPosition()`
- `Animation` class uses `geometry` property animation instead of `QTransform`

**Consequences of Violation**: Compilation warnings, runtime errors, future version compatibility issues.

---

## Principle 8: Naming and Export Convention

**Core Spirit**: All components must follow unified naming and export conventions.

**Naming Conventions**:

| Type | Rule | Example |
|------|------|---------|
| Class | PascalCase | `GlassWidget`, `Progress` |
| File | snake_case | `glass_widget.h`, `progress.cpp` |
| Enum | PascalCase with `Role` suffix | `ColorRole`, `CornerRadius` |
| Namespace | `sc` (SoulCore abbreviation) | `namespace sc` |

**Export Conventions**:

| Rule | Description |
|------|-------------|
| Unified Entry | Export all components through `soul/ui/soul_ui.h` |
| Header Guard | Use `#ifndef SOUL_UI_XXX_H` format |
| Source File | `.cpp` implementation corresponding to header |
| MOC Support | Classes requiring signal/slot must add `Q_OBJECT` macro |

**Consequences of Violation**: Code chaos, difficulty finding and maintaining, compilation errors.

---

## Principle 9: Test Coverage

**Core Spirit**: All public components must have corresponding test coverage.

**Rules**:

| Recommended | Prohibited |
|-------------|------------|
| Write unit tests for each component | Skip component testing before release |
| Test core functionality and edge cases | Only test normal paths |
| Test theme switching impact on components | Ignore theme compatibility testing |
| Test animation correctness | Skip animation-related testing |

**Consequences of Violation**: Component defects undetected, increased regression risk, impaired user experience.

---

## Principle Priority Summary

```
Principle 1: Architecture First          ← Highest priority, determines overall structure
    ↓
Principle 2: Component First             ← Determines code reuse strategy
    ↓
Principle 3: Single Responsibility       ← Determines class design quality
    ↓
Principle 4: Unified Design Language     ← Determines visual consistency
    ↓
Principle 5: Minimal Change              ← Determines implementation approach
    ↓
Principle 6: Interface First             ← Determines interface design
    ↓
Principle 7: Qt6 Compatibility           ← Determines technology selection
    ↓
Principle 8: Naming and Export           ← Determines code conventions
    ↓
Principle 9: Test Coverage               ← Determines code quality assurance
```

**Note**: Lower priority principles must not violate higher priority principles. When conflicts arise between principles, the higher priority principle takes precedence.