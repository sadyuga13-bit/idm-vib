#include "ui/security/SerialKeyInputWidget.h"
#include "security/CryptoEngine.h"
#include "ui/theme/ThemeTokens.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QRegularExpression>

namespace idm::ui::security {

using idm::security::CryptoEngine;
using theme::ThemeTokens;

SerialKeyInputWidget::SerialKeyInputWidget(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    for (int i = 0; i < 5; ++i) {
        auto* edit = new QLineEdit(this);
        edit->setMaxLength(5);
        edit->setAlignment(Qt::AlignCenter);
        edit->setFont(ThemeTokens::monoFont(true));
        edit->setPlaceholderText(QStringLiteral("XXXXX"));
        edit->setFixedHeight(32);
        edit->setMinimumWidth(78);
        edit->installEventFilter(this);

        m_boxes[static_cast<size_t>(i)] = edit;
        layout->addWidget(edit, 1);

        if (i < 4) {
            auto* dash = new QLabel(QStringLiteral("-"), this);
            dash->setFont(ThemeTokens::monoFont(true));
            dash->setStyleSheet(QStringLiteral("color: #636A75;"));
            layout->addWidget(dash);
        }

        connect(edit, &QLineEdit::textEdited, this, [this, i](const QString& txt) {
            onSegmentEdited(i, txt);
        });
    }

    updateVisualValidationState();
}

QString SerialKeyInputWidget::serialKey() const {
    QStringList parts;
    parts.reserve(5);
    for (const QLineEdit* box : m_boxes) {
        parts.append(box->text().trimmed().toUpper());
    }
    return parts.join(QLatin1Char('-'));
}

void SerialKeyInputWidget::setSerialKey(const QString& fullKey) {
    handlePasteText(fullKey);
}

void SerialKeyInputWidget::clear() {
    m_updatingProgrammatically = true;
    for (QLineEdit* box : m_boxes) {
        box->clear();
    }
    m_updatingProgrammatically = false;
    updateVisualValidationState();
}

bool SerialKeyInputWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Paste)) {
            if (const QClipboard* cb = QGuiApplication::clipboard()) {
                handlePasteText(cb->text());
                return true;
            }
        }
        if (keyEvent->key() == Qt::Key_Backspace) {
            for (int i = 1; i < 5; ++i) {
                if (watched == m_boxes[static_cast<size_t>(i)] &&
                    m_boxes[static_cast<size_t>(i)]->text().isEmpty()) {
                    m_boxes[static_cast<size_t>(i - 1)]->setFocus();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SerialKeyInputWidget::onSegmentEdited(int index, const QString& text) {
    if (m_updatingProgrammatically) {
        return;
    }

    QString cleaned = text.toUpper();
    static const QRegularExpression kStripRx(QStringLiteral("[^2-9A-HJ-NP-Z]"));
    cleaned.remove(kStripRx);

    m_updatingProgrammatically = true;
    m_boxes[static_cast<size_t>(index)]->setText(cleaned);
    m_updatingProgrammatically = false;

    if (cleaned.size() == 5 && index < 4) {
        m_boxes[static_cast<size_t>(index + 1)]->setFocus();
        m_boxes[static_cast<size_t>(index + 1)]->selectAll();
    }

    updateVisualValidationState();
}

void SerialKeyInputWidget::handlePasteText(const QString& rawClipboard) {
    QString clean = rawClipboard.trimmed().toUpper();
    static const QRegularExpression kStripRx(QStringLiteral("[^2-9A-HJ-NP-Z]"));
    clean.remove(kStripRx);

    m_updatingProgrammatically = true;
    for (int i = 0; i < 5; ++i) {
        const QString slice = clean.mid(i * 5, 5);
        m_boxes[static_cast<size_t>(i)]->setText(slice);
    }
    m_updatingProgrammatically = false;
    updateVisualValidationState();
}

void SerialKeyInputWidget::updateVisualValidationState() {
    int totalChars = 0;
    for (const QLineEdit* box : m_boxes) {
        totalChars += static_cast<int>(box->text().size());
    }

    const QString full = serialKey();
    m_checksumValid = (totalChars == 25) && CryptoEngine::validateSerialKeyChecksum(full);

    QString borderColor = ThemeTokens::borderHairline().name();
    if (totalChars == 25) {
        borderColor = m_checksumValid
                          ? QStringLiteral("#10B981")
                          : QStringLiteral("#EF4444");
    }

    const QString style = QStringLiteral(
        "QLineEdit { border: 1px solid %1; border-radius: 3px; padding: 4px; }"
        "QLineEdit:focus { border: 1px solid #3B82F6; }"
    ).arg(borderColor);

    for (QLineEdit* box : m_boxes) {
        box->setStyleSheet(style);
    }

    emit serialChanged(full, m_checksumValid);
}

} // namespace idm::ui::security
