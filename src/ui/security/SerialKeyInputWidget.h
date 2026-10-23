#pragma once

#include <QEvent>
#include <QLineEdit>
#include <QWidget>
#include <array>

namespace idm::ui::security {

class SerialKeyInputWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SerialKeyInputWidget(QWidget* parent = nullptr);
    ~SerialKeyInputWidget() override = default;

    [[nodiscard]] QString serialKey() const;
    void setSerialKey(const QString& fullKey);
    void clear();

    [[nodiscard]] bool isChecksumValid() const noexcept { return m_checksumValid; }

signals:
    void serialChanged(const QString& fullKey, bool checksumValid);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void onSegmentEdited(int index, const QString& text);
    void handlePasteText(const QString& rawClipboard);
    void updateVisualValidationState();

    std::array<QLineEdit*, 5> m_boxes{};
    bool m_updatingProgrammatically{false};
    bool m_checksumValid{false};
};

} // namespace idm::ui::security
