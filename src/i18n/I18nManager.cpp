#include "i18n/I18nManager.h"
#include "i18n/TranslationsArEn.h"
#include "i18n/TranslationsGlobal.h"

#include <QApplication>
#include <QLocale>

namespace idm::i18n {

I18nManager& I18nManager::instance() {
    static I18nManager s_mgr;
    return s_mgr;
}

I18nManager::I18nManager(QObject* parent)
    : QObject(parent)
    , m_englishDict(buildEnglishDictionary())
    , m_activeDict(m_englishDict) {}

QString I18nManager::detectSystemLanguageCode() {
    const QString bcp = QLocale::system().name();
    if (bcp.startsWith(QStringLiteral("ar"))) { return QStringLiteral("ar"); }
    if (bcp.startsWith(QStringLiteral("es"))) { return QStringLiteral("es"); }
    if (bcp.startsWith(QStringLiteral("fr"))) { return QStringLiteral("fr"); }
    if (bcp.startsWith(QStringLiteral("de"))) { return QStringLiteral("de"); }
    if (bcp.startsWith(QStringLiteral("ru"))) { return QStringLiteral("ru"); }
    if (bcp.startsWith(QStringLiteral("zh"))) { return QStringLiteral("zh_CN"); }
    if (bcp.startsWith(QStringLiteral("ja"))) { return QStringLiteral("ja"); }
    if (bcp.startsWith(QStringLiteral("tr"))) { return QStringLiteral("tr"); }
    if (bcp.startsWith(QStringLiteral("pt"))) { return QStringLiteral("pt_BR"); }
    return QStringLiteral("en");
}

void I18nManager::applyLanguage(const QString& languageSetting) {
    QString resolved = languageSetting.trimmed();
    if (resolved.isEmpty() || resolved == QStringLiteral("auto")) {
        resolved = detectSystemLanguageCode();
    }

    m_activeLang = resolved;
    m_activeDict = m_englishDict;

    QHash<QString, QString> overlay;
    if (resolved == QStringLiteral("ar")) {
        overlay = buildArabicDictionary();
    } else if (resolved != QStringLiteral("en")) {
        overlay = buildGlobalDictionary(resolved);
    }

    for (auto it = overlay.cbegin(); it != overlay.cend(); ++it) {
        m_activeDict.insert(it.key(), it.value());
    }

    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setLayoutDirection(isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    }
    emit languageChanged(m_activeLang);
}

QString I18nManager::trKey(const QString& key, const QString& fallback) const {
    if (const auto it = m_activeDict.constFind(key); it != m_activeDict.cend()) {
        return it.value();
    }
    return fallback.isEmpty() ? key : fallback;
}

} // namespace idm::i18n
