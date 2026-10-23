#include "ui/settings/ConnectionSettingsPage.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "network/YoutubeExtractor.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace idm::ui::settings {

ConnectionSettingsPage::ConnectionSettingsPage(QWidget* parent)
    : QWidget(parent) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto& cfg = config::AppConfig::instance();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);

    auto* connGroup = new QGroupBox(
        ar ? QStringLiteral("الاتصالات والتوازي والشبكة") : QStringLiteral("Multi-Segment Connections & Network"), this);
    auto* form = new QFormLayout(connGroup);
    form->setSpacing(8);

    m_spinDefaultSegments = new QSpinBox(this);
    m_spinDefaultSegments->setRange(1, 32);
    m_spinDefaultSegments->setValue(cfg.defaultSegments());
    form->addRow(ar ? QStringLiteral("أقصى عدد اتصالات افتراضي لكل ملف (1-32):")
                    : QStringLiteral("Default Max Connections per File (1-32):"), m_spinDefaultSegments);

    m_spinMaxConcurrent = new QSpinBox(this);
    m_spinMaxConcurrent->setRange(1, 16);
    m_spinMaxConcurrent->setValue(cfg.maxConcurrentDownloads());
    form->addRow(ar ? QStringLiteral("أقصى عدد ملفات تحمل بالتزامن:")
                    : QStringLiteral("Max Concurrent Active Downloads:"), m_spinMaxConcurrent);

    m_spinSpeedLimitKiB = new QSpinBox(this);
    m_spinSpeedLimitKiB->setRange(0, 1000000);
    m_spinSpeedLimitKiB->setSingleStep(128);
    m_spinSpeedLimitKiB->setSuffix(QStringLiteral(" KiB/s (0 = Unlimited)"));
    m_spinSpeedLimitKiB->setValue(static_cast<int>(cfg.globalSpeedLimitKiB()));
    form->addRow(ar ? QStringLiteral("محدد السرعة الكلي:")
                    : QStringLiteral("Global Speed Limit:"), m_spinSpeedLimitKiB);

    auto* netRow = new QHBoxLayout();
    m_spinTimeoutSec = new QSpinBox(this);
    m_spinTimeoutSec->setRange(5, 300);
    m_spinTimeoutSec->setSuffix(QStringLiteral(" s"));
    m_spinTimeoutSec->setValue(cfg.connectionTimeoutSec());

    m_spinRetryDelaySec = new QSpinBox(this);
    m_spinRetryDelaySec->setRange(1, 120);
    m_spinRetryDelaySec->setSuffix(QStringLiteral(" s"));
    m_spinRetryDelaySec->setValue(cfg.retryDelaySec());

    m_spinMaxRetries = new QSpinBox(this);
    m_spinMaxRetries->setRange(0, 99);
    m_spinMaxRetries->setValue(cfg.maxRetries());

    m_spinIpcPort = new QSpinBox(this);
    m_spinIpcPort->setRange(1024, 65535);
    m_spinIpcPort->setValue(cfg.ipcPort());

    netRow->addWidget(new QLabel(ar ? QStringLiteral("المهلة:") : QStringLiteral("Timeout:"), this));
    netRow->addWidget(m_spinTimeoutSec);
    netRow->addWidget(new QLabel(ar ? QStringLiteral("تأخير الإعادة:") : QStringLiteral("Retry Delay:"), this));
    netRow->addWidget(m_spinRetryDelaySec);
    netRow->addWidget(new QLabel(ar ? QStringLiteral("المحاولات:") : QStringLiteral("Retries:"), this));
    netRow->addWidget(m_spinMaxRetries);
    netRow->addWidget(new QLabel(QStringLiteral("IPC Port:"), this));
    netRow->addWidget(m_spinIpcPort);
    form->addRow(ar ? QStringLiteral("معاملات إعادة المحاولة والمنفذ:") : QStringLiteral("Retry & Extension Port:"), netRow);

    auto* uaRow = new QHBoxLayout();
    m_uaPresetCombo = new QComboBox(this);
    m_uaPresetCombo->addItem(QStringLiteral("Chrome 134 Windows"),
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36"));
    m_uaPresetCombo->addItem(QStringLiteral("Edge 134 Windows"),
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36 Edg/134.0.0.0"));
    m_uaPresetCombo->addItem(QStringLiteral("Firefox 135 Windows"),
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:135.0) Gecko/20100101 Firefox/135.0"));
    m_uaEdit = new QLineEdit(cfg.userAgent(), this);
    connect(m_uaPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_uaEdit->setText(m_uaPresetCombo->itemData(idx).toString());
    });
    uaRow->addWidget(m_uaPresetCombo);
    uaRow->addWidget(m_uaEdit, 1);
    form->addRow(ar ? QStringLiteral("هوية المتصفح (User-Agent):") : QStringLiteral("HTTP User-Agent:"), uaRow);
    root->addWidget(connGroup);

    auto* capGroup = new QGroupBox(
        ar ? QStringLiteral("سقف الاتصالات المخصص لكل سيرفر (Per-Server Connection Cap)")
           : QStringLiteral("Per-Domain / Server Connection Cap"), this);
    auto* capLayout = new QVBoxLayout(capGroup);

    auto* addCapRow = new QHBoxLayout();
    m_domainCapHostEdit = new QLineEdit(this);
    m_domainCapHostEdit->setPlaceholderText(QStringLiteral("example.com"));
    m_domainCapSpin = new QSpinBox(this);
    m_domainCapSpin->setRange(1, 32);
    m_domainCapSpin->setValue(4);
    auto* btnAddCap = new QPushButton(ar ? QStringLiteral("إضافة / تحديث") : QStringLiteral("Add / Update"), this);
    auto* btnRemCap = new QPushButton(ar ? QStringLiteral("حذف المحدد") : QStringLiteral("Remove Selected"), this);
    connect(btnAddCap, &QPushButton::clicked, this, &ConnectionSettingsPage::onAddDomainCapClicked);
    connect(btnRemCap, &QPushButton::clicked, this, &ConnectionSettingsPage::onRemoveDomainCapClicked);
    addCapRow->addWidget(m_domainCapHostEdit, 1);
    addCapRow->addWidget(m_domainCapSpin);
    addCapRow->addWidget(btnAddCap);
    addCapRow->addWidget(btnRemCap);
    capLayout->addLayout(addCapRow);

    m_domainCapsTable = new QTableWidget(0, 2, this);
    m_domainCapsTable->setHorizontalHeaderLabels({
        ar ? QStringLiteral("النطاق / السيرفر") : QStringLiteral("Server Host / Domain"),
        ar ? QStringLiteral("أقصى اتصالات") : QStringLiteral("Max Connections")
    });
    m_domainCapsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_domainCapsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_domainCapsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_domainCapsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_domainCapsTable->setMaximumHeight(110);
    capLayout->addWidget(m_domainCapsTable);
    populateDomainCapsTable();
    root->addWidget(capGroup);

    auto* ytGroup = new QGroupBox(
        ar ? QStringLiteral("محرك يوتيوب والوسائط (Innertube + yt-dlp)")
           : QStringLiteral("YouTube & Media Extractor Engine"), this);
    auto* ytRow = new QHBoxLayout(ytGroup);
    const QString ytPath = network::YoutubeExtractor::locateYtDlpBinary();
    const bool hasYt = !ytPath.isEmpty() && QFileInfo::exists(ytPath);
    m_lblYtDlpStatus = new QLabel(
        hasYt ? (ar ? QStringLiteral("المكون المساعد yt-dlp.exe مثبت وجاهز")
                    : QStringLiteral("yt-dlp.exe is installed and ready"))
              : (ar ? QStringLiteral("يعمل بمحرك Innertube المدمج (يمكن تنزيل yt-dlp لدعم 4K)")
                    : QStringLiteral("Built-in Innertube active (install yt-dlp for full 4K support)")),
        this);
    m_btnUpdateYtDlp = new QPushButton(
        ar ? QStringLiteral("تنزيل / تحديث yt-dlp.exe") : QStringLiteral("Install / Update yt-dlp.exe"), this);
    connect(m_btnUpdateYtDlp, &QPushButton::clicked, this, &ConnectionSettingsPage::onUpdateYtDlpClicked);
    connect(&m_ytExtractor, &network::YoutubeExtractor::ytDlpDownloadFinished, this, [this](bool ok, const QString& msg) {
        m_btnUpdateYtDlp->setEnabled(true);
        m_lblYtDlpStatus->setText(ok ? QStringLiteral("yt-dlp.exe Ready: ") + msg : msg);
    });
    ytRow->addWidget(m_lblYtDlpStatus, 1);
    ytRow->addWidget(m_btnUpdateYtDlp);
    root->addWidget(ytGroup);

    root->addStretch(1);
}

void ConnectionSettingsPage::populateDomainCapsTable() {
    m_domainCapsTable->setRowCount(0);
    const auto caps = config::AppConfig::instance().domainConnectionCaps();
    for (auto it = caps.cbegin(); it != caps.cend(); ++it) {
        const int r = m_domainCapsTable->rowCount();
        m_domainCapsTable->insertRow(r);
        m_domainCapsTable->setItem(r, 0, new QTableWidgetItem(it.key()));
        m_domainCapsTable->setItem(r, 1, new QTableWidgetItem(QString::number(it.value())));
    }
}

void ConnectionSettingsPage::onAddDomainCapClicked() {
    const QString host = m_domainCapHostEdit->text().trimmed().toLower();
    if (host.isEmpty()) return;
    for (int r = 0; r < m_domainCapsTable->rowCount(); ++r) {
        if (m_domainCapsTable->item(r, 0)->text() == host) {
            m_domainCapsTable->item(r, 1)->setText(QString::number(m_domainCapSpin->value()));
            m_domainCapHostEdit->clear();
            return;
        }
    }
    const int r = m_domainCapsTable->rowCount();
    m_domainCapsTable->insertRow(r);
    m_domainCapsTable->setItem(r, 0, new QTableWidgetItem(host));
    m_domainCapsTable->setItem(r, 1, new QTableWidgetItem(QString::number(m_domainCapSpin->value())));
    m_domainCapHostEdit->clear();
}

void ConnectionSettingsPage::onRemoveDomainCapClicked() {
    const int row = m_domainCapsTable->currentRow();
    if (row >= 0) m_domainCapsTable->removeRow(row);
}

void ConnectionSettingsPage::onUpdateYtDlpClicked() {
    m_btnUpdateYtDlp->setEnabled(false);
    m_lblYtDlpStatus->setText(QStringLiteral("Downloading yt-dlp.exe from GitHub..."));
    const QString dest = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("yt-dlp.exe"));
    m_ytExtractor.downloadYtDlpBinary(dest);
}

void ConnectionSettingsPage::applyToConfig() {
    auto& cfg = config::AppConfig::instance();
    cfg.setDefaultSegments(m_spinDefaultSegments->value());
    cfg.setMaxConcurrentDownloads(m_spinMaxConcurrent->value());
    cfg.setGlobalSpeedLimitKiB(m_spinSpeedLimitKiB->value());
    cfg.setConnectionTimeoutSec(m_spinTimeoutSec->value());
    cfg.setRetryDelaySec(m_spinRetryDelaySec->value());
    cfg.setMaxRetries(m_spinMaxRetries->value());
    cfg.setIpcPort(static_cast<quint16>(m_spinIpcPort->value()));
    cfg.setUserAgent(m_uaEdit->text().trimmed());

    QMap<QString, int> caps;
    for (int r = 0; r < m_domainCapsTable->rowCount(); ++r) {
        caps.insert(m_domainCapsTable->item(r, 0)->text(),
                    m_domainCapsTable->item(r, 1)->text().toInt());
    }
    cfg.setDomainConnectionCaps(caps);
}

} // namespace idm::ui::settings
