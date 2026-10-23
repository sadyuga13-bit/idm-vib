#include "system/AntivirusScanner.h"
#include "config/AppConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace idm::system {

AntivirusScanner::AntivirusScanner(QObject* parent)
    : QObject(parent) {}

bool AntivirusScanner::applyMarkOfTheWeb(const QString& filePath, const QString& sourceUrl) {
#ifdef _WIN32
    if (!config::AppConfig::instance().writeMarkOfTheWeb() || filePath.isEmpty()) {
        return false;
    }
    const QString adsPath = filePath + QStringLiteral(":Zone.Identifier");
    QFile adsFile(adsPath);
    if (!adsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray content = QStringLiteral("[ZoneTransfer]\r\nZoneId=3\r\nHostUrl=%1\r\n")
                                   .arg(sourceUrl).toUtf8();
    adsFile.write(content);
    adsFile.close();
    return true;
#else
    Q_UNUSED(filePath);
    Q_UNUSED(sourceUrl);
    return false;
#endif
}

void AntivirusScanner::scanFileAsync(const QString& filePath) {
    const config::AppConfig& cfg = config::AppConfig::instance();
    if (!cfg.enableAntivirusScan()) {
        emit scanFinished(filePath, true, QStringLiteral("Antivirus scan disabled in settings."));
        return;
    }

    QString exePath;
    QStringList args;
    if (cfg.useWindowsDefender()) {
        exePath = QStringLiteral("C:/Program Files/Windows Defender/MpCmdRun.exe");
        args = {
            QStringLiteral("-Scan"),
            QStringLiteral("-ScanType"),
            QStringLiteral("3"),
            QStringLiteral("-File"),
            QDir::toNativeSeparators(filePath)
        };
    } else {
        exePath = cfg.customAntivirusPath();
        args = {QDir::toNativeSeparators(filePath)};
    }

    if (exePath.isEmpty() || !QFileInfo::exists(exePath)) {
        emit scanFinished(filePath, true, QStringLiteral("Scanner binary not found (%1).").arg(exePath));
        return;
    }

    auto* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, filePath](int exitCode, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        proc->deleteLater();
        const bool clean = (exitCode == 0);
        const QString summary = clean
            ? QStringLiteral("CLEAN - Verified by Windows Defender / Antivirus (Exit 0)")
            : QStringLiteral("WARNING - Scanner returned code %1: %2").arg(exitCode).arg(out.left(120));
        emit scanFinished(filePath, clean, summary);
    });
    proc->start(exePath, args);
}

} // namespace idm::system
