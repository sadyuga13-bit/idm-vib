#include "ui/dialogs/DownloadCompleteDialog.h"
#include "i18n/I18nManager.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace idm::ui::dialogs {

using i18n::trText;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

DownloadCompleteDialog::DownloadCompleteDialog(const core::TaskMetrics& metrics, QWidget* parent)
    : QDialog(parent)
    , m_metrics(metrics) {
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(trText(QStringLiteral("comp.title"), QStringLiteral("Download Complete")));
    setMinimumWidth(520);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* box = new QGroupBox(QStringLiteral("Completed File & Security Verification"), this);
    auto* form = new QFormLayout(box);
    form->setSpacing(8);

    auto makeLbl = [box](const QString& txt) {
        auto* l = new QLabel(txt, box);
        l->setFont(ThemeTokens::monoFont(false));
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        l->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        l->setMinimumWidth(0);
        l->setToolTip(txt);
        return l;
    };

    form->addRow(QStringLiteral("File Name:"), makeLbl(m_metrics.fileName));
    form->addRow(QStringLiteral("Total Size:"), makeLbl(core::FormatUtils::formatBytes(m_metrics.totalBytes)));
    form->addRow(QStringLiteral("Saved To:"), makeLbl(m_metrics.fullSavePath()));
    form->addRow(QStringLiteral("SHA-256:"), makeLbl(m_metrics.computedSha256.isEmpty()
                                                        ? QStringLiteral("Verified") : m_metrics.computedSha256));

    m_lblAvStatus = makeLbl(QStringLiteral("Scanning with Windows Defender / Antivirus..."));
    m_lblAvStatus->setStyleSheet(QStringLiteral("color: #3B82F6;"));
    form->addRow(QStringLiteral("Antivirus Scan:"), m_lblAvStatus);
    root->addWidget(box);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto* btnOpenFile = new QPushButton(trText(QStringLiteral("comp.open_file"), QStringLiteral("Open File")), this);
    btnOpenFile->setObjectName(QStringLiteral("primaryActionBtn"));
    btnOpenFile->setIcon(QIcon(IconRegistry::pixmapColored(IconId::StartPlay, 14, Qt::white)));

    auto* btnOpenFolder = new QPushButton(trText(QStringLiteral("comp.open_folder"), QStringLiteral("Open Folder")), this);
    btnOpenFolder->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));

    auto* btnClose = new QPushButton(trText(QStringLiteral("comp.close"), QStringLiteral("Close")), this);
    btnRow->addWidget(btnOpenFile);
    btnRow->addWidget(btnOpenFolder);
    btnRow->addWidget(btnClose);
    root->addLayout(btnRow);

    connect(btnOpenFile, &QPushButton::clicked, this, &DownloadCompleteDialog::onOpenFile);
    connect(btnOpenFolder, &QPushButton::clicked, this, &DownloadCompleteDialog::onOpenFolder);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    connect(&m_avScanner, &system::AntivirusScanner::scanFinished, this,
        [this](const QString&, bool clean, const QString& report) {
            m_lblAvStatus->setText(report);
            m_lblAvStatus->setStyleSheet(clean ? QStringLiteral("color: #10B981;") : QStringLiteral("color: #EF4444;"));
        });

    system::AntivirusScanner::applyMarkOfTheWeb(m_metrics.fullSavePath(), m_metrics.url);
    m_avScanner.scanFileAsync(m_metrics.fullSavePath());
}

void DownloadCompleteDialog::onOpenFile() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_metrics.fullSavePath()));
    accept();
}

void DownloadCompleteDialog::onOpenFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_metrics.saveDirectory));
    accept();
}

} // namespace idm::ui::dialogs
