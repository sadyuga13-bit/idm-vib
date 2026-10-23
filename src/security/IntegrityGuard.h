#pragma once

#include "security/SecurityTypes.h"
#include <QByteArray>
#include <cstdint>

namespace idm::security {

class IntegrityGuard final {
public:
    IntegrityGuard();
    ~IntegrityGuard() = default;

    void captureBaselineSnapshot();

    [[nodiscard]] QByteArray computeLiveTextSectionSha256() const;
    [[nodiscard]] bool isCodeSectionIntact() const;
    [[nodiscard]] bool isDebuggerOrHardwareBreakpointActive() const;

    [[nodiscard]] uint64_t computeLiveIntegrityEpochTag() const;
    void verifyAndEnforceToken(RuntimeCapabilityToken& token) const;

    static constexpr uint64_t kCleanEpochMagic = 0x5A3C96E17F42B80DULL;

private:
    QByteArray m_baselineTextSha256;
};

} // namespace idm::security
