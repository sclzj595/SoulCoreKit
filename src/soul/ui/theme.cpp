#include "soul/ui/theme.h"
#include "soul/configuration/config.h"
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QOperatingSystemVersion>

namespace sc {

Theme::Theme() : m_mode(ThemeMode::Dark) {
}

void Theme::init() {
    restoreThemeMode();
}

void Theme::shutdown() {
}

void Theme::setMode(ThemeMode mode) {
    if (m_mode == mode) {
        return;
    }

    m_mode = mode;

    switch (mode) {
        case ThemeMode::Light:
            loadLightTheme();
            break;
        case ThemeMode::Dark:
            loadDarkTheme();
            break;
        case ThemeMode::System:
            loadSystemTheme();
            break;
    }

    saveThemeMode();
    applyToApp();
    emit themeChanged();
}

ThemeMode Theme::mode() const {
    return m_mode;
}

const Style& Theme::style() const {
    return m_style;
}

Style& Theme::style() {
    return m_style;
}

void Theme::applyToApp() {
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) {
        return;
    }

    QPalette palette = app->palette();
    palette.setColor(QPalette::Window, m_style.color(ColorRole::Background));
    palette.setColor(QPalette::WindowText, m_style.color(ColorRole::TextPrimary));
    palette.setColor(QPalette::Base, m_style.color(ColorRole::Surface));
    palette.setColor(QPalette::AlternateBase, m_style.color(ColorRole::SurfaceVariant));
    palette.setColor(QPalette::ToolTipBase, m_style.color(ColorRole::Surface));
    palette.setColor(QPalette::ToolTipText, m_style.color(ColorRole::TextPrimary));
    palette.setColor(QPalette::Text, m_style.color(ColorRole::TextPrimary));
    palette.setColor(QPalette::Button, m_style.color(ColorRole::Surface));
    palette.setColor(QPalette::ButtonText, m_style.color(ColorRole::TextPrimary));
    palette.setColor(QPalette::BrightText, m_style.color(ColorRole::Error));
    palette.setColor(QPalette::Highlight, m_style.color(ColorRole::Primary));
    palette.setColor(QPalette::HighlightedText, m_style.color(ColorRole::OnPrimary));

    app->setPalette(palette);
    app->setFont(m_style.font());
}

bool Theme::isSystemDarkMode() {
#ifdef Q_OS_WINDOWS
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#elif defined(Q_OS_MACOS)
    QSettings settings("Apple Global Domain", QSettings::NativeFormat);
    return settings.value("AppleInterfaceStyle", "").toString() == "Dark";
#else
    return false;
#endif
}

void Theme::saveThemeMode() {
    Config::instance().setString("theme.mode", QString::number(static_cast<int>(m_mode)));
}

void Theme::restoreThemeMode() {
    QString modeStr = Config::instance().getString("theme.mode", "0");
    int modeInt = modeStr.toInt();
    
    if (modeInt >= 0 && modeInt <= static_cast<int>(ThemeMode::System)) {
        m_mode = static_cast<ThemeMode>(modeInt);
    } else {
        m_mode = ThemeMode::Dark;
    }

    switch (m_mode) {
        case ThemeMode::Light:
            loadLightTheme();
            break;
        case ThemeMode::Dark:
            loadDarkTheme();
            break;
        case ThemeMode::System:
            loadSystemTheme();
            break;
    }
}

void Theme::onSystemThemeChanged() {
    if (m_mode == ThemeMode::System) {
        loadSystemTheme();
        applyToApp();
        emit themeChanged();
    }
}

void Theme::loadLightTheme() {
    m_style.setColor(ColorRole::Primary, QColor("#6366f1"));
    m_style.setColor(ColorRole::PrimaryLight, QColor("#818cf8"));
    m_style.setColor(ColorRole::PrimaryDark, QColor("#4f46e5"));
    m_style.setColor(ColorRole::Secondary, QColor("#f59e0b"));
    m_style.setColor(ColorRole::SecondaryLight, QColor("#fbbf24"));
    m_style.setColor(ColorRole::SecondaryDark, QColor("#d97706"));
    m_style.setColor(ColorRole::Background, QColor("#f8fafc"));
    m_style.setColor(ColorRole::Surface, QColor("#ffffff"));
    m_style.setColor(ColorRole::SurfaceVariant, QColor("#e2e8f0"));
    m_style.setColor(ColorRole::Error, QColor("#ef4444"));
    m_style.setColor(ColorRole::ErrorContainer, QColor("#fef2f2"));
    m_style.setColor(ColorRole::Success, QColor("#10b981"));
    m_style.setColor(ColorRole::OnPrimary, QColor("#ffffff"));
    m_style.setColor(ColorRole::OnSecondary, QColor("#0f172a"));
    m_style.setColor(ColorRole::OnBackground, QColor("#1e293b"));
    m_style.setColor(ColorRole::OnSurface, QColor("#1e293b"));
    m_style.setColor(ColorRole::OnSurfaceVariant, QColor("#64748b"));
    m_style.setColor(ColorRole::OnError, QColor("#ffffff"));
    m_style.setColor(ColorRole::TextPrimary, QColor("#1e293b"));
    m_style.setColor(ColorRole::TextSecondary, QColor("#64748b"));
    m_style.setColor(ColorRole::TextTertiary, QColor("#94a3b8"));
    m_style.setColor(ColorRole::Border, QColor("#e2e8f0"));
    m_style.setColor(ColorRole::BorderLight, QColor("#f1f5f9"));
    m_style.setColor(ColorRole::Shadow, QColor("#000000"));
    m_style.setColor(ColorRole::Overlay, QColor("#000000"));
    m_style.setColor(ColorRole::GlassBackground, QColor("#ffffff"));
    m_style.setColor(ColorRole::GlassTint, QColor(28, 28, 30, 65));
    m_style.setColor(ColorRole::GlowColor, QColor("#6366f1"));
}

void Theme::loadDarkTheme() {
    m_style.setColor(ColorRole::Primary, QColor("#818cf8"));
    m_style.setColor(ColorRole::PrimaryLight, QColor("#a5b4fc"));
    m_style.setColor(ColorRole::PrimaryDark, QColor("#6366f1"));
    m_style.setColor(ColorRole::Secondary, QColor("#fbbf24"));
    m_style.setColor(ColorRole::SecondaryLight, QColor("#fcd34d"));
    m_style.setColor(ColorRole::SecondaryDark, QColor("#f59e0b"));
    m_style.setColor(ColorRole::Background, QColor("#0f172a"));
    m_style.setColor(ColorRole::Surface, QColor("#1e293b"));
    m_style.setColor(ColorRole::SurfaceVariant, QColor("#334155"));
    m_style.setColor(ColorRole::Error, QColor("#f87171"));
    m_style.setColor(ColorRole::ErrorContainer, QColor("#450a0a"));
    m_style.setColor(ColorRole::Success, QColor("#34d399"));
    m_style.setColor(ColorRole::OnPrimary, QColor("#ffffff"));
    m_style.setColor(ColorRole::OnSecondary, QColor("#0f172a"));
    m_style.setColor(ColorRole::OnBackground, QColor("#f8fafc"));
    m_style.setColor(ColorRole::OnSurface, QColor("#f1f5f9"));
    m_style.setColor(ColorRole::OnSurfaceVariant, QColor("#cbd5e1"));
    m_style.setColor(ColorRole::OnError, QColor("#ffffff"));
    m_style.setColor(ColorRole::TextPrimary, QColor("#f8fafc"));
    m_style.setColor(ColorRole::TextSecondary, QColor("#94a3b8"));
    m_style.setColor(ColorRole::TextTertiary, QColor("#64748b"));
    m_style.setColor(ColorRole::Border, QColor("#334155"));
    m_style.setColor(ColorRole::BorderLight, QColor("#475569"));
    m_style.setColor(ColorRole::Shadow, QColor("#000000"));
    m_style.setColor(ColorRole::Overlay, QColor("#000000"));
    m_style.setColor(ColorRole::GlassBackground, QColor("#1e293b"));
    m_style.setColor(ColorRole::GlassTint, QColor(255, 255, 255, 40));
    m_style.setColor(ColorRole::GlowColor, QColor("#818cf8"));
}

void Theme::loadSystemTheme() {
    if (isSystemDarkMode()) {
        loadDarkTheme();
    } else {
        loadLightTheme();
    }
}

}
