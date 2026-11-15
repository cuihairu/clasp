vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cuihairu/clasp
    REF v0.1.2
    SHA512 88f8c222f27356832b2f5b2c93c0475230efd686af2600d2d90fd7d86379b093b8c5cd1303b0d89b079d7c70f9cb6461a361957e274a6634e6af8c7cb7072341
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCLASP_BUILD_EXAMPLES=OFF
        -DCLASP_ENABLE_COVERAGE=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME clasp
    CONFIG_PATH lib/cmake/clasp
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
