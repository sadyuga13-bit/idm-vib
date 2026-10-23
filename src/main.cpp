#include "config/AppConfig.h"
#include "core/Types.h"
#include "i18n/I18nManager.h"
#include "network/RequestProfile.h"
#include "network/UrlProber.h"
#include "network/YoutubeExtractor.h"
#include "security/SecurityTypes.h"
#include "system/SingleInstanceGuard.h"
#include "ui/MainWindow.h"
#include "ui/theme/AppStyleSheet.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("IdmVibe"));
    QApplication::setOrganizationName(QStringLiteral("SystemsArchitecture"));
    QApplication::setApplicationVersion(QStringLiteral("2.0.0"));
    QApplication::setQuitOnLastWindowClosed(false);

    const QStringList args = QApplication::arguments();
    QString forwardPayload = QStringLiteral("ACTIVATE");
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
            args.at(i).startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            forwardPayload = args.at(i);
            break;
        }
    }

    idm::system::SingleInstanceGuard instanceGuard;
    if (!instanceGuard.tryAcquirePrimaryInstance(forwardPayload)) {
        return 0;
    }

    qRegisterMetaType<idm::core::TaskState>("idm::core::TaskState");
    qRegisterMetaType<idm::core::FileCategory>("idm::core::FileCategory");
    qRegisterMetaType<idm::network::ProbeResult>("idm::network::ProbeResult");
    qRegisterMetaType<idm::network::RequestProfile>("idm::network::RequestProfile");
    qRegisterMetaType<idm::network::YoutubeVideoInfo>("idm::network::YoutubeVideoInfo");
    qRegisterMetaType<idm::security::ActivationStep>("idm::security::ActivationStep");
    qRegisterMetaType<idm::security::LicenseCertificate>("idm::security::LicenseCertificate");

    const auto& cfg = idm::config::AppConfig::instance();
    idm::i18n::I18nManager::instance().applyLanguage(cfg.languageCode());
    idm::ui::theme::AppStyleSheet::applyToApplication(app);
    idm::ui::theme::AppStyleSheet::setDayMode(cfg.isDayMode());

    idm::ui::MainWindow window;
    QObject::connect(&instanceGuard, &idm::system::SingleInstanceGuard::secondaryInstanceLaunched,
                     &window, &idm::ui::MainWindow::handleSecondaryInstancePayload);

    if (!idm::system::SingleInstanceGuard::hasSilentTrayArg(args)) {
        window.show();
    }

    return QApplication::exec();
}
