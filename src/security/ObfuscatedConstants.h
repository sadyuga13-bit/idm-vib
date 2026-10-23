#pragma once

#include <QByteArray>
#include <array>
#include <cstddef>
#include <cstdint>

namespace idm::security {

template <size_t N>
class ScopedSecretBuffer final {
public:
    constexpr explicit ScopedSecretBuffer(const std::array<uint8_t, N>& initial) noexcept
        : m_bytes(initial) {}

    ~ScopedSecretBuffer() noexcept {
        volatile uint8_t* ptr = m_bytes.data();
        for (size_t i = 0; i < N; ++i) {
            ptr[i] = 0U;
        }
    }

    ScopedSecretBuffer(const ScopedSecretBuffer&) = delete;
    ScopedSecretBuffer& operator=(const ScopedSecretBuffer&) = delete;

    [[nodiscard]] const uint8_t* data() const noexcept { return m_bytes.data(); }
    [[nodiscard]] constexpr size_t size() const noexcept { return N; }

    [[nodiscard]] QByteArray toByteArray() const {
        return QByteArray(reinterpret_cast<const char*>(m_bytes.data()), static_cast<qsizetype>(N));
    }

private:
    std::array<uint8_t, N> m_bytes;
};

class ObfuscatedConstants final {
public:
    ObfuscatedConstants() = delete;

    static constexpr uint8_t maskByte(size_t index, uint8_t seed) noexcept {
        return static_cast<uint8_t>(((index * 37U + 0x5AU) ^ seed) & 0xFFU);
    }

    // Reconstructs the 32-byte Ed25519 Root Licensing Public Key on the stack
    [[nodiscard]] static ScopedSecretBuffer<32> reconstructEd25519PublicKey() noexcept {
        // Raw public key bytes (d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a)
        // stored as 4 XOR-masked constexpr shards
        constexpr std::array<uint8_t, 8> kShard0 = {
            static_cast<uint8_t>(0xD7U ^ maskByte(0, 0xA3U)),
            static_cast<uint8_t>(0x5AU ^ maskByte(1, 0xA3U)),
            static_cast<uint8_t>(0x98U ^ maskByte(2, 0xA3U)),
            static_cast<uint8_t>(0x01U ^ maskByte(3, 0xA3U)),
            static_cast<uint8_t>(0x82U ^ maskByte(4, 0xA3U)),
            static_cast<uint8_t>(0xB1U ^ maskByte(5, 0xA3U)),
            static_cast<uint8_t>(0x0AU ^ maskByte(6, 0xA3U)),
            static_cast<uint8_t>(0xB7U ^ maskByte(7, 0xA3U))
        };
        constexpr std::array<uint8_t, 8> kShard1 = {
            static_cast<uint8_t>(0xD5U ^ maskByte(8,  0x5CU)),
            static_cast<uint8_t>(0x4BU ^ maskByte(9,  0x5CU)),
            static_cast<uint8_t>(0xFEU ^ maskByte(10, 0x5CU)),
            static_cast<uint8_t>(0xD3U ^ maskByte(11, 0x5CU)),
            static_cast<uint8_t>(0xC9U ^ maskByte(12, 0x5CU)),
            static_cast<uint8_t>(0x64U ^ maskByte(13, 0x5CU)),
            static_cast<uint8_t>(0x07U ^ maskByte(14, 0x5CU)),
            static_cast<uint8_t>(0x3AU ^ maskByte(15, 0x5CU))
        };
        constexpr std::array<uint8_t, 8> kShard2 = {
            static_cast<uint8_t>(0x0EU ^ maskByte(16, 0xC9U)),
            static_cast<uint8_t>(0xE1U ^ maskByte(17, 0xC9U)),
            static_cast<uint8_t>(0x72U ^ maskByte(18, 0xC9U)),
            static_cast<uint8_t>(0xF3U ^ maskByte(19, 0xC9U)),
            static_cast<uint8_t>(0xDAU ^ maskByte(20, 0xC9U)),
            static_cast<uint8_t>(0xA6U ^ maskByte(21, 0xC9U)),
            static_cast<uint8_t>(0x23U ^ maskByte(22, 0xC9U)),
            static_cast<uint8_t>(0x25U ^ maskByte(23, 0xC9U))
        };
        constexpr std::array<uint8_t, 8> kShard3 = {
            static_cast<uint8_t>(0xAFU ^ maskByte(24, 0x71U)),
            static_cast<uint8_t>(0x02U ^ maskByte(25, 0x71U)),
            static_cast<uint8_t>(0x1AU ^ maskByte(26, 0x71U)),
            static_cast<uint8_t>(0x68U ^ maskByte(27, 0x71U)),
            static_cast<uint8_t>(0xF7U ^ maskByte(28, 0x71U)),
            static_cast<uint8_t>(0x07U ^ maskByte(29, 0x71U)),
            static_cast<uint8_t>(0x51U ^ maskByte(30, 0x71U)),
            static_cast<uint8_t>(0x1AU ^ maskByte(31, 0x71U))
        };

        std::array<uint8_t, 32> out{};
        for (size_t i = 0; i < 8; ++i) {
            out[i]      = static_cast<uint8_t>(kShard0[i] ^ maskByte(i,      0xA3U));
            out[i + 8]  = static_cast<uint8_t>(kShard1[i] ^ maskByte(i + 8,  0x5CU));
            out[i + 16] = static_cast<uint8_t>(kShard2[i] ^ maskByte(i + 16, 0xC9U));
            out[i + 24] = static_cast<uint8_t>(kShard3[i] ^ maskByte(i + 24, 0x71U));
        }
        return ScopedSecretBuffer<32>(out);
    }

    // Reconstructs the 32-byte TLS 1.3 SPKI SHA-256 Pin for the licensing server
    [[nodiscard]] static ScopedSecretBuffer<32> reconstructServerSpkiSha256Pin() noexcept {
        constexpr std::array<uint8_t, 32> kMaskedPin = {
            static_cast<uint8_t>(0x4BU ^ maskByte(0,  0x3EU)),
            static_cast<uint8_t>(0x91U ^ maskByte(1,  0x3EU)),
            static_cast<uint8_t>(0x2CU ^ maskByte(2,  0x3EU)),
            static_cast<uint8_t>(0xE8U ^ maskByte(3,  0x3EU)),
            static_cast<uint8_t>(0x74U ^ maskByte(4,  0x3EU)),
            static_cast<uint8_t>(0x19U ^ maskByte(5,  0x3EU)),
            static_cast<uint8_t>(0xA5U ^ maskByte(6,  0x3EU)),
            static_cast<uint8_t>(0x63U ^ maskByte(7,  0x3EU)),
            static_cast<uint8_t>(0xD0U ^ maskByte(8,  0x3EU)),
            static_cast<uint8_t>(0x58U ^ maskByte(9,  0x3EU)),
            static_cast<uint8_t>(0x8FU ^ maskByte(10, 0x3EU)),
            static_cast<uint8_t>(0x3BU ^ maskByte(11, 0x3EU)),
            static_cast<uint8_t>(0xC2U ^ maskByte(12, 0x3EU)),
            static_cast<uint8_t>(0x14U ^ maskByte(13, 0x3EU)),
            static_cast<uint8_t>(0x7EU ^ maskByte(14, 0x3EU)),
            static_cast<uint8_t>(0x9AU ^ maskByte(15, 0x3EU)),
            static_cast<uint8_t>(0xE1U ^ maskByte(16, 0x3EU)),
            static_cast<uint8_t>(0x05U ^ maskByte(17, 0x3EU)),
            static_cast<uint8_t>(0x6CU ^ maskByte(18, 0x3EU)),
            static_cast<uint8_t>(0xB7U ^ maskByte(19, 0x3EU)),
            static_cast<uint8_t>(0x42U ^ maskByte(20, 0x3EU)),
            static_cast<uint8_t>(0xF9U ^ maskByte(21, 0x3EU)),
            static_cast<uint8_t>(0x88U ^ maskByte(22, 0x3EU)),
            static_cast<uint8_t>(0x21U ^ maskByte(23, 0x3EU)),
            static_cast<uint8_t>(0x5DU ^ maskByte(24, 0x3EU)),
            static_cast<uint8_t>(0xA4U ^ maskByte(25, 0x3EU)),
            static_cast<uint8_t>(0x36U ^ maskByte(26, 0x3EU)),
            static_cast<uint8_t>(0x79U ^ maskByte(27, 0x3EU)),
            static_cast<uint8_t>(0xCEU ^ maskByte(28, 0x3EU)),
            static_cast<uint8_t>(0x1BU ^ maskByte(29, 0x3EU)),
            static_cast<uint8_t>(0x90U ^ maskByte(30, 0x3EU)),
            static_cast<uint8_t>(0x67U ^ maskByte(31, 0x3EU))
        };
        std::array<uint8_t, 32> out{};
        for (size_t i = 0; i < 32; ++i) {
            out[i] = static_cast<uint8_t>(kMaskedPin[i] ^ maskByte(i, 0x3EU));
        }
        return ScopedSecretBuffer<32>(out);
    }

    // Reconstructs 16-byte DPAPI additional entropy
    [[nodiscard]] static ScopedSecretBuffer<16> reconstructDpapiEntropy() noexcept {
        std::array<uint8_t, 16> out{};
        constexpr std::array<uint8_t, 16> kRaw = {
            0x9DU, 0x41U, 0xE7U, 0x2BU, 0x83U, 0x5CU, 0x10U, 0xF6U,
            0x3AU, 0x7EU, 0xC4U, 0x69U, 0xB2U, 0x08U, 0x55U, 0xDFU
        };
        for (size_t i = 0; i < 16; ++i) {
            out[i] = static_cast<uint8_t>(kRaw[i] ^ maskByte(i, 0x4DU));
        }
        return ScopedSecretBuffer<16>(out);
    }
};

} // namespace idm::security
