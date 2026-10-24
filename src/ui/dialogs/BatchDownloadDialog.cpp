#include "ui/dialogs/BatchDownloadDialog.h"
#include "core/BatchExpander.h"
#include "i18n/I18nManager.h"
#include "storage/CategoryRouter.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace idm::ui::dialogs {

using core::BatchExpander;
using core::BatchExpandResult;
using storage::CategoryRouter;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

BatchDownloadDialog::BatchDownloadDialog(QWidget* parent)
    : QDialog(parent) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    setWindowTitle(ar ? QStringLiteral("تحميل دفعة روابط") : QStringLiteral("Batch Download"));
    setMinimumSize(620, 480);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto* patternBox = new QGroupBox(ar ? QStringLiteral("إعدادات الدفعة") : QStringLiteral("Batch Settings"), this);
    auto* form = new QFormLayout(patternBox);
    form->setSpacing(8);

    m_modeCombo = new QComboBox(patternBox);
    m_modeCombo->addItems({
        ar ? QStringLiteral("نطاق بين أقواس ([01-50] أو [a-z])") : QStringLiteral("Bracket Range ([01-50] or [a-z])"),
        ar ? QStringLiteral("نجمة رقمية (*)") : QStringLiteral("Wildcard Asterisk (*)")
    });
    form->addRow(ar ? QStringLiteral("النمط:") : QStringLiteral("Mode:"), m_modeCombo);

    m_patternEdit = new QLineEdit(
        QStringLiteral("https://cdn.example.com/archive-[01-12].zip"),
        patternBox
    );
    m_patternEdit->setFont(ThemeTokens::monoFont(false));
    form->addRow(ar ? QStringLiteral("الرابط:") : QStringLiteral("Address:"), m_patternEdit);

    auto* rangeRow = new QHBoxLayout();
    m_startSpin = new QSpinBox(patternBox);
    m_startSpin->setRange(0, 999999);
    m_startSpin->setValue(1);
    m_endSpin = new QSpinBox(patternBox);
    m_endSpin->setRange(0, 999999);
    m_endSpin->setValue(15);
    m_stepSpin = new QSpinBox(patternBox);
    m_stepSpin->setRange(1, 1000);
    m_stepSpin->setValue(1);
    m_padSpin = new QSpinBox(patternBox);
    m_padSpin->setRange(0, 8);
    m_padSpin->setValue(2);

    rangeRow->addWidget(new QLabel(ar ? QStringLiteral("من:") : QStringLiteral("From:"), patternBox));
    rangeRow->addWidget(m_startSpin);
    rangeRow->addWidget(new QLabel(ar ? QStringLiteral("إلى:") : QStringLiteral("To:"), patternBox));
    rangeRow->addWidget(m_endSpin);
    rangeRow->addWidget(new QLabel(ar ? QStringLiteral("الخطوة:") : QStringLiteral("Step:"), patternBox));
    rangeRow->addWidget(m_stepSpin);
    rangeRow->addWidget(new QLabel(ar ? QStringLiteral("الأصفار:") : QStringLiteral("Width:"), patternBox));
    rangeRow->addWidget(m_padSpin);
    form->addRow(ar ? QStringLiteral("النطاق:") : QStringLiteral("Range:"), rangeRow);

    auto* dirRow = new QHBoxLayout();
    m_saveDirEdit = new QLineEdit(
        CategoryRouter::defaultDirectoryForCategory(core::FileCategory::Archive),
        patternBox
    );
    auto* btnBrowse = new QPushButton(ar ? QStringLiteral("استعراض...") : QStringLiteral("Browse..."), patternBox);
    btnBrowse->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));
    dirRow->addWidget(m_saveDirEdit, 1);
    dirRow->addWidget(btnBrowse);
    form->addRow(ar ? QStringLiteral("الحفظ في:") : QStringLiteral("Save to:"), dirRow);

    auto* queueRow = new QHBoxLayout();
    m_queueCombo = new QComboBox(patternBox);
    m_queueCombo->addItems({QStringLiteral("Main Queue"), QStringLiteral("Night Queue")});
    m_segmentsSpin = new QSpinBox(patternBox);
    m_segmentsSpin->setRange(1, 32);
    m_segmentsSpin->setValue(4);
    queueRow->addWidget(m_queueCombo, 1);
    queueRow->addWidget(new QLabel(ar ? QStringLiteral("الاتصالات:") : QStringLiteral("Connections:"), patternBox));
    queueRow->addWidget(m_segmentsSpin);
    form->addRow(ar ? QStringLiteral("الطابور:") : QStringLiteral("Queue:"), queueRow);

    rootLayout->addWidget(patternBox);

    auto* previewGroup = new QGroupBox(ar ? QStringLiteral("معاينة الروابط") : QStringLiteral("Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    m_statusLabel = new QLabel(previewGroup);
    m_statusLabel->setFont(ThemeTokens::monoFont(true));
    m_previewBox = new QPlainTextEdit(previewGroup);
    m_previewBox->setReadOnly(true);
    m_previewBox->setFont(ThemeTokens::monoFont(false));
    previewLayout->addWidget(m_statusLabel);
    previewLayout->addWidget(m_previewBox, 1);
    rootLayout->addWidget(previewGroup, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto* btnCancel = new QPushButton(ar ? QStringLiteral("إلغاء") : QStringLiteral("Cancel"), this);
    auto* btnEnqueue = new QPushButton(ar ? QStringLiteral("إضافة") : QStringLiteral("OK"), this);
    btnEnqueue->setObjectName(QStringLiteral("primaryActionBtn"));
    btnEnqueue->setIcon(QIcon(IconRegistry::pixmapColored(IconId::BatchSequence, 14, Qt::white)));
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnEnqueue);
    rootLayout->addLayout(btnRow);

    connect(m_modeCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx == 1 && !m_patternEdit->text().contains(QLatin1Char('*'))) {
            m_patternEdit->setText(QStringLiteral("https://cdn.example.com/archive-*.zip"));
        }
        updateLivePreview();
    });
    connect(m_patternEdit, &QLineEdit::textChanged, this, &BatchDownloadDialog::updateLivePreview);
    connect(m_startSpin, &QSpinBox::valueChanged, this, &BatchDownloadDialog::updateLivePreview);
    connect(m_endSpin, &QSpinBox::valueChanged, this, &BatchDownloadDialog::updateLivePreview);
    connect(m_stepSpin, &QSpinBox::valueChanged, this, &BatchDownloadDialog::updateLivePreview);
    connect(m_padSpin, &QSpinBox::valueChanged, this, &BatchDownloadDialog::updateLivePreview);
    connect(btnBrowse, &QPushButton::clicked, this, &BatchDownloadDialog::onBrowseDirectory);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnEnqueue, &QPushButton::clicked, this, [this]() {
        if (!m_expandedUrls.isEmpty()) {
            accept();
        }
    });

    updateLivePreview();
}

void BatchDownloadDialog::updateLivePreview() {
    BatchExpandResult res;
    if (m_modeCombo->currentIndex() == 0) {
        res = BatchExpander::expandPattern(m_patternEdit->text());
    } else {
        res = BatchExpander::expandWildcardNumeric(
            m_patternEdit->text(),
            m_startSpin->value(),
            m_endSpin->value(),
            m_stepSpin->value(),
            m_padSpin->value()
        );
    }

    m_expandedUrls = res.urls;
    if (!res.isValid()) {
        m_statusLabel->setText(res.errorMessage);
        m_statusLabel->setStyleSheet(QStringLiteral("color: #EF4444;"));
        m_previewBox->setPlainText(QString());
        return;
    }

    m_statusLabel->setText(QStringLiteral("Generated %1 sequential download URLs:")
                               .arg(m_expandedUrls.size()));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #10B981;"));
    m_previewBox->setPlainText(m_expandedUrls.join(QLatin1Char('\n')));
}

void BatchDownloadDialog::onBrowseDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Select Batch Destination Folder"), m_saveDirEdit->text());
    if (!dir.isEmpty()) {
        m_saveDirEdit->setText(dir);
    }
}

QString BatchDownloadDialog::saveDirectory() const {
    return m_saveDirEdit->text().trimmed();
}

QString BatchDownloadDialog::selectedQueue() const {
    return m_queueCombo->currentText();
}

int BatchDownloadDialog::maxSegments() const {
    return m_segmentsSpin->value();
}

} // namespace idm::ui::dialogs
