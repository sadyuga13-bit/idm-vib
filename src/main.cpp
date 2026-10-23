#include "core/Types.h"
#include "network/RequestProfile.h"
#include "network/UrlProber.h"
#include "security/SecurityTypes.h"
#include "ui/MainWindow.h"
#include "ui/theme/AppStyleSheet.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("IdmVibe"));
    QApplication::setOrganizationName(QStringLiteral("SystemsArchitecture"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    qRegisterMetaType<idm::core::TaskState>("idm::core::TaskState");
    qRegisterMetaType<idm::core::FileCategory>("idm::core::FileCategory");
    qRegisterMetaType<idm::network::ProbeResult>("idm::network::ProbeResult");
    qRegisterMetaType<idm::network::RequestProfile>("idm::network::RequestProfile");
    qRegisterMetaType<idm::security::ActivationStep>("idm::security::ActivationStep");
    qRegisterMetaType<idm::security::LicenseCertificate>("idm::security::LicenseCertificate");

    idm::ui::theme::AppStyleSheet::applyToApplication(app);

    idm::ui::MainWindow window;
    window.show();

    return QApplication::exec();
}
