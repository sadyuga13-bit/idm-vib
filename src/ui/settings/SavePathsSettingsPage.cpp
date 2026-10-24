#include "ui/settings/SavePathsSettingsPage.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "storage/CategoryRouter.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStorageInfo>
#include <QTimer>
#include <QVBoxLayout>

namespace idm::ui::settings {

using config::AppConfig;
using storage::CategoryRouter;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

SavePathsSettingsPage::SavePathsSettingsPage(QWidget* parent)
    : QWidget(parent) {
    const AppConfig& cfg = AppConfig::instance();
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto* catBox = new QGroupBox(ar ? QStringLiteral("مجلدات الحفظ")
                                    : QStringLiteral("Save To Directories"), this);
    auto* catForm = new QFormLayout(catBox);
    catForm->setSpacing(10);

    m_categoryCombo = new QComboBox(catBox);
    const QList<core::FileCategory> cats = {
        core::FileCategory::All,
        core::FileCategory::Application,
        core::FileCategory::Archive,
        core::FileCategory::Video,
        core::FileCategory::Audio,
        core::FileCategory::Document,
        core::FileCategory::Image,
        core::FileCategory::Other
    };
    for (core::FileCategory c : cats) {
        m_pendingPaths.insert(c, cfg.categoryDirectory(c));
        m_categoryCombo->addItem(
            IconRegistry::icon(IconRegistry::iconForCategory(c), 15),
            CategoryRouter::categoryDisplayName(c),
            static_cast<int>(c));
    }
    catForm->addRow(ar ? QStringLiteral("الفئة:") : QStringLiteral("Category:"), m_categoryCombo);

    auto* pathRow = new QHBoxLayout();
    m_catPathEdit = new QLineEdit(catBox);
    auto* btnBrowseCat = new QPushButton(ar ? QStringLiteral("استعراض...") : QStringLiteral("Browse..."), catBox);
    btnBrowseCat->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));
    pathRow->addWidget(m_catPathEdit, 1);
    pathRow->addWidget(btnBrowseCat);
    catForm->addRow(ar ? QStringLiteral("المجلد:") : QStringLiteral("Path:"), pathRow);

    m_lblFreeSpace = new QLabel(catBox);
    m_lblFreeSpace->setFont(ThemeTokens::monoFont(false));
    catForm->addRow(ar ? QStringLiteral("المساحة المتوفرة:") : QStringLiteral("Free space:"), m_lblFreeSpace);
    root->addWidget(catBox);

    auto* tempBox = new QGroupBox(ar ? QStringLiteral("الملفات المؤقتة")
                                     : QStringLiteral("Temporary Directory"), this);
    auto* tempForm = new QFormLayout(tempBox);
    tempForm->setSpacing(10);

    auto* tempRow = new QHBoxLayout();
    m_tempPartEdit = new QLineEdit(cfg.tempPartDirectory(), tempBox);
    auto* btnBrowseTemp = new QPushButton(ar ? QStringLiteral("استعراض...") : QStringLiteral("Browse..."), tempBox);
    btnBrowseTemp->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));
    tempRow->addWidget(m_tempPartEdit, 1);
    tempRow->addWidget(btnBrowseTemp);
    tempForm->addRow(ar ? QStringLiteral("مجلد الملفات المؤقتة:") : QStringLiteral("Temporary folder:"), tempRow);

    m_chkPreallocate = new QCheckBox(
        ar ? QStringLiteral("حجز مساحة الملف مسبقاً على القرص")
           : QStringLiteral("Preallocate disk space before downloading"), tempBox);
    m_chkPreallocate->setChecked(cfg.preallocateDiskSpace());
    tempForm->addRow(m_chkPreallocate);
    root->addWidget(tempBox);
    root->addStretch(1);

    connect(m_categoryCombo, &QComboBox::currentIndexChanged, this, &SavePathsSettingsPage::onCategoryIndexChanged);
    connect(m_catPathEdit, &QLineEdit::textChanged, this, [this](const QString& txt) {
        const auto c = static_cast<core::FileCategory>(m_categoryCombo->currentData().toInt());
        m_pendingPaths.insert(c, txt.trimmed());
        refreshFreeDiskLabel(txt.trimmed());
    });
    connect(btnBrowseCat, &QPushButton::clicked, this, [this]() {
        const QString p = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Category Directory"), m_catPathEdit->text());
        if (!p.isEmpty()) { m_catPathEdit->setText(p); }
    });
    connect(btnBrowseTemp, &QPushButton::clicked, this, [this]() {
        const QString p = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Temp .part Directory"), m_tempPartEdit->text());
        if (!p.isEmpty()) { m_tempPartEdit->setText(p); }
    });

    m_catPathEdit->blockSignals(true);
    m_catPathEdit->setText(m_pendingPaths.value(core::FileCategory::All));
    m_catPathEdit->blockSignals(false);
    QTimer::singleShot(25, this, [this]() { refreshFreeDiskLabel(m_catPathEdit->text()); });
}

void SavePathsSettingsPage::onCategoryIndexChanged(int idx) {
    if (idx < 0) { return; }
    const auto c = static_cast<core::FileCategory>(m_categoryCombo->itemData(idx).toInt());
    m_catPathEdit->setText(m_pendingPaths.value(c));
    refreshFreeDiskLabel(m_catPathEdit->text());
}

void SavePathsSettingsPage::refreshFreeDiskLabel(const QString& folderPath) {
    const QStorageInfo st(folderPath);
    if (st.isValid() && st.bytesTotal() > 0) {
        m_lblFreeSpace->setText(QStringLiteral("%1 Free of %2 (%3)")
            .arg(core::FormatUtils::formatBytes(st.bytesAvailable()),
                 core::FormatUtils::formatBytes(st.bytesTotal()),
                 st.rootPath()));
        m_lblFreeSpace->setStyleSheet(QStringLiteral("color: #10B981;"));
    } else {
        m_lblFreeSpace->setText(QStringLiteral("Volume telemetry unavailable"));
    }
}

void SavePathsSettingsPage::applyToConfig() {
    AppConfig& cfg = AppConfig::instance();
    for (auto it = m_pendingPaths.cbegin(); it != m_pendingPaths.cend(); ++it) {
        cfg.setCategoryDirectory(it.key(), it.value());
    }
    cfg.setTempPartDirectory(m_tempPartEdit->text());
    cfg.setPreallocateDiskSpace(m_chkPreallocate->isChecked());
}

} // namespace idm::ui::settings
