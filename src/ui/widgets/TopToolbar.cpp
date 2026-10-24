#include "ui/widgets/TopToolbar.h"
#include "core/Types.h"
#include "i18n/I18nManager.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QSizePolicy>

namespace idm::ui::widgets {

using i18n::trText;
using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

TopToolbar::TopToolbar(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(48);
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(5);

    m_btnAddUrl = createActionBtn(trText(QStringLiteral("tb.add_url"), QStringLiteral("Add URL")), IconRegistry::icon(IconId::AddUrl, 16), this);
    m_btnAddUrl->setObjectName(QStringLiteral("primaryActionBtn"));
    m_btnAddUrl->setIcon(QIcon(IconRegistry::pixmapColored(IconId::AddUrl, 16, Qt::white)));

    m_btnBatch = createActionBtn(trText(QStringLiteral("tb.batch"), QStringLiteral("Batch")), IconRegistry::icon(IconId::BatchSequence, 16), this);
    m_btnResume = createActionBtn(trText(QStringLiteral("tb.resume"), QStringLiteral("Resume")), IconRegistry::icon(IconId::StartPlay, 16), this);
    m_btnPause = createActionBtn(trText(QStringLiteral("tb.pause"), QStringLiteral("Pause")), IconRegistry::icon(IconId::Pause, 16), this);
    m_btnStopAll = createActionBtn(trText(QStringLiteral("tb.stop_all"), QStringLiteral("Stop All")), IconRegistry::icon(IconId::StopAll, 16), this);
    m_btnDelete = createActionBtn(trText(QStringLiteral("tb.delete"), QStringLiteral("Delete")), IconRegistry::icon(IconId::TrashDelete, 16), this);
    m_btnOpenFolder = createActionBtn(trText(QStringLiteral("tb.location"), QStringLiteral("Location")), IconRegistry::icon(IconId::FolderOpen, 16), this);
    m_btnVerifyHash = createActionBtn(trText(QStringLiteral("tb.verify"), QStringLiteral("Verify")), IconRegistry::icon(IconId::ShieldVerify, 16), this);
    m_btnThemeToggle = createActionBtn(trText(QStringLiteral("tb.day_mode"), QStringLiteral("Day")), IconRegistry::icon(IconId::ThemeSun, 16), this);
    m_btnSettings = createActionBtn(trText(QStringLiteral("tb.settings"), QStringLiteral("Settings")), IconRegistry::icon(IconId::SettingsGear, 16), this);

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
    m_spinSpeedCap->setLocale(QLocale::c());
    m_spinSpeedCap->setRange(0, 1000000);
    m_spinSpeedCap->setSingleStep(256);
    m_spinSpeedCap->setSpecialValueText(QStringLiteral("Unlimited"));
    m_spinSpeedCap->setSuffix(QStringLiteral(" KB/s"));
    m_spinSpeedCap->setFont(ThemeTokens::monoFont(false));
    m_spinSpeedCap->setFixedHeight(32);
    m_spinSpeedCap->setFixedWidth(104);
    layout->addWidget(m_spinSpeedCap);

    layout->addStretch(1);

    auto* searchIconLbl = new QLabel(this);
    searchIconLbl->setPixmap(IconRegistry::pixmap(IconId::SearchLens, 16, IconState::Normal));
    layout->addWidget(searchIconLbl);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(trText(QStringLiteral("tb.filter_placeholder"), QStringLiteral("Filter tasks...")));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(32);
    m_searchEdit->setMinimumWidth(90);
    m_searchEdit->setMaximumWidth(240);
    m_searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_searchEdit, 1);

    layout->addWidget(createVerticalSeparator(this));

    m_lblLiveSpeed = new QLabel(QStringLiteral("0.0 KB/s"), this);
    m_lblLiveSpeed->setFont(ThemeTokens::monoFont(true));
    m_lblLiveSpeed->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblLiveSpeed->setFixedWidth(86);
    m_lblLiveSpeed->setStyleSheet(QStringLiteral("color: #3B82F6; padding-right: 4px;"));
    layout->addWidget(m_lblLiveSpeed);

    layout->addWidget(createVerticalSeparator(this));
    layout->addWidget(m_btnThemeToggle);
    layout->addWidget(m_btnSettings);
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
    connect(m_btnThemeToggle, &QToolButton::clicked, this, &TopToolbar::themeToggleClicked);
    connect(m_btnSettings, &QToolButton::clicked, this, &TopToolbar::settingsClicked);
    connect(m_licenseBadge, &security::LicenseStatusBadge::clicked, this, &TopToolbar::licenseBadgeClicked);
    connect(m_spinSpeedCap, &QSpinBox::valueChanged, this, [this](int val) {
        emit globalSpeedLimitChanged(static_cast<qint64>(val));
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TopToolbar::searchQueryChanged);

    setTaskActionsEnabled(false, false, false);
    refreshThemeIcons();
}

void TopToolbar::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    applyResponsiveButtonMode(event->size().width());
}

void TopToolbar::applyResponsiveButtonMode(int availableWidth) {
    const Qt::ToolButtonStyle primaryStyle =
        (availableWidth >= 820) ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly;
    const Qt::ToolButtonStyle utilityStyle =
        (availableWidth >= 1020) ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly;

    m_btnAddUrl->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_btnResume->setToolButtonStyle(primaryStyle);
    m_btnPause->setToolButtonStyle(primaryStyle);
    m_btnSettings->setToolButtonStyle(primaryStyle);

    m_btnBatch->setToolButtonStyle(utilityStyle);
    m_btnStopAll->setToolButtonStyle(utilityStyle);
    m_btnDelete->setToolButtonStyle(utilityStyle);
    m_btnOpenFolder->setToolButtonStyle(utilityStyle);
    m_btnVerifyHash->setToolButtonStyle(utilityStyle);
    m_btnThemeToggle->setToolButtonStyle(utilityStyle);
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
            : QStringLiteral("color: #64748B; padding-right: 4px;")
    );
}

void TopToolbar::updateLicenseBadge(idm::security::LicenseTier tier, const QString& labelText) {
    m_licenseBadge->updateBadgeState(tier, labelText);
}

void TopToolbar::setSpeedLimitValue(qint64 kibPerSec) {
    m_spinSpeedCap->blockSignals(true);
    m_spinSpeedCap->setValue(static_cast<int>(kibPerSec));
    m_spinSpeedCap->blockSignals(false);
}

void TopToolbar::refreshThemeIcons() {
    auto updateBtnTextAndTip = [](QToolButton* btn, const QString& txt) {
        btn->setText(txt);
        btn->setToolTip(txt);
    };
    updateBtnTextAndTip(m_btnAddUrl, trText(QStringLiteral("tb.add_url"), QStringLiteral("Add URL")));
    updateBtnTextAndTip(m_btnBatch, trText(QStringLiteral("tb.batch"), QStringLiteral("Batch")));
    updateBtnTextAndTip(m_btnResume, trText(QStringLiteral("tb.resume"), QStringLiteral("Resume")));
    updateBtnTextAndTip(m_btnPause, trText(QStringLiteral("tb.pause"), QStringLiteral("Pause")));
    updateBtnTextAndTip(m_btnStopAll, trText(QStringLiteral("tb.stop_all"), QStringLiteral("Stop All")));
    updateBtnTextAndTip(m_btnDelete, trText(QStringLiteral("tb.delete"), QStringLiteral("Delete")));
    updateBtnTextAndTip(m_btnOpenFolder, trText(QStringLiteral("tb.location"), QStringLiteral("Location")));
    updateBtnTextAndTip(m_btnVerifyHash, trText(QStringLiteral("tb.verify"), QStringLiteral("Verify")));
    updateBtnTextAndTip(m_btnSettings, trText(QStringLiteral("tb.settings"), QStringLiteral("Settings")));
    m_searchEdit->setPlaceholderText(trText(QStringLiteral("tb.filter_placeholder"), QStringLiteral("Filter tasks...")));

    m_btnBatch->setIcon(IconRegistry::icon(IconId::BatchSequence, 16));
    m_btnResume->setIcon(IconRegistry::icon(IconId::StartPlay, 16));
    m_btnPause->setIcon(IconRegistry::icon(IconId::Pause, 16));
    m_btnStopAll->setIcon(IconRegistry::icon(IconId::StopAll, 16));
    m_btnDelete->setIcon(IconRegistry::icon(IconId::TrashDelete, 16));
    m_btnOpenFolder->setIcon(IconRegistry::icon(IconId::FolderOpen, 16));
    m_btnVerifyHash->setIcon(IconRegistry::icon(IconId::ShieldVerify, 16));
    m_btnSettings->setIcon(IconRegistry::icon(IconId::SettingsGear, 16));

    const bool day = ThemeTokens::isDayMode();
    m_btnThemeToggle->setText(day ? trText(QStringLiteral("tb.dark_mode"), QStringLiteral("Dark"))
                                  : trText(QStringLiteral("tb.day_mode"), QStringLiteral("Day")));
    m_btnThemeToggle->setIcon(IconRegistry::icon(day ? IconId::ThemeMoon : IconId::ThemeSun, 16));
    m_btnThemeToggle->setToolTip(day ? QStringLiteral("Switch to Dark Engineering Theme")
                                     : QStringLiteral("Switch to Daylight Theme"));
    applyResponsiveButtonMode(width());
}

QToolButton* TopToolbar::createActionBtn(const QString& text, const QIcon& icon, QWidget* parent) {
    auto* btn = new QToolButton(parent);
    btn->setText(text);
    btn->setToolTip(text);
    btn->setIcon(icon);
    btn->setIconSize(QSize(16, 16));
    btn->setFixedHeight(32);
    btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QWidget* TopToolbar::createVerticalSeparator(QWidget* parent) {
    auto* line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);
    line->setFixedWidth(1);
    line->setFixedHeight(20);
    line->setStyleSheet(QStringLiteral("background-color: rgba(148,163,184,45); border: none;"));
    return line;
}

} // namespace idm::ui::widgets
