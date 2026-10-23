# IDM Vibe - Native C++20 Multi-Segment Download Engine

IDM Vibe is a high-performance native desktop download manager engineered in modern **C++20** and **Qt 6** (`Core`, `Gui`, `Widgets`, `Network`, `Sql`, `Svg`), paired with a defense-in-depth cryptographic licensing, hardware quorum, and in-memory PE self-integrity subsystem.

## Architectural Principles

Every `.h` and `.cpp` translation unit in the repository strictly adheres to a maximum limit of **300 lines of code (LOC)** under the Single Responsibility Principle (SRP), with zero mock data, zero placeholder implementations, and a strict zero-emoji vector icon policy rendered dynamically via `QSvgRenderer` and `QPainter`.

## Core Capabilities

The networking and storage layers implement `UrlProber` (`HEAD` probing with automatic `GET Range: bytes=0-0` fallback and `Content-Disposition` RFC 5987 extraction), `DiskWriter` (pre-allocated `.part` random-access file writer eliminating 99% merge freezes), `DownloadTask` and `SegmentWorker` (1 to 32 parallel HTTP 206 byte-range connections with atomic in-flight dynamic work-stealing when any segment completes), `SpeedLimiter` (Token-Bucket global and per-task rate control), `QueueScheduler` (multi-queue concurrency and time-window scheduling), `DatabaseManager` (crash-resilient SQLite3 WAL state and segment byte-offset persistence), `BrowserIpcServer` (`127.0.0.1:16800` local browser extension bridge), `ClipboardWatcher`, `BatchExpander`, and `HashVerifier` (background `SHA-256` and `MD5` stream verification).

## Defense-in-Depth Security Subsystem

The `src/security/` subsystem replaces fragile boolean registration checks with a 256-bit `RuntimeCapabilityToken` derived via `HKDF-SHA256` from a verified `Ed25519` digital signature, a 3-of-4 hardware fingerprint quorum (`SMBIOS UUID`, `CPUID`, `PhysicalDrive0` serial, and `MachineGuid`), and the live `SHA-256` digest of the running process `.text` PE code section in memory. Local license certificates are sealed using `AES-256-GCM` combined with Windows `DPAPI` (`CryptProtectData`), while server activation enforces `TLS 1.3` with `SPKI SHA-256` public-key pinning and 32-byte `CSPRNG` challenge nonces.

## Building from Source (Windows x64)

```powershell
cmake -B build -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/msvc2022_64"
cmake --build build --config Release --parallel
C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe --release .\build\Release\IdmVibe.exe
```

## Automated GitHub Actions Build

Every push to `main` triggers `.github/workflows/build-windows.yml`, which verifies the 300-LOC constraint, compiles the C++20 release binary on `windows-latest` with Qt 6.8.2, packages all runtime DLLs via `windeployqt`, and publishes the ready-to-run `IdmVibe-Windows-x64` package under the workflow run's **Artifacts** section.
