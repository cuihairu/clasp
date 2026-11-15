# vcpkg Port Draft

This directory is a draft for a future vcpkg port.

The port pins `v0.1.2`: `REF` and `SHA512` in `portfile.cmake` and
`version-semver` in `vcpkg.json` are filled for that tag. When bumping the
version, update all three together and recompute the hash from the tag
archive (`curl -sL https://github.com/cuihairu/clasp/archive/refs/tags/vX.Y.Z.tar.gz | sha512sum`).
