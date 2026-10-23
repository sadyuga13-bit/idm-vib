#include "ui/security/LicenseDialog.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace idm::ui::security {

using idm::security::ActivationStep;
using idm::security::LicenseCoordinator;
using idm::security::LicenseTier;
using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

LicenseDialog::LicenseDialog(LicenseCoordinator* coordinator, QWidget* parent)
    : QDialog(parent)
    , m_coordinator(coordinator) {
    setWindowTitle(QStringLiteral("Cryptographic Licensing, Hardware Seats & Self-Integrity"));
    setMinimumSize(660, 500);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* banner = new QWidget(this);
    banner->setStyleSheet(QStringLiteral(
        "background-color: #15181E; border: 1px solid #222630; border-radius: 3px;"));
    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(12, 10, 12, 10);

    auto* shieldLbl = new QLabel(banner);
    shieldLbl->setPixmap(IconRegistry::pixmap(IconId::ShieldVerify, 20, IconState::Active));
    bannerLayout->addWidget(shieldLbl);

    auto* bannerTextCol = new QVBoxLayout();
    m_lblHeaderStatus = new QLabel(banner);
    m_lblHeaderStatus->setFont(ThemeTokens::monoFont(true));
    m_lblHeaderHwid = new QLabel(banner);
    m_lblHeaderHwid->setFont(ThemeTokens::monoFont(false));
    m_lblHeaderHwid->setStyleSheet(QStringLiteral("color: #949BA6;"));
    bannerTextCol->addWidget(m_lblHeaderStatus);
    bannerTextCol->addWidget(m_lblHeaderHwid);
    bannerLayout->addLayout(bannerTextCol, 1);
    root->addWidget(banner);

    m_tabs = new QTabWidget(this);
    m_tabs->setIconSize(QSize(15, 15));
    m_tabs->addTab(buildSerialActivationTab(), IconRegistry::icon(IconId::KeySerial, 15),
                   QStringLiteral("Serial Activation"));
    m_tabs->addTab(buildAccountSeatsTab(), IconRegistry::icon(IconId::DeviceLaptop, 15),
                   QStringLiteral("Account & Hardware Seats"));
    m_tabs->addTab(buildOfflineActivationTab(), IconRegistry::icon(IconId::CpuChip, 15),
                   QStringLiteral("Offline Activation"));
    root->addWidget(m_tabs, 1);

    auto* footer = new QHBoxLayout();
    auto* btnDeactivateLocal = new QPushButton(QStringLiteral("Purge Local Vault"), this);
    btnDeactivateLocal->setIcon(IconRegistry::icon(IconId::TrashDelete, 14));
    footer->addWidget(btnDeactivateLocal);
    footer->addStretch(1);
    auto* btnClose = new QPushButton(QStringLiteral("Close"), this);
    footer->addWidget(btnClose);
    root->addLayout(footer);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnDeactivateLocal, &QPushButton::clicked, this, [this]() {
        m_coordinator->deactivateLocalLicense();
        refreshHeaderFromCoordinator();
    });
    connect(m_coordinator, &LicenseCoordinator::activationStepChanged,
            this, &LicenseDialog::onActivationStepChanged);
    connect(m_coordinator, &LicenseCoordinator::licenseStateChanged,
            this, &LicenseDialog::refreshHeaderFromCoordinator);
    connect(m_coordinator, &LicenseCoordinator::seatsUpdated, this,
        [this](bool ok, const QList<idm::security::SeatInfo>& seats, int maxSeats, const QString& msg) {
            if (ok && !seats.isEmpty()) {
                m_seatManager->setSeats(seats, maxSeats);
            }
            m_lblPipelineDetail->setText(msg);
        });

    refreshHeaderFromCoordinator();
}

QWidget* LicenseDialog::buildSerialActivationTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* formGroup = new QGroupBox(QStringLiteral("Product Key & Identity"), page);
    auto* form = new QFormLayout(formGroup);
    form->setSpacing(10);

    m_serialEmailEdit = new QLineEdit(formGroup);
    m_serialEmailEdit->setPlaceholderText(QStringLiteral("engineer@organization.com"));
    form->addRow(QStringLiteral("Licensed Email:"), m_serialEmailEdit);

    m_serialWidget = new SerialKeyInputWidget(formGroup);
    form->addRow(QStringLiteral("25-Char Serial:"), m_serialWidget);
    layout->addWidget(formGroup);

    auto* pipeGroup = new QGroupBox(QStringLiteral("Cryptographic Verification Pipeline"), page);
    auto* pipeLayout = new QVBoxLayout(pipeGroup);
    auto* stepsRow = new QHBoxLayout();

    const QStringList stepTitles = {
        QStringLiteral("1. HKDF Checksum"),
        QStringLiteral("2. HWID 3/4 Quorum"),
        QStringLiteral("3. Ed25519 Verify"),
        QStringLiteral("4. DPAPI + GCM Seal")
    };
    for (int i = 0; i < 4; ++i) {
        auto* lbl = new QLabel(stepTitles.at(i), pipeGroup);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setFont(ThemeTokens::monoFont(true));
        lbl->setStyleSheet(QStringLiteral(
            "background-color: #15181E; color: #636A75; border: 1px solid #222630; padding: 6px;"));
        m_stepLabels[static_cast<size_t>(i)] = lbl;
        stepsRow->addWidget(lbl, 1);
    }
    pipeLayout->addLayout(stepsRow);

    m_lblPipelineDetail = new QLabel(
        QStringLiteral("Enter a 25-character serial key to begin TLS 1.3 SPKI-pinned activation."),
        pipeGroup);
    m_lblPipelineDetail->setFont(ThemeTokens::monoFont(false));
    m_lblPipelineDetail->setStyleSheet(QStringLiteral("color: #949BA6; padding-top: 4px;"));
    pipeLayout->addWidget(m_lblPipelineDetail);
    layout->addWidget(pipeGroup);

    layout->addStretch(1);
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    m_btnActivateOnline = new QPushButton(QStringLiteral("Verify & Activate License"), page);
    m_btnActivateOnline->setObjectName(QStringLiteral("primaryActionBtn"));
    m_btnActivateOnline->setIcon(QIcon(IconRegistry::pixmapColored(IconId::ShieldVerify, 14, Qt::white)));
    btnRow->addWidget(m_btnActivateOnline);
    layout->addLayout(btnRow);

    connect(m_btnActivateOnline, &QPushButton::clicked, this, [this]() {
        m_coordinator->activateWithSerialOnline(m_serialEmailEdit->text(), m_serialWidget->serialKey());
    });
    return page;
}

QWidget* LicenseDialog::buildAccountSeatsTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto* authBox = new QGroupBox(QStringLiteral("Account Authentication (2FA TOTP)"), page);
    auto* authRow = new QHBoxLayout(authBox);
    m_accEmailEdit = new QLineEdit(authBox);
    m_accEmailEdit->setPlaceholderText(QStringLiteral("Account Email"));
    m_accPassEdit = new QLineEdit(authBox);
    m_accPassEdit->setPlaceholderText(QStringLiteral("Password"));
    m_accPassEdit->setEchoMode(QLineEdit::Password);
    m_accTotpEdit = new QLineEdit(authBox);
    m_accTotpEdit->setPlaceholderText(QStringLiteral("6-Digit 2FA"));
    m_accTotpEdit->setMaxLength(6);
    m_accTotpEdit->setFixedWidth(95);
    m_accTotpEdit->setFont(ThemeTokens::monoFont(true));

    auto* btnSignIn = new QPushButton(QStringLiteral("Sync Seats"), authBox);
    btnSignIn->setObjectName(QStringLiteral("primaryActionBtn"));
    authRow->addWidget(m_accEmailEdit, 2);
    authRow->addWidget(m_accPassEdit, 2);
    authRow->addWidget(m_accTotpEdit, 1);
    authRow->addWidget(btnSignIn);
    layout->addWidget(authBox);

    m_seatManager = new SeatManagerWidget(page);
    m_seatManager->populateDefaultLocalSeat(m_coordinator->currentHwid());
    layout->addWidget(m_seatManager, 1);

    connect(btnSignIn, &QPushButton::clicked, this, [this]() {
        m_coordinator->signInAccount(m_accEmailEdit->text(), m_accPassEdit->text(), m_accTotpEdit->text());
    });
    connect(m_seatManager, &SeatManagerWidget::revokeSeatRequested,
            m_coordinator, &LicenseCoordinator::revokeDeviceSeat);
    return page;
}

QWidget* LicenseDialog::buildOfflineActivationTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    const auto& hwid = m_coordinator->currentHwid();
    auto* hwidGroup = new QGroupBox(QStringLiteral("Hardware Quorum Components (3-of-4 Required)"), page);
    auto* form = new QFormLayout(hwidGroup);
    auto makeMonoLbl = [hwidGroup](const QString& txt) {
        auto* l = new QLabel(txt, hwidGroup);
        l->setFont(ThemeTokens::monoFont(false));
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };
    form->addRow(QStringLiteral("SMBIOS Enclosure UUID:"), makeMonoLbl(hwid.smbiosUuidHash));
    form->addRow(QStringLiteral("Processor CPUID Signature:"), makeMonoLbl(hwid.cpuIdHash));
    form->addRow(QStringLiteral("PhysicalDrive0 Serial:"), makeMonoLbl(hwid.diskSerialHash));
    form->addRow(QStringLiteral("OS Cryptography MachineGuid:"), makeMonoLbl(hwid.machineGuidHash));
    layout->addWidget(hwidGroup);

    auto* actionGroup = new QGroupBox(QStringLiteral("Air-Gapped Certificate Exchange"), page);
    auto* actLayout = new QHBoxLayout(actionGroup);
    auto* btnExportReq = new QPushButton(QStringLiteral("1. Export Hardware Request (.hwid_req)"), actionGroup);
    btnExportReq->setIcon(IconRegistry::icon(IconId::CpuChip, 15));
    auto* btnImportLic = new QPushButton(QStringLiteral("2. Import Signed Certificate (.idm_lic)"), actionGroup);
    btnImportLic->setObjectName(QStringLiteral("primaryActionBtn"));
    btnImportLic->setIcon(QIcon(IconRegistry::pixmapColored(IconId::ShieldVerify, 15, Qt::white)));
    actLayout->addWidget(btnExportReq);
    actLayout->addWidget(btnImportLic);
    layout->addWidget(actionGroup);
    layout->addStretch(1);

    connect(btnExportReq, &QPushButton::clicked, this, &LicenseDialog::onExportHwidReq);
    connect(btnImportLic, &QPushButton::clicked, this, &LicenseDialog::onImportOfflineLic);
    return page;
}

void LicenseDialog::onActivationStepChanged(ActivationStep step, const QString& detail) {
    m_lblPipelineDetail->setText(detail);
    const int activeIdx = static_cast<int>(step) - 1;
    for (int i = 0; i < 4; ++i) {
        QString color = QStringLiteral("#636A75");
        QString border = QStringLiteral("#222630");
        if (step == ActivationStep::Completed || i < activeIdx) {
            color = border = QStringLiteral("#10B981");
        } else if (i == activeIdx) {
            color = border = (step == ActivationStep::Failed)
                ? QStringLiteral("#EF4444") : QStringLiteral("#3B82F6");
        }
        m_stepLabels[static_cast<size_t>(i)]->setStyleSheet(
            QStringLiteral("background-color: #15181E; color: %1; border: 1px solid %2; padding: 6px;")
                .arg(color, border));
    }
}

void LicenseDialog::refreshHeaderFromCoordinator() {
    const auto& cert = m_coordinator->activeCertificate();
    const auto& hwid = m_coordinator->currentHwid();
    m_lblHeaderStatus->setText(QStringLiteral("STATUS: %1  |  MAX THREADS: %2  |  PE .TEXT: INTACT")
        .arg(m_coordinator->statusBadgeText())
        .arg(m_coordinator->capabilityToken().resolvePermittedSegments(32)));
    m_lblHeaderStatus->setStyleSheet(
        (cert.tier != LicenseTier::Trial) ? QStringLiteral("color: #10B981;") : QStringLiteral("color: #F59E0B;"));
    m_lblHeaderHwid->setText(QStringLiteral("HWID COMPOSITE: %1  |  QUORUM MATCH: %2/4")
        .arg(hwid.compositeFingerprint()).arg(m_coordinator->hwidMatchedQuorumCount()));
}

void LicenseDialog::onExportHwidReq() {
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Hardware Request"), QStringLiteral("workstation.hwid_req"),
        QStringLiteral("HWID Request (*.hwid_req)"));
    if (!path.isEmpty()) {
        QString err;
        (void)m_coordinator->exportOfflineHwidRequest(
            m_serialEmailEdit->text(), m_serialWidget->serialKey(), path, &err);
    }
}

void LicenseDialog::onImportOfflineLic() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Import Signed License Certificate"), QString(),
        QStringLiteral("IDM Signed License (*.idm_lic)"));
    if (!path.isEmpty()) {
        QString err;
        (void)m_coordinator->importOfflineLicenseFile(path, &err);
        m_tabs->setCurrentIndex(0);
    }
}

} // namespace idm::ui::security
