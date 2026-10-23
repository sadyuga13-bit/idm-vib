#pragma once

#include <QHash>
#include <QObject>
#include <QString>

namespace idm::i18n {

class I18nManager final : public QObject {
    Q_OBJECT

public:
    static I18nManager& instance();

    void applyLanguage(const QString& languageSetting);
    [[nodiscard]] QString activeLanguageCode() const noexcept { return m_activeLang; }
    [[nodiscard]] bool isRightToLeft() const noexcept { return m_activeLang == QStringLiteral("ar"); }

    [[nodiscard]] QString trKey(const QString& key, const QString& fallback = QString()) const;
    [[nodiscard]] static QString detectSystemLanguageCode();

signals:
    void languageChanged(const QString& activeLangCode);

private:
    explicit I18nManager(QObject* parent = nullptr);
    ~I18nManager() override = default;

    QString m_activeLang{QStringLiteral("en")};
    QHash<QString, QString> m_englishDict;
    QHash<QString, QString> m_activeDict;
};

[[nodiscard]] inline QString trText(const QString& key, const QString& fallback = QString()) {
    return I18nManager::instance().trKey(key, fallback);
}

} // namespace idm::i18n
