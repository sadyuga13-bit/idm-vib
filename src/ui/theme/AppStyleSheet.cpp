#include "ui/theme/AppStyleSheet.h"
#include "config/AppConfig.h"
#include "core/AppSettings.h"
#include "ui/theme/ThemeTokens.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

namespace idm::ui::theme {

namespace {
QString writeArrowSvg(const QString& fileName, const QString& colorHex, bool upArrow) {
    const QString path = QDir::fromNativeSeparators(QDir::tempPath() + QStringLiteral("/") + fileName);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString poly = upArrow ? QStringLiteral("18 15 12 9 6 15") : QStringLiteral("6 9 12 15 18 9");
        const QString svg = QStringLiteral(
            "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
            "stroke='%1' stroke-width='2.4' stroke-linecap='round' stroke-linejoin='round'>"
            "<polyline points='%2'/></svg>").arg(colorHex, poly);
        file.write(svg.toUtf8());
        file.close();
    }
    return path;
}
} // namespace

void AppStyleSheet::setDayMode(bool dayMode) {
    ThemeTokens::setDayMode(dayMode);
    config::AppConfig::instance().setDayMode(dayMode);
    core::AppSettings::instance().setDayMode(dayMode);
    config::AppConfig::instance().sync();
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        applyToApplication(*app);
        for (QWidget* w : QApplication::allWidgets()) {
            w->update();
        }
    }
}

void AppStyleSheet::applyToApplication(QApplication& app) {
    QLocale::setDefault(QLocale::c());
    ThemeTokens::setDayMode(config::AppConfig::instance().isDayMode());
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setFont(ThemeTokens::uiFont(false));

    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, ThemeTokens::bgWindow());
    pal.setColor(QPalette::WindowText, ThemeTokens::textPrimary());
    pal.setColor(QPalette::Base, ThemeTokens::bgWindow());
    pal.setColor(QPalette::AlternateBase, ThemeTokens::bgRowAlt());
    pal.setColor(QPalette::ToolTipBase, ThemeTokens::bgToolbar());
    pal.setColor(QPalette::ToolTipText, ThemeTokens::textPrimary());
    pal.setColor(QPalette::Text, ThemeTokens::textPrimary());
    pal.setColor(QPalette::Button, ThemeTokens::bgToolbar());
    pal.setColor(QPalette::ButtonText, ThemeTokens::textPrimary());
    pal.setColor(QPalette::BrightText, Qt::white);
    pal.setColor(QPalette::Highlight, ThemeTokens::bgSelected());
    pal.setColor(QPalette::HighlightedText, ThemeTokens::textPrimary());
    app.setPalette(pal);

    app.setStyleSheet(buildGlobalStyleSheet());
}

QString AppStyleSheet::buildGlobalStyleSheet() {
    const QString bgWin = ThemeTokens::bgWindow().name();
    const QString bgPan = ThemeTokens::bgPanel().name();
    const QString bgBar = ThemeTokens::bgToolbar().name();
    const QString bgAlt = ThemeTokens::bgRowAlt().name();
    const QString bgHov = ThemeTokens::bgHover().name();
    const QString bgSel = ThemeTokens::bgSelected().name();
    const QString border = ThemeTokens::borderHairline().name();
    const QString txtPri = ThemeTokens::textPrimary().name();
    const QString txtSec = ThemeTokens::textSecondary().name();
    const QString txtTec = ThemeTokens::textTechnical().name();
    const QString accent = ThemeTokens::statusActive().name();
    const QString upArrowUrl = writeArrowSvg(QStringLiteral("idm_vibe_up.svg"), txtPri, true);
    const QString downArrowUrl = writeArrowSvg(QStringLiteral("idm_vibe_down.svg"), txtPri, false);

    return QStringLiteral(R"(
        QMainWindow, QDialog { background-color: %1; color: %8; }
        QWidget { color: %8; font-size: 13px; selection-background-color: %6; selection-color: %8; }
        TopToolbar { background-color: %3; border-bottom: 1px solid %7; }
        SidebarTree { background-color: %2; border-right: 1px solid %7; }
        QSplitter::handle { background-color: %7; }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical { height: 1px; }
        QHeaderView { background-color: %3; border: none; border-bottom: 1px solid %7; }
        QHeaderView::section {
            background-color: %3; color: %9; font-weight: 600; font-size: 11px;
            padding: 6px 10px; border: none; border-right: 1px solid %7; border-bottom: 1px solid %7;
        }
        QHeaderView::section:hover { background-color: %5; color: %8; }
        QTableView, QTableWidget {
            background-color: %1; alternate-background-color: %4;
            border: none; gridline-color: %7; outline: none;
        }
        QTreeWidget, QListWidget {
            background-color: %2; border: 1px solid %7; border-radius: 6px; outline: none; padding: 6px 4px;
        }
        QTreeWidget::item, QListWidget::item {
            height: 30px; padding: 3px 8px; border-radius: 5px; color: %9;
        }
        QTreeWidget::item:hover, QListWidget::item:hover { background-color: %5; color: %8; }
        QTreeWidget::item:selected, QListWidget::item:selected {
            background-color: %6; color: %8; font-weight: 600;
        }
        QTabWidget::pane { background-color: %2; border-top: 1px solid %7; }
        QTabBar::tab {
            background-color: %3; color: %9; padding: 8px 16px; border: none;
            border-right: 1px solid %7; border-bottom: 2px solid transparent; font-weight: 600;
        }
        QTabBar::tab:hover { background-color: %5; color: %8; }
        QTabBar::tab:selected { background-color: %2; color: %8; border-bottom: 2px solid %11; }
        QLineEdit, QPlainTextEdit {
            background-color: %2; color: %8; border: 1px solid %7; border-radius: 6px; padding: 4px 8px; min-height: 22px;
        }
        QSpinBox, QComboBox, QTimeEdit {
            background-color: %2; color: %8; border: 1px solid %7; border-radius: 6px;
            padding: 4px 24px 4px 8px; min-height: 22px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTimeEdit:focus, QPlainTextEdit:focus {
            border: 1px solid %11;
        }
        QSpinBox::up-button, QTimeEdit::up-button {
            subcontrol-origin: border; subcontrol-position: top right; width: 20px;
            background-color: %3; border-left: 1px solid %7; border-bottom: 1px solid %7; border-top-right-radius: 5px;
        }
        QSpinBox::down-button, QTimeEdit::down-button {
            subcontrol-origin: border; subcontrol-position: bottom right; width: 20px;
            background-color: %3; border-left: 1px solid %7; border-bottom-right-radius: 5px;
        }
        QComboBox::drop-down {
            subcontrol-origin: border; subcontrol-position: top right; width: 22px;
            background-color: %3; border-left: 1px solid %7; border-top-right-radius: 5px; border-bottom-right-radius: 5px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover, QTimeEdit::up-button:hover, QTimeEdit::down-button:hover, QComboBox::drop-down:hover {
            background-color: %5;
        }
        QSpinBox::up-arrow, QTimeEdit::up-arrow { image: url(%12); width: 10px; height: 10px; }
        QSpinBox::down-arrow, QTimeEdit::down-arrow, QComboBox::down-arrow { image: url(%13); width: 10px; height: 10px; }
        QPushButton, QToolButton {
            background-color: %3; color: %8; border: 1px solid %7; border-radius: 6px; padding: 4px 8px; min-height: 22px; font-weight: 500;
        }
        QPushButton:hover, QToolButton:hover { background-color: %5; border-color: %11; }
        QPushButton:pressed, QToolButton:pressed { background-color: %6; border-color: %11; }
        QPushButton:disabled, QToolButton:disabled { background-color: %2; color: %10; border-color: %7; }
        QPushButton#primaryActionBtn, QToolButton#primaryActionBtn, QPushButton#PrimaryButton {
            background-color: %11; color: #FFFFFF; border: 1px solid #1D4ED8; font-weight: 600;
        }
        QPushButton#primaryActionBtn:hover, QToolButton#primaryActionBtn:hover, QPushButton#PrimaryButton:hover {
            background-color: #1D4ED8;
        }
        QScrollBar:vertical { background: %1; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: %7; min-height: 24px; border-radius: 4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal { background: %1; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: %7; min-width: 24px; border-radius: 4px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QStatusBar { background-color: %3; color: %9; border-top: 1px solid %7; padding: 2px 8px; }
        QGroupBox {
            border: 1px solid %7; border-radius: 8px; margin-top: 10px;
            padding-top: 10px; font-weight: 600; color: %8;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }
    )").arg(bgWin, bgPan, bgBar, bgAlt, bgHov, bgSel, border, txtPri, txtSec)
       .arg(txtTec, accent, upArrowUrl, downArrowUrl);
}

} // namespace idm::ui::theme
