# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project follows SemVer.

## 0.1.0 - 2026-05-27

### Added

- CMake install rules and CMake package config for `find_package(clasp CONFIG)` consumers.
- URL helper flag (`withURLFlag`) with best-effort validation/canonicalization.
- IPNet/IPMask helper flags (`withIPNetFlag` / `withIPMaskFlag`).

### Fixed

- Windows shared builds now export the non-inline library symbols used by `clasp::Command`.
- MSVC warning noise from environment variable access has been removed.
