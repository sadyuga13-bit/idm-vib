#include "core/BatchExpander.h"

#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

namespace idm::core {

BatchExpandResult BatchExpander::expandPattern(const QString& patternUrl, int maxGeneratedCount) {
    BatchExpandResult result;
    const QString trimmed = patternUrl.trimmed();
    if (trimmed.isEmpty()) {
        result.errorMessage = QStringLiteral("Pattern URL cannot be empty.");
        return result;
    }

    static const QRegularExpression kNumericBracketRx(
        QStringLiteral(R"(\[(\d+)-(\d+)(?::(\d+))?\])")
    );
    const QRegularExpressionMatch numMatch = kNumericBracketRx.match(trimmed);
    if (numMatch.hasMatch()) {
        const QString startStr = numMatch.captured(1);
        const QString endStr = numMatch.captured(2);
        const QString stepStr = numMatch.captured(3);

        bool okStart = false;
        bool okEnd = false;
        const int startVal = startStr.toInt(&okStart);
        const int endVal = endStr.toInt(&okEnd);
        const int stepVal = stepStr.isEmpty() ? 1 : std::max(1, stepStr.toInt());

        if (!okStart || !okEnd) {
            result.errorMessage = QStringLiteral("Invalid numeric range in bracket pattern.");
            return result;
        }

        const int padWidth = (startStr.startsWith(QLatin1Char('0')) && startStr.size() > 1)
                                 ? static_cast<int>(startStr.size())
                                 : 0;

        const QString prefix = trimmed.left(numMatch.capturedStart(0));
        const QString suffix = trimmed.mid(numMatch.capturedEnd(0));
        const QString wildcardForm = prefix + QLatin1Char('*') + suffix;
        return expandWildcardNumeric(wildcardForm, startVal, endVal, stepVal, padWidth, maxGeneratedCount);
    }

    static const QRegularExpression kAlphaBracketRx(
        QStringLiteral(R"(\[([a-zA-Z])-([a-zA-Z])\])")
    );
    const QRegularExpressionMatch alphaMatch = kAlphaBracketRx.match(trimmed);
    if (alphaMatch.hasMatch()) {
        const QChar startChar = alphaMatch.captured(1).at(0);
        const QChar endChar = alphaMatch.captured(2).at(0);
        const QString prefix = trimmed.left(alphaMatch.capturedStart(0));
        const QString suffix = trimmed.mid(alphaMatch.capturedEnd(0));
        return expandWildcardAlpha(prefix + QLatin1Char('*') + suffix, startChar, endChar);
    }

    result.errorMessage = QStringLiteral(
        "No valid range token found. Use [01-50], [a-z], or '*' wildcard mode."
    );
    return result;
}

BatchExpandResult BatchExpander::expandWildcardNumeric(
    const QString& wildcardUrl,
    int startValue,
    int endValue,
    int stepValue,
    int zeroPadWidth,
    int maxGeneratedCount
) {
    BatchExpandResult result;
    const QString trimmed = wildcardUrl.trimmed();
    if (!trimmed.contains(QLatin1Char('*'))) {
        result.errorMessage = QStringLiteral("Wildcard URL must contain '*' placeholder.");
        return result;
    }
    if (stepValue <= 0) {
        result.errorMessage = QStringLiteral("Step value must be at least 1.");
        return result;
    }

    const int direction = (startValue <= endValue) ? 1 : -1;
    int current = startValue;
    int generated = 0;

    while ((direction > 0) ? (current <= endValue) : (current >= endValue)) {
        if (generated >= maxGeneratedCount) {
            result.errorMessage = QStringLiteral("Pattern exceeds maximum limit of %1 URLs.")
                                      .arg(maxGeneratedCount);
            result.urls.clear();
            return result;
        }

        const QString token = (zeroPadWidth > 0)
                                  ? QStringLiteral("%1").arg(current, zeroPadWidth, 10, QLatin1Char('0'))
                                  : QString::number(current);

        QString expanded = trimmed;
        expanded.replace(QLatin1Char('*'), token);

        const QUrl candidate(expanded);
        if (!candidate.isValid() || candidate.scheme().isEmpty()) {
            result.errorMessage = QStringLiteral("Generated invalid URL: %1").arg(expanded);
            result.urls.clear();
            return result;
        }

        result.urls.append(expanded);
        current += direction * stepValue;
        ++generated;
    }

    return result;
}

BatchExpandResult BatchExpander::expandWildcardAlpha(
    const QString& wildcardUrl,
    QChar startChar,
    QChar endChar
) {
    BatchExpandResult result;
    const QString trimmed = wildcardUrl.trimmed();
    if (!trimmed.contains(QLatin1Char('*'))) {
        result.errorMessage = QStringLiteral("Wildcard URL must contain '*' placeholder.");
        return result;
    }

    if ((startChar.isLower() && !endChar.isLower()) ||
        (startChar.isUpper() && !endChar.isUpper())) {
        result.errorMessage = QStringLiteral("Alphabetical range must use matching letter case.");
        return result;
    }

    const ushort first = startChar.unicode();
    const ushort last = endChar.unicode();
    const int step = (first <= last) ? 1 : -1;

    for (int ch = first; (step > 0) ? (ch <= last) : (ch >= last); ch += step) {
        QString expanded = trimmed;
        expanded.replace(QLatin1Char('*'), QChar(static_cast<ushort>(ch)));
        result.urls.append(expanded);
    }

    return result;
}

} // namespace idm::core
