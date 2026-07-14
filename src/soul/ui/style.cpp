#include "soul/ui/style.h"

namespace sc {

Style::Style() {
    m_colors.primary = QColor("#6366f1");
    m_colors.primaryLight = QColor("#818cf8");
    m_colors.primaryDark = QColor("#4f46e5");
    m_colors.secondary = QColor("#f59e0b");
    m_colors.secondaryLight = QColor("#fbbf24");
    m_colors.secondaryDark = QColor("#d97706");
    m_colors.background = QColor("#0f172a");
    m_colors.surface = QColor("#1e293b");
    m_colors.surfaceVariant = QColor("#334155");
    m_colors.error = QColor("#ef4444");
    m_colors.errorContainer = QColor("#fef2f2");
    m_colors.success = QColor("#10b981");
    m_colors.onPrimary = QColor("#ffffff");
    m_colors.onSecondary = QColor("#0f172a");
    m_colors.onBackground = QColor("#f8fafc");
    m_colors.onSurface = QColor("#f1f5f9");
    m_colors.onSurfaceVariant = QColor("#cbd5e1");
    m_colors.onError = QColor("#ffffff");
    m_colors.textPrimary = QColor("#f8fafc");
    m_colors.textSecondary = QColor("#94a3b8");
    m_colors.textTertiary = QColor("#64748b");
    m_colors.border = QColor("#334155");
    m_colors.borderLight = QColor("#475569");
    m_colors.shadow = QColor("#000000");
    m_colors.overlay = QColor("#000000");
    m_colors.glassBackground = QColor("#1e293b");
    m_colors.glassTint = QColor(255, 255, 255, 40);
    m_colors.glowColor = QColor("#6366f1");

    m_cornerRadii.none = 0;
    m_cornerRadii.small = 4;
    m_cornerRadii.medium = 8;
    m_cornerRadii.large = 12;
    m_cornerRadii.extraLarge = 16;
    m_cornerRadii.full = 9999;

    m_spacings.none = 0;
    m_spacings.tiny = 4;
    m_spacings.small = 8;
    m_spacings.medium = 16;
    m_spacings.large = 24;
    m_spacings.extraLarge = 32;

    m_font = QFont("Segoe UI", 14);
}

QColor Style::color(ColorRole role) const {
    switch (role) {
        case ColorRole::Primary: return m_colors.primary;
        case ColorRole::PrimaryLight: return m_colors.primaryLight;
        case ColorRole::PrimaryDark: return m_colors.primaryDark;
        case ColorRole::Secondary: return m_colors.secondary;
        case ColorRole::SecondaryLight: return m_colors.secondaryLight;
        case ColorRole::SecondaryDark: return m_colors.secondaryDark;
        case ColorRole::Background: return m_colors.background;
        case ColorRole::Surface: return m_colors.surface;
        case ColorRole::SurfaceVariant: return m_colors.surfaceVariant;
        case ColorRole::Error: return m_colors.error;
        case ColorRole::ErrorContainer: return m_colors.errorContainer;
        case ColorRole::Success: return m_colors.success;
        case ColorRole::OnPrimary: return m_colors.onPrimary;
        case ColorRole::OnSecondary: return m_colors.onSecondary;
        case ColorRole::OnBackground: return m_colors.onBackground;
        case ColorRole::OnSurface: return m_colors.onSurface;
        case ColorRole::OnSurfaceVariant: return m_colors.onSurfaceVariant;
        case ColorRole::OnError: return m_colors.onError;
        case ColorRole::TextPrimary: return m_colors.textPrimary;
        case ColorRole::TextSecondary: return m_colors.textSecondary;
        case ColorRole::TextTertiary: return m_colors.textTertiary;
        case ColorRole::Border: return m_colors.border;
        case ColorRole::BorderLight: return m_colors.borderLight;
        case ColorRole::Shadow: return m_colors.shadow;
        case ColorRole::Overlay: return m_colors.overlay;
        case ColorRole::GlassBackground: return m_colors.glassBackground;
        case ColorRole::GlassTint: return m_colors.glassTint;
        case ColorRole::GlowColor: return m_colors.glowColor;
        default: return m_colors.primary;
    }
}

void Style::setColor(ColorRole role, const QColor& color) {
    switch (role) {
        case ColorRole::Primary: m_colors.primary = color; break;
        case ColorRole::PrimaryLight: m_colors.primaryLight = color; break;
        case ColorRole::PrimaryDark: m_colors.primaryDark = color; break;
        case ColorRole::Secondary: m_colors.secondary = color; break;
        case ColorRole::SecondaryLight: m_colors.secondaryLight = color; break;
        case ColorRole::SecondaryDark: m_colors.secondaryDark = color; break;
        case ColorRole::Background: m_colors.background = color; break;
        case ColorRole::Surface: m_colors.surface = color; break;
        case ColorRole::SurfaceVariant: m_colors.surfaceVariant = color; break;
        case ColorRole::Error: m_colors.error = color; break;
        case ColorRole::ErrorContainer: m_colors.errorContainer = color; break;
        case ColorRole::Success: m_colors.success = color; break;
        case ColorRole::OnPrimary: m_colors.onPrimary = color; break;
        case ColorRole::OnSecondary: m_colors.onSecondary = color; break;
        case ColorRole::OnBackground: m_colors.onBackground = color; break;
        case ColorRole::OnSurface: m_colors.onSurface = color; break;
        case ColorRole::OnSurfaceVariant: m_colors.onSurfaceVariant = color; break;
        case ColorRole::OnError: m_colors.onError = color; break;
        case ColorRole::TextPrimary: m_colors.textPrimary = color; break;
        case ColorRole::TextSecondary: m_colors.textSecondary = color; break;
        case ColorRole::TextTertiary: m_colors.textTertiary = color; break;
        case ColorRole::Border: m_colors.border = color; break;
        case ColorRole::BorderLight: m_colors.borderLight = color; break;
        case ColorRole::Shadow: m_colors.shadow = color; break;
        case ColorRole::Overlay: m_colors.overlay = color; break;
        case ColorRole::GlassBackground: m_colors.glassBackground = color; break;
        case ColorRole::GlassTint: m_colors.glassTint = color; break;
        case ColorRole::GlowColor: m_colors.glowColor = color; break;
    }
}

int Style::cornerRadius(CornerRadius radius) const {
    switch (radius) {
        case CornerRadius::None: return m_cornerRadii.none;
        case CornerRadius::Small: return m_cornerRadii.small;
        case CornerRadius::Medium: return m_cornerRadii.medium;
        case CornerRadius::Large: return m_cornerRadii.large;
        case CornerRadius::ExtraLarge: return m_cornerRadii.extraLarge;
        case CornerRadius::Full: return m_cornerRadii.full;
        default: return m_cornerRadii.medium;
    }
}

void Style::setCornerRadius(CornerRadius radius, int value) {
    switch (radius) {
        case CornerRadius::None: m_cornerRadii.none = value; break;
        case CornerRadius::Small: m_cornerRadii.small = value; break;
        case CornerRadius::Medium: m_cornerRadii.medium = value; break;
        case CornerRadius::Large: m_cornerRadii.large = value; break;
        case CornerRadius::ExtraLarge: m_cornerRadii.extraLarge = value; break;
        case CornerRadius::Full: m_cornerRadii.full = value; break;
    }
}

int Style::spacing(Spacing space) const {
    switch (space) {
        case Spacing::None: return m_spacings.none;
        case Spacing::Tiny: return m_spacings.tiny;
        case Spacing::Small: return m_spacings.small;
        case Spacing::Medium: return m_spacings.medium;
        case Spacing::Large: return m_spacings.large;
        case Spacing::ExtraLarge: return m_spacings.extraLarge;
        default: return m_spacings.medium;
    }
}

void Style::setSpacing(Spacing space, int value) {
    switch (space) {
        case Spacing::None: m_spacings.none = value; break;
        case Spacing::Tiny: m_spacings.tiny = value; break;
        case Spacing::Small: m_spacings.small = value; break;
        case Spacing::Medium: m_spacings.medium = value; break;
        case Spacing::Large: m_spacings.large = value; break;
        case Spacing::ExtraLarge: m_spacings.extraLarge = value; break;
    }
}

QFont Style::font() const {
    return m_font;
}

void Style::setFont(const QFont& font) {
    m_font = font;
}

QFont Style::boldFont() const {
    QFont bold = m_font;
    bold.setBold(true);
    return bold;
}

QFont Style::smallFont() const {
    QFont small = m_font;
    small.setPointSize(m_font.pointSize() - 2);
    return small;
}

QFont Style::largeFont() const {
    QFont large = m_font;
    large.setPointSize(m_font.pointSize() + 4);
    return large;
}

}