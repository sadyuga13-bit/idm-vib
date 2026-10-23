#include "ui/security/LicenseStatusBadge.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

namespace idm::ui::security {

using idm::security::LicenseTier;
using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

LicenseStatusBadge::LicenseStatusBadge(QWidget* parent)
    : QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(26);
    setToolTip(QStringLiteral("Click to manage cryptographic license, hardware seats & offline activation"));
}

void LicenseStatusBadge::updateBadgeState(LicenseTier tier, const QString& labelText) {
    m_tier = tier;
    m_labelText = labelText;
    updateGeometry();
    update();
}

QSize LicenseStatusBadge::sizeHint() const {
    const QFontMetrics fm(ThemeTokens::monoFont(true));
    const int textW = fm.horizontalAdvance(m_labelText);
    return QSize(textW + 36, 26);
}

void LicenseStatusBadge::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool isVerified = (m_tier == LicenseTier::Pro || m_tier == LicenseTier::Enterprise);
    const QColor accent = isVerified ? ThemeTokens::statusDone() : ThemeTokens::statusPaused();
    QColor fillBg = isVerified ? QColor(0x10, 0xB9, 0x81, m_hovered ? 38 : 22)
                               : QColor(0xF5, 0x9E, 0x0B, m_hovered ? 38 : 22);

    const QRectF bounds = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    painter.setPen(QPen(accent, 1.0));
    painter.setBrush(fillBg);
    painter.drawRoundedRect(bounds, 3.0, 3.0);

    const IconId iconId = isVerified ? IconId::ShieldVerify : IconId::LockClosed;
    const IconState icoState = isVerified ? IconState::Success : IconState::Warning;
    const QPixmap ico = IconRegistry::pixmap(iconId, 14, icoState);

    const int iconY = (height() - 14) / 2;
    painter.drawPixmap(8, iconY, 14, 14, ico);

    painter.setFont(ThemeTokens::monoFont(true));
    painter.setPen(accent);
    const QRect textRect(26, 0, width() - 32, height());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_labelText);
}

void LicenseStatusBadge::enterEvent(QEnterEvent*) {
    m_hovered = true;
    update();
}

void LicenseStatusBadge::leaveEvent(QEvent*) {
    m_hovered = false;
    update();
}

void LicenseStatusBadge::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

} // namespace idm::ui::security
