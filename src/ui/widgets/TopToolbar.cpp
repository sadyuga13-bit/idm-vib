#include "ui/widgets/TopToolbar.h"
#include "core/Types.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFrame>
#include <QHBoxLayout>

namespace idm::ui::widgets {

using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

TopToolbar::TopToolbar(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(44);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "TopToolbar { background-color: #15181E; border-bottom: 1px solid #222630; }"
    ));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(6);

    m_btnAddUrl = createActionBtn(
        QStringLiteral("Add URL"),
        IconRegistry::icon(IconId::AddUrl, 16),
        this
    );
    m_btnAddUrl->setObjectName(QStringLiteral("primaryActionBtn"));
    m_btnAddUrl->setIcon(QIcon(IconRegistry::pixmapColored(IconId::AddUrl, 16, Qt::white)));

    m_btnBatch = createActionBtn(
        QStringLiteral("Batch"),
        IconRegistry::icon(IconId::BatchSequence, 16),
        this
    );
    m_btnResume = createActionBtn(
        QStringLiteral("Resume"),
        IconRegistry::icon(IconId::StartPlay, 16),
        this
    );
    m_btnPause = createActionBtn(
        QStringLiteral("Pause"),
        IconRegistry::icon(IconId::Pause, 16),
        this
    );
    m_btnStopAll = createActionBtn(
        QStringLiteral("Stop All"),
        IconRegistry::icon(IconId::StopAll, 16),
        this
    );
    m_btnDelete = createActionBtn(
        QStringLiteral("Delete"),
        IconRegistry::icon(IconId::TrashDelete, 16),
        this
    );
    m_btnOpenFolder = createActionBtn(
        QStringLiteral("Location"),
        IconRegistry::icon(IconId::FolderOpen, 16),
        this
    );
    m_btnVerifyHash = createActionBtn(
        QStringLiteral("Verify"),
        IconRegistry::icon(IconId::ShieldVerify, 16),
        this
    );

    layout->addWidget(m_btnAddUrl);
    layout->addWidget(m_btnBatch);
    layout->addWidget(createVerticalSeparator(this));
    layout->addWidget(m_btnResume);
    layout->addWidget(m_btnPause);
    layout->addWidget(m_btnStopAll);
    layout->addWidget(m_btnDelete);
    layout->addWidget(createVerticalSeparator(this));
    layout->addWidget(m_btnOpenFolder);
    layout->addWidget(m_btnVerifyHash);
    layout->addWidget(createVerticalSeparator(this));

    auto* speedIconLbl = new QLabel(this);
    speedIconLbl->setPixmap(IconRegistry::pixmap(IconId::SpeedGauge, 16, IconState::Normal));
    speedIconLbl->setToolTip(QStringLiteral("Global Token-Bucket Rate Limiter (0 = Unlimited)"));
    layout->addWidget(speedIconLbl);

    m_spinSpeedCap = new QSpinBox(this);
    m_spinSpeedCap->setRange(0, 1000000);
    m_spinSpeedCap->setSingleStep(256);
    m_spinSpeedCap->setSpecialValueText(QStringLiteral("Unlimited"));
    m_spinSpeedCap->setSuffix(QStringLiteral(" KB/s"));
    m_spinSpeedCap->setFont(ThemeTokens::monoFont(false));
    m_spinSpeedCap->setFixedWidth(115);
    layout->addWidget(m_spinSpeedCap);

    layout->addStretch(1);

    auto* searchIconLbl = new QLabel(this);
    searchIconLbl->setPixmap(IconRegistry::pixmap(IconId::SearchLens, 16, IconState::Normal));
    layout->addWidget(searchIconLbl);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Filter tasks..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(175);
    layout->addWidget(m_searchEdit);

    layout->addWidget(createVerticalSeparator(this));

    m_lblLiveSpeed = new QLabel(QStringLiteral("0.0 KB/s"), this);
    m_lblLiveSpeed->setFont(ThemeTokens::monoFont(true));
    m_lblLiveSpeed->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblLiveSpeed->setFixedWidth(96);
    m_lblLiveSpeed->setStyleSheet(QStringLiteral("color: #3B82F6; padding-right: 4px;"));
    layout->addWidget(m_lblLiveSpeed);

    layout->addWidget(createVerticalSeparator(this));

    m_licenseBadge = new security::LicenseStatusBadge(this);
    layout->addWidget(m_licenseBadge);

    connect(m_btnAddUrl, &QToolButton::clicked, this, &TopToolbar::addUrlClicked);
    connect(m_btnBatch, &QToolButton::clicked, this, &TopToolbar::batchDownloadClicked);
    connect(m_btnResume, &QToolButton::clicked, this, &TopToolbar::resumeSelectedClicked);
    connect(m_btnPause, &QToolButton::clicked, this, &TopToolbar::pauseSelectedClicked);
    connect(m_btnStopAll, &QToolButton::clicked, this, &TopToolbar::stopAllClicked);
    connect(m_btnDelete, &QToolButton::clicked, this, &TopToolbar::deleteSelectedClicked);
    connect(m_btnOpenFolder, &QToolButton::clicked, this, &TopToolbar::openFolderClicked);
    connect(m_btnVerifyHash, &QToolButton::clicked, this, &TopToolbar::verifyHashClicked);
    connect(m_licenseBadge, &security::LicenseStatusBadge::clicked, this, &TopToolbar::licenseBadgeClicked);
    connect(m_spinSpeedCap, &QSpinBox::valueChanged, this, [this](int val) {
        emit globalSpeedLimitChanged(static_cast<qint64>(val));
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TopToolbar::searchQueryChanged);

    setTaskActionsEnabled(false, false, false);
}

void TopToolbar::setTaskActionsEnabled(bool canStart, bool canPause, bool canDelete) {
    m_btnResume->setEnabled(canStart);
    m_btnPause->setEnabled(canPause);
    m_btnDelete->setEnabled(canDelete);
    m_btnOpenFolder->setEnabled(canDelete);
    m_btnVerifyHash->setEnabled(canDelete);
}

void TopToolbar::updateAggregateSpeed(qint64 bytesPerSec, int activeCount) {
    m_lblLiveSpeed->setText(core::FormatUtils::formatSpeed(bytesPerSec));
    m_lblLiveSpeed->setStyleSheet(
        activeCount > 0
            ? QStringLiteral("color: #3B82F6; padding-right: 4px;")
            : QStringLiteral("color: #636A75; padding-right: 4px;")
    );
}

void TopToolbar::updateLicenseBadge(idm::security::LicenseTier tier, const QString& labelText) {
    m_licenseBadge->updateBadgeState(tier, labelText);
}

QToolButton* TopToolbar::createActionBtn(
    const QString& text,
    const QIcon& icon,
    QWidget* parent
) {
    auto* btn = new QToolButton(parent);
    btn->setText(text);
    btn->setIcon(icon);
    btn->setIconSize(QSize(16, 16));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QWidget* TopToolbar::createVerticalSeparator(QWidget* parent) {
    auto* line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);
    line->setFixedWidth(1);
    line->setFixedHeight(22);
    line->setStyleSheet(QStringLiteral("background-color: #222630; border: none;"));
    return line;
}

} // namespace idm::ui::widgets
