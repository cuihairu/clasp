vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cuihairu/clasp
    REF v0.1.0
    SHA512 0
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
