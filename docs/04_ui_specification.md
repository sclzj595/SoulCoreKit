# SoulCoreKit UI Specification

## Overview

This document specifies the visual design, interaction patterns, and technical implementation standards for the SoulCoreKit UI component library.

---

## Design Philosophy

### Native Quality · Immersive Interaction

The UI component library strictly follows the philosophy of "iOS-style quality + desktop native performance". All visual and interaction designs revolve around this core.

### Visual Language

- **Glassmorphism Base**: Transparent, high-quality texture
- **Glow Edge Accent**: Subtle glow effects for emphasis
- **Consistent Design Tokens**: Colors, spacing, corners managed centrally

### Interaction Soul

- **Breathing Effect**: Subtle hover, pulse, and bounce animations
- **Smooth Transitions**: 60fps animation performance
- **Instant Feedback**: Clear visual response to user actions

### Technical Foundation

- **Pure Qt Widgets + QSS**: No Qt Quick dependency
- **Configuration-Driven Styles**: QSS generated dynamically from theme system
- **Composition Over Inheritance**: Build complex components by composing simpler ones

---

## Design Tokens

### Color System

| Color Role | Semantic | Light Theme | Dark Theme |
|------------|----------|-------------|------------|
| `Primary` | Main brand color | `#6366f1` | `#818cf8` |
| `PrimaryHover` | Primary on hover | `#818cf8` | `#a5b4fc` |
| `Secondary` | Secondary color | `#64748b` | `#94a3b8` |
| `Success` | Success state | `#10b981` | `#34d399` |
| `Warning` | Warning state | `#f59e0b` | `#fbbf24` |
| `Error` | Error state | `#ef4444` | `#f87171` |
| `Info` | Information | `#3b82f6` | `#60a5fa` |
| `TextPrimary` | Primary text | `#1c1c1e` | `#f5f5f7` |
| `TextSecondary` | Secondary text | `#86868b` | `#8e8e93` |
| `TextTertiary` | Tertiary text | `#aeaeaf` | `#636366` |
| `Background` | Main background | `#ffffff` | `#000000` |
| `Surface` | Surface background | `#f2f2f7` | `#1c1c1e` |
| `GlassBackground` | Glassmorphism bg | `#ffffff` | `#1e293b` |
| `GlassTint` | Glassmorphism tint | `rgba(28,28,30,65)` | `rgba(255,255,255,40)` |
| `GlowColor` | Glow effect | `#6366f1` | `#818cf8` |
| `Border` | Border color | `#e5e5ea` | `#38383a` |
| `Divider` | Divider color | `#f2f2f7` | `#2c2c2e` |

### Typography

| Token | Value | Description |
|-------|-------|-------------|
| `FontFamily` | System default | Platform-native font |
| `FontSizeXS` | 10px | Extra small text |
| `FontSizeSM` | 12px | Small text |
| `FontSizeMD` | 14px | Normal text |
| `FontSizeLG` | 16px | Large text |
| `FontSizeXL` | 18px | Extra large text |
| `FontSizeXXL` | 24px | Title text |
| `FontWeightRegular` | 400 | Regular weight |
| `FontWeightMedium` | 500 | Medium weight |
| `FontWeightSemibold` | 600 | Semibold weight |

### Spacing

| Token | Value | Description |
|-------|-------|-------------|
| `SpacingXS` | 4px | Extra small gap |
| `SpacingSM` | 8px | Small gap |
| `SpacingMD` | 16px | Normal gap |
| `SpacingLG` | 24px | Large gap |
| `SpacingXL` | 32px | Extra large gap |
| `SpacingXXL` | 48px | Section gap |

### Corner Radius

| Token | Value | Description |
|-------|-------|-------------|
| `RadiusNone` | 0px | Sharp corners |
| `RadiusSM` | 8px | Small rounded |
| `RadiusMD` | 12px | Normal rounded |
| `RadiusLG` | 16px | Large rounded |
| `RadiusXL` | 24px | Extra large rounded |
| `RadiusFull` | 9999px | Pill shape |

### Shadows

| Token | Value | Description |
|-------|-------|-------------|
| `ShadowSM` | `0 2px 4px rgba(0,0,0,0.05)` | Small shadow |
| `ShadowMD` | `0 4px 12px rgba(0,0,0,0.08)` | Normal shadow |
| `ShadowLG` | `0 8px 24px rgba(0,0,0,0.12)` | Large shadow |
| `ShadowGlow` | `0 0 20px rgba(99,102,241,0.3)` | Glow effect |

### Animation

| Token | Value | Description |
|-------|-------|-------------|
| `DurationFast` | 100ms | Fast transition |
| `DurationNormal` | 200ms | Normal transition |
| `DurationSlow` | 300ms | Slow transition |
| `DurationBreathing` | 1200ms | Breathing cycle |
| `EasingStandard` | `QEasingCurve::InOutQuad` | Standard easing |
| `EasingSpring` | `QEasingCurve::OutElastic` | Spring effect |
| `EasingSine` | `QEasingCurve::InOutSine` | Smooth sine |

---

## Glassmorphism Implementation

### Strategy

Since QSS does not natively support `backdrop-filter`, we implement glassmorphism in C++ while controlling parameters via QSS.

### Implementation Steps

1. In `GlassWidget::paintEvent()`, capture the underlying window background
2. Apply Gaussian blur using `QGraphicsBlurEffect`
3. Draw the blurred image as background
4. Overlay with semi-transparent color controlled by QSS

### QSS Custom Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `glass-opacity` | double | 0.65 | Background opacity |
| `glass-blur` | int | 20 | Blur radius in pixels |
| `glass-tint-color` | color | rgba(255,255,255,0.3) | Tint color |

### Performance Optimization

- Cache blurred results for static backgrounds
- Only redraw when position/size changes
- Use `QGraphicsBlurEffect` at lower blur radius for better performance

---

## Animation Specification

### Animation Types

| Type | Trigger | Effect | Curve | Duration |
|------|---------|--------|-------|----------|
| **Glow** | Hover / Focus | Glow opacity and range change | InOutQuad | 200ms |
| **Breathing** | Hover | Slight scale (1.0 ↔ 1.02) | Sine | 1200ms loop |
| **Bounce** | Press / Release | Press scale 0.97, release to 1.0 | OutElastic | 300ms |
| **Fade** | Page transition | Opacity change | InOutQuad | 200-300ms |
| **Slide** | Page transition | Position change | InOutQuad | 200-300ms |
| **Scale** | Appear / Disappear | Scale from 0.9 to 1.0 | OutBack | 300ms |

### Animation States

```
Idle → Hover → Pressed → Idle
   ↑                   ↓
   └───────────────────┘
```

### Animation Cache

- Each component maintains an animation cache to prevent stacking
- When a new animation starts, previous animations of the same type are stopped
- Use `QAnimationGroup` for coordinated multi-property animations

---

## Component Specifications

### Button

**States**:
- Normal: Semi-transparent background
- Hover: Glow edge + breathing pulse
- Pressed: Scale 0.97 + bounce back
- Disabled: Reduced opacity

**QSS Structure**:
```qss
Button {
    background-color: rgba(99, 102, 241, 0.1);
    color: #6366f1;
    border-radius: 12px;
    padding: 8px 16px;
}
Button:hover {
    background-color: rgba(99, 102, 241, 0.2);
}
Button:pressed {
    background-color: rgba(99, 102, 241, 0.3);
}
Button:disabled {
    opacity: 0.5;
}
```

### Card

**States**:
- Normal: Glassmorphism background + shadow
- Hover: Lift effect + glow edge + breathing

**QSS Structure**:
```qss
Card {
    background-color: rgba(255, 255, 255, 0.65);
    border-radius: 16px;
    padding: 16px;
}
```

### Input

**States**:
- Normal: Border + background
- Focus: Glow border + shadow

**QSS Structure**:
```qss
Input {
    background-color: rgba(255, 255, 255, 0.8);
    border: 1px solid #e5e5ea;
    border-radius: 12px;
    padding: 10px 14px;
}
Input:focus {
    border-color: #6366f1;
}
```

### Window

**Features**:
- Frameless with custom title bar
- Draggable
- Resizable
- Glassmorphism background
- Activation/deactivation glow adjustment

---

## Theme System

### Theme Types

| Theme | Description |
|-------|-------------|
| `Light` | Light color scheme |
| `Dark` | Dark color scheme |
| `System` | Follow system preference |

### Theme Switching

1. `Theme::setCurrentTheme(ThemeType)`
2. Emit `themeChanged()` signal
3. All components listen and update QSS
4. Settings persist via `Storage::Settings`

### System Theme Adaptation

- Windows: Monitor registry for theme changes
- macOS: Use `NSWorkspace` notifications
- Linux: Monitor `GTK_THEME` environment variable

---

## Layout System

### Layout Principles

- Use Qt's built-in layout managers (`QVBoxLayout`, `QHBoxLayout`, `QGridLayout`)
- Apply design tokens for spacing
- Responsive design via `QSizePolicy`

### Spacing Rules

| Layout Type | Spacing |
|-------------|---------|
| Form layout | `SpacingSM` |
| Card content | `SpacingMD` |
| Page layout | `SpacingLG` |
| Section layout | `SpacingXL` |

---

## Accessibility

### Requirements

- All components must support keyboard navigation
- Focus indicators must be visible
- High contrast mode support
- Screen reader compatibility

---

## Performance Requirements

- Component initialization: < 50ms per component
- Animation performance: 60fps minimum
- Memory usage: < 10MB for basic UI module
- Paint time: < 16ms per frame

---

## Cross-Platform Considerations

| Platform | Specific Considerations |
|----------|------------------------|
| **Windows** | DPI scaling, Acrylic effect support |
| **macOS** | Vibrancy effect, rounded corners |
| **Linux** | GTK theme integration, Wayland support |

---

## UI Review Checklist

- ☑ Uses design tokens instead of hardcoded values
- ☑ Follows animation specifications
- ☑ Implements glassmorphism correctly
- ☑ Supports both light and dark themes
- ☑ Has proper hover/press/disabled states
- ☑ Meets performance requirements
- ☑ Supports keyboard navigation
- ☑ Follows layout spacing rules
- ☑ Works across all target platforms
- ☑ Has unit tests for visual states