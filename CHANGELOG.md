# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project follows SemVer.

## Unreleased

### Added

- Narrow-width pflag type helpers: `withInt8Flag`, `withInt16Flag`, `withInt32Flag`, `withUint8Flag`, `withUint16Flag`, `withUintFlag`
  (and persistent variants), with width-aware range validation and `Parser::getInt8/getInt16/getInt32/getUint8/getUint16/getUint`
  getters plus int8/int16/uint8/uint16 slice/array getters.
- IP slice helper `withIPSliceFlag` (pflag `IPSlice` parity: repeated/CSV values, each canonicalized as IPv4/IPv6) and
  `Parser::getIPSlice()`.
- Base64 helper `withBytesBase64Flag` (pflag `bytesBase64` parity: strict standard base64 in, canonical re-encoding stored) and
  `Parser::getBytesBase64()`.
- `Parser::getStringToFloat()` map getter.
- `examples/pflag_types2_example.cpp` covering the new types end-to-end.

### Changed

- Compatibility contract tightened: the former "Should Support" items are now Must Support with per-item example + CTest coverage —
  canonical Cobra-like strings are locked byte-for-byte (`examples/cobra_strings_example.cpp`), a defined set of pflag edge
  interactions is pinned by behavior tests (`examples/pflag_edge_example.cpp`), and every generated completion script's
  directive-handling logic (bash/zsh/fish/powershell) is asserted. See `COMPAT.md`.
- The vcpkg draft port now pins `v0.1.2` with a computed `SHA512`, and its `version-semver` matches
  (`packaging/vcpkg/portfile.cmake`, `packaging/vcpkg/vcpkg.json`).

## 0.1.2 - 2026-09-15

_Backfilled from release history._

### Added

- Branch coverage raised to 100% on top of the existing line/function coverage.

### Changed

- VitePress build output is no longer tracked in version control.
- The license badge is static, avoiding `shields.io` rate limiting on the README.

### Fixed

- Completion actions are now resolved through the runtime root command, so completion works after the root is moved.
- Child parent pointers are reattached when a `Command` is moved, keeping the moved tree dispatchable.
- Portable temp paths and MSVC Debug-CRT report routing for the Windows test matrix.

## 0.1.1 - 2026-06-09

_Backfilled from release history._

### Added

- INI parser for `.ini`/`.cfg` config files, with unknown config extensions rejected.
- Coverage tests across parser, command, completion, color, and value-parsing paths.

### Changed

- Documentation site migrated from VuePress to VitePress with English/Chinese locales.
- Cross-platform CI test differences handled instead of skipped.

### Fixed

- `NoOptDefVal` command scanning no longer misreads a following token as a subcommand name.
- Shared parsing and help rendering consolidated in one place.

## 0.1.0 - 2026-05-27

### Added

- CMake install rules and CMake package config for `find_package(clasp CONFIG)` consumers.
- URL helper flag (`withURLFlag`) with best-effort validation/canonicalization.
- IPNet/IPMask helper flags (`withIPNetFlag` / `withIPMaskFlag`).

### Fixed

- Windows shared builds now export the non-inline library symbols used by `clasp::Command`.
- MSVC warning noise from environment variable access has been removed.
