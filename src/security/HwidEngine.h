#pragma once

#include "security/SecurityTypes.h"
#include <QByteArray>
#include <QString>

namespace idm::security {

class HwidEngine final {
public:
    HwidEngine() = delete;

    [[nodiscard]] static HwidComponents collectCurrentHwid();

    [[nodiscard]] static bool evaluateQuorum3Of4(
        const HwidComponents& current,
        const HwidComponents& licensed,
        int* matchedComponentsOut = nullptr
    );

    [[nodiscard]] static QByteArray exportOfflineRequestPayload(
        const QString& email,
        const QString& serialKey,
        const HwidComponents& hwid
    );

private:
    [[nodiscard]] static QString querySmbiosUuidHash();
    [[nodiscard]] static QString queryCpuIdHash();
    [[nodiscard]] static QString queryBootDiskSerialHash();
    [[nodiscard]] static QString queryMachineGuidHash();
    [[nodiscard]] static QString hashComponentTag(const QByteArray& rawValue, const char* domainTag);
};

} // namespace idm::security
