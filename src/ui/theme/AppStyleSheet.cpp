#include "ui/theme/AppStyleSheet.h"
#include "ui/theme/ThemeTokens.h"

#include <QPalette>
#include <QStyleFactory>

namespace idm::ui::theme {

void AppStyleSheet::applyToApplication(QApplication& app) {
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
    return QStringLiteral(R"(
        QMainWindow, QDialog {
            background-color: #0B0C0E;
            color: #E6E8EC;
        }
        QWidget {
            color: #E6E8EC;
            font-size: 12px;
            selection-background-color: #1E2638;
            selection-color: #E6E8EC;
        }
        QSplitter::handle {
            background-color: #222630;
        }
        QSplitter::handle:horizontal {
            width: 1px;
        }
        QSplitter::handle:vertical {
            height: 1px;
        }
        QHeaderView {
            background-color: #15181E;
            border: none;
            border-bottom: 1px solid #222630;
        }
        QHeaderView::section {
            background-color: #15181E;
            color: #949BA6;
            font-weight: 600;
            font-size: 11px;
            padding: 4px 8px;
            border: none;
            border-right: 1px solid #222630;
            border-bottom: 1px solid #222630;
        }
        QHeaderView::section:hover {
            background-color: #1A1E26;
            color: #E6E8EC;
        }
        QTableView {
            background-color: #0B0C0E;
            alternate-background-color: #13161B;
            border: none;
            gridline-color: #222630;
            outline: none;
        }
        QTreeWidget {
            background-color: #111317;
            border: none;
            border-right: 1px solid #222630;
            outline: none;
            padding: 4px 0px;
        }
        QTreeWidget::item {
            height: 26px;
            padding: 2px 6px;
            color: #949BA6;
            border-left: 2px solid transparent;
        }
        QTreeWidget::item:hover {
            background-color: #1A1E26;
            color: #E6E8EC;
        }
        QTreeWidget::item:selected {
            background-color: #1E2638;
            color: #E6E8EC;
            border-left: 2px solid #3B82F6;
        }
        QTabWidget::pane {
            background-color: #111317;
            border-top: 1px solid #222630;
        }
        QTabBar::tab {
            background-color: #15181E;
            color: #949BA6;
            padding: 6px 14px;
            border: none;
            border-right: 1px solid #222630;
            border-bottom: 2px solid transparent;
            font-weight: 600;
        }
        QTabBar::tab:hover {
            background-color: #1A1E26;
            color: #E6E8EC;
        }
        QTabBar::tab:selected {
            background-color: #111317;
            color: #E6E8EC;
            border-bottom: 2px solid #3B82F6;
        }
        QLineEdit, QSpinBox, QComboBox, QTimeEdit, QPlainTextEdit {
            background-color: #111317;
            color: #E6E8EC;
            border: 1px solid #222630;
            border-radius: 3px;
            padding: 4px 8px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTimeEdit:focus, QPlainTextEdit:focus {
            border: 1px solid #3B82F6;
        }
        QPushButton, QToolButton {
            background-color: #15181E;
            color: #E6E8EC;
            border: 1px solid #222630;
            border-radius: 3px;
            padding: 4px 10px;
        }
        QPushButton:hover, QToolButton:hover {
            background-color: #1A1E26;
            border-color: #323846;
        }
        QPushButton:pressed, QToolButton:pressed {
            background-color: #1E2638;
            border-color: #3B82F6;
        }
        QPushButton:disabled, QToolButton:disabled {
            background-color: #111317;
            color: #636A75;
            border-color: #1A1D24;
        }
        QPushButton#primaryActionBtn {
            background-color: #3B82F6;
            color: #FFFFFF;
            border: 1px solid #2563EB;
            font-weight: 600;
        }
        QPushButton#primaryActionBtn:hover {
            background-color: #2563EB;
        }
        QScrollBar:vertical {
            background: #0B0C0E;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #222630;
            min-height: 24px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #323846;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: #0B0C0E;
            height: 8px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: #222630;
            min-width: 24px;
            border-radius: 4px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QStatusBar {
            background-color: #15181E;
            color: #949BA6;
            border-top: 1px solid #222630;
        }
        QGroupBox {
            border: 1px solid #222630;
            border-radius: 3px;
            margin-top: 8px;
            padding-top: 8px;
            font-weight: 600;
            color: #949BA6;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px;
        }
    )");
}

} // namespace idm::ui::theme
