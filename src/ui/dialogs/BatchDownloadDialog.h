#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStringList>

namespace idm::ui::dialogs {

class BatchDownloadDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BatchDownloadDialog(QWidget* parent = nullptr);
    ~BatchDownloadDialog() override = default;

    [[nodiscard]] QStringList expandedUrls() const { return m_expandedUrls; }
    [[nodiscard]] QString saveDirectory() const;
    [[nodiscard]] QString selectedQueue() const;
    [[nodiscard]] int maxSegments() const;

private:
    void updateLivePreview();
    void onBrowseDirectory();

    QLineEdit* m_patternEdit{nullptr};
    QComboBox* m_modeCombo{nullptr};
    QSpinBox* m_startSpin{nullptr};
    QSpinBox* m_endSpin{nullptr};
    QSpinBox* m_stepSpin{nullptr};
    QSpinBox* m_padSpin{nullptr};

    QLineEdit* m_saveDirEdit{nullptr};
    QComboBox* m_queueCombo{nullptr};
    QSpinBox* m_segmentsSpin{nullptr};

    QPlainTextEdit* m_previewBox{nullptr};
    QLabel* m_statusLabel{nullptr};
    QStringList m_expandedUrls;
};

} // namespace idm::ui::dialogs
