set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps)

include(ExternalProject)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/3rdParty)
set(EP_PREFIX  ${CMAKE_BINARY_DIR}/3rdParty)

ExternalProject_Add(catch
        PREFIX ${EP_PREFIX}/catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG "v2.13.1"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR};-DCATCH_BUILD_EXAMPLES=OFF;-DCATCH_BUILD_TESTING=OFF;-DCATCH_INSTALL_DOCS=OFF;-DCATCH_INSTALL_HELPERS=OFF")
set_target_properties(catch PROPERTIES EXCLUDE_FROM_ALL True)

ExternalProject_Add(iod
        PREFIX ${EP_PREFIX}/iod
        GIT_REPOSITORY https://gitlab.com/sw-devel/thirdparty/iod.git
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR}")
set_target_properties(iod PROPERTIES EXCLUDE_FROM_ALL True)

include_directories(${EP_INSTALL_DIR}/include)
link_directories(${EP_INSTALL_DIR}/lib)
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH} ${EP_INSTALL_DIR}/share/cmake/Modules)

add_custom_target(deps
        DEPENDS catch iod)

set_target_properties(deps PROPERTIES EXCLUDE_FROM_ALL True)