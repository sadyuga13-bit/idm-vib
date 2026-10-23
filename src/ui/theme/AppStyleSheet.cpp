#include "ui/theme/AppStyleSheet.h"
#include "core/AppSettings.h"
#include "ui/theme/ThemeTokens.h"

#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

namespace idm::ui::theme {

void AppStyleSheet::setDayMode(bool dayMode) {
    ThemeTokens::setDayMode(dayMode);
    core::AppSettings::instance().setDayMode(dayMode);
    core::AppSettings::instance().sync();
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        applyToApplication(*app);
        for (QWidget* w : QApplication::allWidgets()) {
            w->update();
        }
    }
}

void AppStyleSheet::applyToApplication(QApplication& app) {
    ThemeTokens::setDayMode(core::AppSettings::instance().isDayMode());
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

    return QStringLiteral(R"(
        QMainWindow, QDialog { background-color: %1; color: %8; }
        QWidget { color: %8; font-size: 12px; selection-background-color: %6; selection-color: %8; }
        TopToolbar { background-color: %3; border-bottom: 1px solid %7; }
        SidebarTree { background-color: %2; border-right: 1px solid %7; }
        QSplitter::handle { background-color: %7; }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical { height: 1px; }
        QHeaderView { background-color: %3; border: none; border-bottom: 1px solid %7; }
        QHeaderView::section {
            background-color: %3; color: %9; font-weight: 600; font-size: 11px;
            padding: 4px 8px; border: none; border-right: 1px solid %7; border-bottom: 1px solid %7;
        }
        QHeaderView::section:hover { background-color: %5; color: %8; }
        QTableView, QTableWidget {
            background-color: %1; alternate-background-color: %4;
            border: none; gridline-color: %7; outline: none;
        }
        QTreeWidget { background-color: %2; border: none; border-right: 1px solid %7; outline: none; padding: 4px 0px; }
        QTreeWidget::item { height: 26px; padding: 2px 6px; color: %9; border-left: 2px solid transparent; }
        QTreeWidget::item:hover { background-color: %5; color: %8; }
        QTreeWidget::item:selected { background-color: %6; color: %8; border-left: 2px solid %11; }
        QTabWidget::pane { background-color: %2; border-top: 1px solid %7; }
        QTabBar::tab {
            background-color: %3; color: %9; padding: 6px 14px; border: none;
            border-right: 1px solid %7; border-bottom: 2px solid transparent; font-weight: 600;
        }
        QTabBar::tab:hover { background-color: %5; color: %8; }
        QTabBar::tab:selected { background-color: %2; color: %8; border-bottom: 2px solid %11; }
        QLineEdit, QSpinBox, QComboBox, QTimeEdit, QPlainTextEdit {
            background-color: %2; color: %8; border: 1px solid %7; border-radius: 3px; padding: 4px 8px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTimeEdit:focus, QPlainTextEdit:focus {
            border: 1px solid %11;
        }
        QPushButton, QToolButton {
            background-color: %3; color: %8; border: 1px solid %7; border-radius: 3px; padding: 4px 10px;
        }
        QPushButton:hover, QToolButton:hover { background-color: %5; border-color: %9; }
        QPushButton:pressed, QToolButton:pressed { background-color: %6; border-color: %11; }
        QPushButton:disabled, QToolButton:disabled { background-color: %2; color: %10; border-color: %7; }
        QPushButton#primaryActionBtn, QToolButton#primaryActionBtn {
            background-color: %11; color: #FFFFFF; border: 1px solid #1D4ED8; font-weight: 600;
        }
        QPushButton#primaryActionBtn:hover, QToolButton#primaryActionBtn:hover { background-color: #1D4ED8; }
        QScrollBar:vertical { background: %1; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: %7; min-height: 24px; border-radius: 4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal { background: %1; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: %7; min-width: 24px; border-radius: 4px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QStatusBar { background-color: %3; color: %9; border-top: 1px solid %7; }
        QGroupBox {
            border: 1px solid %7; border-radius: 3px; margin-top: 8px;
            padding-top: 8px; font-weight: 600; color: %9;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
    )").arg(bgWin, bgPan, bgBar, bgAlt, bgHov, bgSel, border, txtPri, txtSec)
       .arg(txtTec, accent);
}

} // namespace idm::ui::theme
