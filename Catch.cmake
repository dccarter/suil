set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps)

include(ExternalProject)

ExternalProject_Add(catch
        PREFIX ${EP_PREFIX}/catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG "v2.13.1"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR};-DCATCH_BUILD_EXAMPLES=OFF;-DCATCH_BUILD_TESTING=OFF;-DCATCH_INSTALL_DOCS=OFF;-DCATCH_INSTALL_HELPERS=OFF")
set_target_properties(catch PROPERTIES EXCLUDE_FROM_ALL True)

include_directories(${EP_INSTALL_DIR}/include)