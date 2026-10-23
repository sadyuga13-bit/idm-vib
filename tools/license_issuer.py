#!/usr/bin/env python3
"""
IDM Vibe - Official Admin Serial Key & Offline Certificate Issuer (.idm_lic)
Matches the exact HKDF-SHA256 & Ed25519 verification logic in CryptoEngine.cpp.
"""

import argparse
import hashlib
import hmac
import json
import secrets
import time
import uuid
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

ALPHABET = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ"

# Reconstructed 32-byte Ed25519 Root Public Key from ObfuscatedConstants.h
PUBLIC_KEY_32 = bytes.fromhex(
    "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"
)
PRIVATE_SEED_32 = bytes.fromhex(
    "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"
)


def hkdf_sha256(ikm: bytes, salt: bytes, info: bytes, length: int = 32) -> bytes:
    if not salt:
        salt = b"\x00" * 32
    prk = hmac.new(salt, ikm, hashlib.sha256).digest()
    okm = bytearray()
    prev = b""
    counter = 1
    while len(okm) < length:
        msg = prev + info + bytes([counter])
        prev = hmac.new(prk, msg, hashlib.sha256).digest()
        okm.extend(prev)
        counter += 1
    return bytes(okm[:length])


def compute_serial_check_segment(first_20_chars: str) -> str:
    digest = hkdf_sha256(
        first_20_chars.upper().encode("utf-8"),
        b"IDM-VIBE-SERIAL-SALT-V1",
        b"SERIAL-CHECK-SEGMENT",
        5,
    )
    return "".join(ALPHABET[b & 0x1F] for b in digest)


def generate_serial_key() -> str:
    groups = [
        "".join(secrets.choice(ALPHABET) for _ in range(5))
        for _ in range(4)
    ]
    body_20 = "".join(groups)
    check_5 = compute_serial_check_segment(body_20)
    groups.append(check_5)
    return "-".join(groups)


def sign_certificate_payload(canonical_bytes: bytes) -> bytes:
    sk = Ed25519PrivateKey.from_private_bytes(PRIVATE_SEED_32)
    return sk.sign(canonical_bytes)


def issue_offline_license(hwid_req_path: Path, output_lic_path: Path, tier: int = 2, max_threads: int = 32) -> None:
    req_data = json.loads(hwid_req_path.read_text(encoding="utf-8"))
    email = req_data.get("email", "").strip().lower() or "admin@idm-vibe.local"
    serial_key = req_data.get("serialKey", "").strip().upper()
    if not serial_key or len(serial_key.replace("-", "")) != 25:
        serial_key = generate_serial_key()

    hwid_obj = req_data.get("hwid", {})
    now_unix = int(time.time())
    expires_unix = now_unix + (365 * 5 * 86400)  # 5 years validity

    # Qt QJsonDocument::Compact sorts keys alphabetically with no spaces
    payload_dict = {
        "boundHwid": {
            "cpuid": hwid_obj.get("cpuid", ""),
            "disk": hwid_obj.get("disk", ""),
            "mguid": hwid_obj.get("mguid", ""),
            "smbios": hwid_obj.get("smbios", ""),
        },
        "email": email,
        "expiresAtUnix": expires_unix,
        "issuedAtUnix": now_unix,
        "licenseId": f"LIC-{uuid.uuid4().hex[:12].upper()}",
        "maxAllowedThreads": max_threads,
        "serialKey": serial_key,
        "serverNonceHex": secrets.token_hex(32),
        "tier": tier,
    }

    canonical_bytes = json.dumps(
        payload_dict, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")

    signature = sign_certificate_payload(canonical_bytes)
    payload_dict["signatureHex"] = signature.hex()

    signed_json = json.dumps(payload_dict, separators=(",", ":"), sort_keys=True)
    output_lic_path.write_text(signed_json, encoding="utf-8")
    print(f"[+] Generated signed license file: {output_lic_path}")
    print(f"[+] Associated Serial Key:         {serial_key}")
    print(f"[+] Tier:                          {'ENTERPRISE' if tier == 2 else 'PRO'} ({max_threads} threads)")


def main() -> None:
    parser = argparse.ArgumentParser(description="IDM Vibe Serial & License Certificate Generator")
    parser.add_argument("--count", type=int, default=5, help="Number of serial keys to generate")
    parser.add_argument("--sign", type=str, help="Path to exported .hwid_req file to sign")
    parser.add_argument("--out", type=str, default="activated_license.idm_lic", help="Output .idm_lic path")
    args = parser.parse_args()

    print("=== VALID 25-CHARACTER SERIAL KEYS (HKDF-SHA256 CHECKSUM) ===")
    for i in range(args.count):
        print(f"  {i + 1}: {generate_serial_key()}")

    if args.sign:
        req_path = Path(args.sign)
        out_path = Path(args.out)
        print("\n=== SIGNING OFFLINE HARDWARE REQUEST ===")
        issue_offline_license(req_path, out_path)


if __name__ == "__main__":
    main()
