# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project follows SemVer.

## Unreleased

### Added

- CMake install rules and CMake package config for `find_package(clasp CONFIG)` consumers.
- URL helper flag (`withURLFlag`) with best-effort validation/canonicalization.
- IPNet/IPMask helper flags (`withIPNetFlag` / `withIPMaskFlag`).
