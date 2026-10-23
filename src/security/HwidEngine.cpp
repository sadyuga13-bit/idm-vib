#include "security/HwidEngine.h"
#include "security/CryptoEngine.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#include <winioctl.h>
#endif

namespace idm::security {

HwidComponents HwidEngine::collectCurrentHwid() {
    HwidComponents comp;
    comp.smbiosUuidHash = querySmbiosUuidHash();
    comp.cpuIdHash = queryCpuIdHash();
    comp.diskSerialHash = queryBootDiskSerialHash();
    comp.machineGuidHash = queryMachineGuidHash();
    return comp;
}

bool HwidEngine::evaluateQuorum3Of4(
    const HwidComponents& current,
    const HwidComponents& licensed,
    int* matchedComponentsOut
) {
    int matches = 0;
    if (!current.smbiosUuidHash.isEmpty() &&
        CryptoEngine::constantTimeEquals(current.smbiosUuidHash.toLatin1(), licensed.smbiosUuidHash.toLatin1())) {
        ++matches;
    }
    if (!current.cpuIdHash.isEmpty() &&
        CryptoEngine::constantTimeEquals(current.cpuIdHash.toLatin1(), licensed.cpuIdHash.toLatin1())) {
        ++matches;
    }
    if (!current.diskSerialHash.isEmpty() &&
        CryptoEngine::constantTimeEquals(current.diskSerialHash.toLatin1(), licensed.diskSerialHash.toLatin1())) {
        ++matches;
    }
    if (!current.machineGuidHash.isEmpty() &&
        CryptoEngine::constantTimeEquals(current.machineGuidHash.toLatin1(), licensed.machineGuidHash.toLatin1())) {
        ++matches;
    }

    if (matchedComponentsOut != nullptr) {
        *matchedComponentsOut = matches;
    }
    return matches >= 3;
}

QByteArray HwidEngine::exportOfflineRequestPayload(
    const QString& email,
    const QString& serialKey,
    const HwidComponents& hwid
) {
    QJsonObject req;
    req[QStringLiteral("version")] = 1;
    req[QStringLiteral("email")] = email.trimmed().toLower();
    req[QStringLiteral("serialKey")] = serialKey.trimmed().toUpper();
    req[QStringLiteral("hostname")] = QSysInfo::machineHostName();
    req[QStringLiteral("osProduct")] = QSysInfo::prettyProductName();
    req[QStringLiteral("requestedAtUnix")] = QDateTime::currentSecsSinceEpoch();
    req[QStringLiteral("hwid")] = hwid.toJson();
    req[QStringLiteral("compositeFingerprint")] = hwid.compositeFingerprint();
    return QJsonDocument(req).toJson(QJsonDocument::Indented);
}

QString HwidEngine::querySmbiosUuidHash() {
    QByteArray rawBytes;
#ifdef _WIN32
    constexpr DWORD kRsmbSignature = 0x52534D42U; // 'RSMB'
    const UINT tableSize = GetSystemFirmwareTable(kRsmbSignature, 0, nullptr, 0);
    if (tableSize > 16) {
        std::vector<uint8_t> buffer(tableSize);
        if (GetSystemFirmwareTable(kRsmbSignature, 0, buffer.data(), tableSize) == tableSize) {
            rawBytes = QByteArray(
                reinterpret_cast<const char*>(buffer.data()),
                static_cast<qsizetype>(std::min<size_t>(tableSize, 256))
            );
        }
    }
#endif
    if (rawBytes.isEmpty()) {
        rawBytes = QSysInfo::machineUniqueId();
    }
    return hashComponentTag(rawBytes, "HWID-SMBIOS-V1");
}

QString HwidEngine::queryCpuIdHash() {
    QByteArray cpuData;
#ifdef _WIN32
    int regs[4] = {0, 0, 0, 0};
    __cpuid(regs, 0);
    cpuData.append(reinterpret_cast<const char*>(regs), sizeof(regs));
    __cpuid(regs, 1);
    regs[1] &= 0x00FFFFFF; // Mask out volatile APIC core ID
    cpuData.append(reinterpret_cast<const char*>(regs), sizeof(regs));
    for (int leaf = 0x80000002; leaf <= 0x80000004; ++leaf) {
        __cpuid(regs, leaf);
        cpuData.append(reinterpret_cast<const char*>(regs), sizeof(regs));
    }
#else
    cpuData = QSysInfo::currentCpuArchitecture().toUtf8();
#endif
    return hashComponentTag(cpuData, "HWID-CPUID-V1");
}

QString HwidEngine::queryBootDiskSerialHash() {
    QByteArray serialBytes;
#ifdef _WIN32
    HANDLE hDrive = CreateFileW(
        L"\\\\.\\PhysicalDrive0",
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if (hDrive != INVALID_HANDLE_VALUE) {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        std::vector<uint8_t> outBuf(1024, 0);
        DWORD bytesReturned = 0;
        if (DeviceIoControl(
                hDrive,
                IOCTL_STORAGE_QUERY_PROPERTY,
                &query,
                sizeof(query),
                outBuf.data(),
                static_cast<DWORD>(outBuf.size()),
                &bytesReturned,
                nullptr) && bytesReturned >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            const auto* desc = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(outBuf.data());
            if (desc->SerialNumberOffset > 0 && desc->SerialNumberOffset < bytesReturned) {
                const char* serialStr = reinterpret_cast<const char*>(outBuf.data() + desc->SerialNumberOffset);
                serialBytes = QByteArray(serialStr).trimmed();
            }
        }
        CloseHandle(hDrive);
    }
#endif
    if (serialBytes.isEmpty()) {
        serialBytes = QSysInfo::bootUniqueId();
    }
    return hashComponentTag(serialBytes, "HWID-DISK0-V1");
}

QString HwidEngine::queryMachineGuidHash() {
    QByteArray guidBytes;
#ifdef _WIN32
    wchar_t valueBuf[128] = {0};
    DWORD bufSize = sizeof(valueBuf);
    const LSTATUS st = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Cryptography",
        L"MachineGuid",
        RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
        nullptr,
        valueBuf,
        &bufSize
    );
    if (st == ERROR_SUCCESS) {
        guidBytes = QString::fromWCharArray(valueBuf).trimmed().toUtf8();
    }
#endif
    if (guidBytes.isEmpty()) {
        guidBytes = QSysInfo::machineUniqueId();
    }
    return hashComponentTag(guidBytes, "HWID-MGUID-V1");
}

QString HwidEngine::hashComponentTag(const QByteArray& rawValue, const char* domainTag) {
    const QByteArray derived = CryptoEngine::hkdfSha256(
        rawValue,
        QByteArrayLiteral("IDM-VIBE-HWID-SALT-2026"),
        QByteArrayView(domainTag, static_cast<qsizetype>(qstrlen(domainTag))),
        16
    );
    return QString::fromLatin1(derived.toHex());
}

} // namespace idm::security
