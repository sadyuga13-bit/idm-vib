#pragma once

#include <QString>
#include <QStringList>

namespace idm::core {

struct BatchExpandResult {
    QStringList urls;
    QString errorMessage;
    [[nodiscard]] bool isValid() const noexcept {
        return errorMessage.isEmpty() && !urls.isEmpty();
    }
};

class BatchExpander final {
public:
    BatchExpander() = delete;

    [[nodiscard]] static BatchExpandResult expandPattern(
        const QString& patternUrl,
        int maxGeneratedCount = 5000
    );

    [[nodiscard]] static BatchExpandResult expandWildcardNumeric(
        const QString& wildcardUrl,
        int startValue,
        int endValue,
        int stepValue = 1,
        int zeroPadWidth = 2,
        int maxGeneratedCount = 5000
    );

    [[nodiscard]] static BatchExpandResult expandWildcardAlpha(
        const QString& wildcardUrl,
        QChar startChar,
        QChar endChar
    );
};

} // namespace idm::core
