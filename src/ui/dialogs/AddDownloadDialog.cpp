#include "ui/dialogs/AddDownloadDialog.h"
#include "core/AppSettings.h"
#include "i18n/I18nManager.h"
#include "storage/CategoryRouter.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace idm::ui::dialogs {

using core::AppSettings;
using storage::CategoryRouter;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

AddDownloadDialog::AddDownloadDialog(
    const QString& initialUrl, const network::RequestProfile& initialProfile,
    const QString& suggestedFileName, QWidget* parent) : QDialog(parent) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    setWindowTitle(ar ? QStringLiteral("إضافة رابط تحميل جديد / فيديو يوتيوب") : QStringLiteral("Add New Download / YouTube Stream"));
    setMinimumWidth(520);
    resize(620, 500);

    const AppSettings& cfg = AppSettings::instance();
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto* targetBox = new QGroupBox(ar ? QStringLiteral("الرابط وإعدادات الملف") : QStringLiteral("Endpoint & Target Configuration"), this);
    auto* targetForm = new QFormLayout(targetBox);
    targetForm->setSpacing(8);
    targetForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto* urlRow = new QHBoxLayout();
    m_urlEdit = new QLineEdit(initialUrl, targetBox);
    m_urlEdit->setPlaceholderText(QStringLiteral("https://www.youtube.com/watch?v=... or direct file URL"));
    m_btnProbe = new QPushButton(ar ? QStringLiteral("فحص واستخراج") : QStringLiteral("Probe / Extract"), targetBox);
    m_btnProbe->setIcon(IconRegistry::icon(IconId::NetworkGlobe, 14));
    urlRow->addWidget(m_urlEdit, 1);
    urlRow->addWidget(m_btnProbe);
    targetForm->addRow(ar ? QStringLiteral("الرابط:") : QStringLiteral("URL:"), urlRow);

    m_lblProbeStatus = new QLabel(ar ? QStringLiteral("جاهز لفحص السيرفر أو استخراج جودات يوتيوب.")
                                     : QStringLiteral("Ready to probe remote server or YouTube Innertube API."), targetBox);
    m_lblProbeStatus->setFont(ThemeTokens::monoFont(false));
    m_lblProbeStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_lblProbeStatus->setMinimumWidth(0);
    targetForm->addRow(ar ? QStringLiteral("حالة الفحص:") : QStringLiteral("Stream Probe:"), m_lblProbeStatus);

    m_ytQualityCombo = new QComboBox(targetBox);
    m_ytQualityCombo->setEnabled(false);
    m_ytQualityCombo->addItem(ar ? QStringLiteral("ملف مباشر قياسي (اكتشاف تلقائي)") : QStringLiteral("Standard Direct HTTP File (Auto-Detect)"));
    targetForm->addRow(ar ? QStringLiteral("الجودة والصيغة:") : QStringLiteral("Media Stream:"), m_ytQualityCombo);

    m_fileNameEdit = new QLineEdit(suggestedFileName, targetBox);
    targetForm->addRow(ar ? QStringLiteral("حفظ باسم:") : QStringLiteral("Save File As:"), m_fileNameEdit);

    auto* dirRow = new QHBoxLayout();
    m_saveDirEdit = new QLineEdit(cfg.defaultSaveDirectory(), targetBox);
    auto* btnBrowse = new QPushButton(ar ? QStringLiteral("استعراض") : QStringLiteral("Browse"), targetBox);
    btnBrowse->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));
    dirRow->addWidget(m_saveDirEdit, 1);
    dirRow->addWidget(btnBrowse);
    targetForm->addRow(ar ? QStringLiteral("مجلد الحفظ:") : QStringLiteral("Directory:"), dirRow);

    auto* optsRow = new QHBoxLayout();
    m_queueCombo = new QComboBox(targetBox);
    m_queueCombo->addItems({QStringLiteral("Main Queue"), QStringLiteral("Night Queue")});
    m_segmentsSpin = new QSpinBox(targetBox);
    m_segmentsSpin->setRange(1, 32);
    m_segmentsSpin->setValue(cfg.defaultSegments());
    m_segmentsSpin->setSuffix(QStringLiteral(" threads"));
    optsRow->addWidget(m_queueCombo, 1);
    optsRow->addWidget(new QLabel(ar ? QStringLiteral("الاتصالات:") : QStringLiteral("Connections:"), targetBox));
    optsRow->addWidget(m_segmentsSpin);
    targetForm->addRow(ar ? QStringLiteral("الطابور والتقسيم:") : QStringLiteral("Queue & Split:"), optsRow);

    m_hashEdit = new QLineEdit(targetBox);
    m_hashEdit->setPlaceholderText(ar ? QStringLiteral("اختياري: بصمة SHA-256 أو MD5 للتحقق") : QStringLiteral("Optional expected SHA-256 or MD5 hex digest"));
    m_hashEdit->setFont(ThemeTokens::monoFont(false));
    targetForm->addRow(ar ? QStringLiteral("البصمة الرقمية:") : QStringLiteral("Checksum:"), m_hashEdit);
    rootLayout->addWidget(targetBox);

    auto* advBox = new QGroupBox(ar ? QStringLiteral("الترويسات والكوكيز والمصادقة والبروكسي") : QStringLiteral("Headers, Cookies, Auth & Proxy"), this);
    auto* advForm = new QFormLayout(advBox);
    advForm->setSpacing(8);

    m_refererEdit = new QLineEdit(initialProfile.referer, advBox);
    m_cookieEdit = new QLineEdit(initialProfile.cookies, advBox);
    const QString initUa = initialProfile.userAgent.isEmpty() ? cfg.customUserAgent() : initialProfile.userAgent;
    m_userAgentEdit = new QLineEdit(initUa, advBox);
    advForm->addRow(QStringLiteral("Referer:"), m_refererEdit);
    advForm->addRow(QStringLiteral("Cookie:"), m_cookieEdit);
    advForm->addRow(QStringLiteral("User-Agent:"), m_userAgentEdit);

    auto* authRow = new QHBoxLayout();
    m_authUserEdit = new QLineEdit(initialProfile.authUser, advBox);
    m_authUserEdit->setPlaceholderText(ar ? QStringLiteral("اسم المستخدم") : QStringLiteral("HTTP Basic Username"));
    m_authPassEdit = new QLineEdit(initialProfile.authPassword, advBox);
    m_authPassEdit->setPlaceholderText(ar ? QStringLiteral("كلمة المرور") : QStringLiteral("Password"));
    m_authPassEdit->setEchoMode(QLineEdit::Password);
    authRow->addWidget(m_authUserEdit);
    authRow->addWidget(m_authPassEdit);
    advForm->addRow(ar ? QStringLiteral("مصادقة HTTP:") : QStringLiteral("Basic Auth:"), authRow);

    auto* proxyRow = new QHBoxLayout();
    m_proxyCheck = new QCheckBox(ar ? QStringLiteral("تفعيل البروكسي") : QStringLiteral("Enable HTTP Proxy"), advBox);
    m_proxyCheck->setChecked(initialProfile.proxyEnabled || cfg.proxyEnabled());
    m_proxyHostEdit = new QLineEdit(initialProfile.proxyHost.isEmpty() ? cfg.proxyHost() : initialProfile.proxyHost, advBox);
    m_proxyPortSpin = new QSpinBox(advBox);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(initialProfile.proxyPort > 0 ? initialProfile.proxyPort : cfg.proxyPort());
    proxyRow->addWidget(m_proxyCheck);
    proxyRow->addWidget(m_proxyHostEdit, 1);
    proxyRow->addWidget(m_proxyPortSpin);
    advForm->addRow(ar ? QStringLiteral("البروكسي:") : QStringLiteral("Proxy:"), proxyRow);
    rootLayout->addWidget(advBox);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto* btnCancel = new QPushButton(ar ? QStringLiteral("إلغاء") : QStringLiteral("Cancel"), this);
    auto* btnQueueLater = new QPushButton(ar ? QStringLiteral("إضافة للطابور") : QStringLiteral("Add to Queue"), this);
    btnQueueLater->setIcon(IconRegistry::icon(IconId::SchedulerClock, 14));
    auto* btnStartNow = new QPushButton(ar ? QStringLiteral("بدء التحميل الآن") : QStringLiteral("Download Now"), this);
    btnStartNow->setObjectName(QStringLiteral("primaryActionBtn"));
    btnStartNow->setIcon(QIcon(IconRegistry::pixmapColored(IconId::StartPlay, 14, Qt::white)));
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnQueueLater);
    btnRow->addWidget(btnStartNow);
    rootLayout->addLayout(btnRow);

    connect(&m_prober, &network::UrlProber::probeCompleted, this, &AddDownloadDialog::onProbeCompleted);
    connect(&m_ytExtractor, &network::YoutubeExtractor::extractionCompleted, this, &AddDownloadDialog::onYoutubeExtracted);
    connect(m_ytQualityCombo, &QComboBox::currentIndexChanged, this, &AddDownloadDialog::onYoutubeQualityChanged);
    connect(m_btnProbe, &QPushButton::clicked, this, &AddDownloadDialog::triggerProbe);
    connect(btnBrowse, &QPushButton::clicked, this, &AddDownloadDialog::onBrowseFolder);
    connect(m_fileNameEdit, &QLineEdit::textChanged, this, [this](const QString& name) {
        if (!name.trimmed().isEmpty()) { m_saveDirEdit->setText(CategoryRouter::defaultDirectoryForFile(name)); }
    });
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnQueueLater, &QPushButton::clicked, this, [this]() {
        m_startImmediately = false;
        if (!m_urlEdit->text().trimmed().isEmpty()) { accept(); }
    });
    connect(btnStartNow, &QPushButton::clicked, this, [this]() {
        m_startImmediately = true;
        if (!m_urlEdit->text().trimmed().isEmpty()) { accept(); }
    });
    if (!initialUrl.trimmed().isEmpty()) {
        QTimer::singleShot(15, this, &AddDownloadDialog::triggerProbe);
    }
}

void AddDownloadDialog::triggerProbe() {
    m_resolvedDirectUrl.clear();
    const QUrl url(m_urlEdit->text().trimmed());
    if (!url.isValid() || url.scheme().isEmpty()) {
        m_lblProbeStatus->setText(QStringLiteral("Enter a valid HTTP/HTTPS or YouTube URL."));
        m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #EF4444;"));
        return;
    }

    if (network::YoutubeExtractor::isYoutubeUrl(url)) {
        m_lblProbeStatus->setText(QStringLiteral("Extracting YouTube streams via Innertube ANDROID client..."));
        m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #3B82F6;"));
        m_ytExtractor.extractVideoInfo(url);
        return;
    }

    m_ytQualityCombo->setEnabled(false);
    m_lblProbeStatus->setText(QStringLiteral("Probing server headers..."));
    m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #3B82F6;"));
    m_prober.probeUrl(url, configuredProfile());
}

void AddDownloadDialog::onYoutubeExtracted(const network::YoutubeVideoInfo& info) {
    m_ytInfo = info;
    m_ytQualityCombo->blockSignals(true);
    m_ytQualityCombo->clear();

    if (!info.success || info.formats.isEmpty()) {
        m_ytQualityCombo->addItem(QStringLiteral("Extraction failed"));
        m_ytQualityCombo->setEnabled(false);
        m_ytQualityCombo->blockSignals(false);
        m_lblProbeStatus->setText(QStringLiteral("YouTube Error: %1").arg(info.errorMessage));
        m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #EF4444;"));
        return;
    }

    for (const network::YoutubeFormat& fmt : info.formats) {
        m_ytQualityCombo->addItem(fmt.displayLabel);
    }
    m_ytQualityCombo->setEnabled(true);
    m_ytQualityCombo->setCurrentIndex(0);
    m_ytQualityCombo->blockSignals(false);
    onYoutubeQualityChanged(0);
}

void AddDownloadDialog::onYoutubeQualityChanged(int index) {
    if (index < 0 || index >= m_ytInfo.formats.size()) {
        return;
    }
    const network::YoutubeFormat& fmt = m_ytInfo.formats.at(index);
    m_resolvedDirectUrl = fmt.directUrl;
    if (!fmt.userAgent.isEmpty()) {
        m_userAgentEdit->setText(fmt.userAgent);
    }
    const QString cleanName = network::YoutubeExtractor::sanitizeVideoFileName(
        m_ytInfo.title, fmt.qualityTag, fmt.fileExtension);
    m_fileNameEdit->setText(cleanName);
    m_saveDirEdit->setText(CategoryRouter::defaultDirectoryForFile(cleanName));

    if (fmt.contentLength > 0) {
        m_probedBytes = fmt.contentLength;
        m_probedResume = true;
        m_lblProbeStatus->setText(QStringLiteral("YouTube Ready | %1 | Size: %2 | Multi-Segment: YES")
            .arg(fmt.qualityTag, core::FormatUtils::formatBytes(fmt.contentLength)));
        m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #10B981;"));
    } else {
        m_prober.probeUrl(QUrl(m_resolvedDirectUrl), configuredProfile());
    }
}

void AddDownloadDialog::onProbeCompleted(const network::ProbeResult& result) {
    if (!result.success) {
        m_lblProbeStatus->setText(QStringLiteral("Probe failed: %1").arg(result.errorMessage));
        m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #EF4444;"));
        if (m_fileNameEdit->text().isEmpty()) {
            m_fileNameEdit->setText(result.suggestedFileName);
        }
        return;
    }
    m_probedBytes = result.contentLength;
    m_probedResume = result.supportsRanges;
    if (!result.suggestedFileName.isEmpty() &&
        result.suggestedFileName != QStringLiteral("videoplayback") &&
        (m_fileNameEdit->text().isEmpty() || m_resolvedDirectUrl.isEmpty())) {
        m_fileNameEdit->setText(result.suggestedFileName);
        m_saveDirEdit->setText(CategoryRouter::defaultDirectoryForFile(result.suggestedFileName));
    }
    m_lblProbeStatus->setText(QStringLiteral("HTTP %1 | Size: %2 | Byte-Range Resume: %3")
        .arg(result.httpStatusCode)
        .arg(core::FormatUtils::formatBytes(result.contentLength))
        .arg(result.supportsRanges ? QStringLiteral("YES") : QStringLiteral("NO")));
    m_lblProbeStatus->setStyleSheet(QStringLiteral("color: #10B981;"));
}

void AddDownloadDialog::onBrowseFolder() {
    const QString picked = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Save Folder"), m_saveDirEdit->text());
    if (!picked.isEmpty()) {
        m_saveDirEdit->setText(picked);
    }
}

core::TaskMetrics AddDownloadDialog::configuredMetrics() const {
    core::TaskMetrics m;
    m.url = m_resolvedDirectUrl.isEmpty() ? m_urlEdit->text().trimmed() : m_resolvedDirectUrl;
    m.fileName = m_fileNameEdit->text().trimmed();
    if (m.fileName.isEmpty()) {
        m.fileName = network::UrlProber::extractFileName(QUrl(m.url), QByteArray());
    }
    m.saveDirectory = m_saveDirEdit->text().trimmed();
    m.queueName = m_queueCombo->currentText();
    m.category = CategoryRouter::classifyFileName(m.fileName);
    m.totalBytes = std::max<qint64>(0, m_probedBytes);
    m.resumeSupported = m_probedResume;
    m.maxSegments = m_segmentsSpin->value();
    m.expectedHash = m_hashEdit->text().trimmed();
    return m;
}

network::RequestProfile AddDownloadDialog::configuredProfile() const {
    network::RequestProfile p;
    p.referer = m_refererEdit->text().trimmed();
    p.cookies = m_cookieEdit->text().trimmed();
    p.userAgent = m_userAgentEdit->text().trimmed();
    p.authUser = m_authUserEdit->text().trimmed();
    p.authPassword = m_authPassEdit->text();
    p.proxyEnabled = m_proxyCheck->isChecked();
    p.proxyHost = m_proxyHostEdit->text().trimmed();
    p.proxyPort = static_cast<quint16>(m_proxyPortSpin->value());
    return p;
}

} // namespace idm::ui::dialogs
