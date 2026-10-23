#include "security/IntegrityGuard.h"
#include "security/CryptoEngine.h"

#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace idm::security {

IntegrityGuard::IntegrityGuard() {
    captureBaselineSnapshot();
}

void IntegrityGuard::captureBaselineSnapshot() {
    m_baselineTextSha256 = computeLiveTextSectionSha256();
}

QByteArray IntegrityGuard::computeLiveTextSectionSha256() const {
#ifdef _WIN32
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (hModule == nullptr) {
        return {};
    }

    const auto* baseAddr = reinterpret_cast<const uint8_t*>(hModule);
    const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(baseAddr);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return {};
    }

    const auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        baseAddr + dosHeader->e_lfanew
    );
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return {};
    }

    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeaders);
    const WORD numSections = ntHeaders->FileHeader.NumberOfSections;

    for (WORD i = 0; i < numSections; ++i, ++section) {
        const bool isTextName = (std::memcmp(section->Name, ".text", 5) == 0);
        const bool isExecCode = (section->Characteristics & IMAGE_SCN_CNT_CODE) != 0 &&
                                (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;

        if (isTextName || isExecCode) {
            const DWORD codeSize = section->Misc.VirtualSize > 0
                                       ? section->Misc.VirtualSize
                                       : section->SizeOfRawData;
            if (codeSize > 0) {
                const auto* codeStart = reinterpret_cast<const char*>(
                    baseAddr + section->VirtualAddress
                );
                return CryptoEngine::sha256(
                    QByteArrayView(codeStart, static_cast<qsizetype>(codeSize))
                );
            }
        }
    }
#endif
    return QByteArrayLiteral("IDM-VIBE-STATIC-CODE-SECTION");
}

bool IntegrityGuard::isCodeSectionIntact() const {
    if (m_baselineTextSha256.isEmpty()) {
        return true;
    }
    const QByteArray currentHash = computeLiveTextSectionSha256();
    return CryptoEngine::constantTimeEquals(currentHash, m_baselineTextSha256);
}

bool IntegrityGuard::isDebuggerOrHardwareBreakpointActive() const {
#ifdef _WIN32
    if (IsDebuggerPresent() != FALSE) {
        return true;
    }

    BOOL remoteDebugger = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebugger) &&
        remoteDebugger != FALSE) {
        return true;
    }

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0) {
            return true;
        }
    }
#endif
    return false;
}

uint64_t IntegrityGuard::computeLiveIntegrityEpochTag() const {
    uint64_t tag = kCleanEpochMagic;

    if (!isCodeSectionIntact()) {
        tag ^= 0x9F4E2B11C8703D55ULL;
    }
    if (isDebuggerOrHardwareBreakpointActive()) {
        tag ^= 0x3C8A77E055194AF2ULL;
    }
    return tag;
}

void IntegrityGuard::verifyAndEnforceToken(RuntimeCapabilityToken& token) const {
    const uint64_t liveTag = computeLiveIntegrityEpochTag();
    if (liveTag != kCleanEpochMagic) {
        token.poisonSilently();
    }
}

} // namespace idm::security
