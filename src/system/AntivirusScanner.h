#pragma once

#include <QObject>
#include <QString>

namespace idm::system {

class AntivirusScanner final : public QObject {
    Q_OBJECT

public:
    explicit AntivirusScanner(QObject* parent = nullptr);
    ~AntivirusScanner() override = default;

    static bool applyMarkOfTheWeb(const QString& filePath, const QString& sourceUrl);
    void scanFileAsync(const QString& filePath);

signals:
    void scanFinished(const QString& filePath, bool clean, const QString& summaryReport);
};

} // namespace idm::system
