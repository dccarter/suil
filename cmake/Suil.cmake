if (NOT SUIL_INCLUDED)
    set(SUIL_INCLUDED ON)

    set(CMAKE_MODULE_PATH
            ${CMAKE_MODULE_PATH}
            /usr/local/share/suil/cmake
            /usr/share/suil/cmake)

    # Configure project for C++20 support
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Force cmake to look for modules on the provided path
    if (SUIL_BASE_PATH)
        set(CMAKE_PREFIX_PATH ${SUIL_BASE_PATH}/lib/cmake)
    endif()

    ##
    # Suil project helper functions
    ##
    function(SuilProject name)
        if (SUIL_PROJECT_CREATED)
            message(FATAL_ERROR
                    "SuilProject already invoked with name=${SUIL_PROJECT_NAME}")
        endif()

        set(options EXPORTS)
        set(kvargs  SCC_OUTDIR)
        set(kvvargs SUIL_LIBS SCC_SOURCES PRIV_SCC_SOURCES)
        cmake_parse_arguments(SUIL_PROJECT "${options}" "${kvargs}" "${kvvargs}" ${ARGN})

        include(CheckSymbolExists)
        include(CheckFunctionExists)
        # Include suil CMake utilities
        include(SuilScc)
        include(SuilUtils)

        # Find find all libraries required to create a Suil application/library
        find_package(OpenSSL REQUIRED)
        find_package(Threads REQUIRED)
        find_package(LuaS REQUIRED)
        find_package(Uuid REQUIRED)
        find_package(Dl REQUIRED)
        find_package(PostgreSQL REQUIRED)
        find_package(Libmill REQUIRED)
        find_package(Suil)

        if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty.cmake)
            # Third party dependencies required
            include(3rdParty.cmake)
        endif()

        # Configure project version
        set(SUIL_PROJECT_VERSION ${CMAKE_PROJECT_VERSION} PARENT_SCOPE)

        set(SUIL_SCC_PLUGINS_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
        if (SUIL_BASE_PATH)
            set(SUIL_SCC_PLUGINS_DIR ${SUIL_SCC_PLUGINS_DIR} ${SUIL_BASE_PATH}/lib PARENT_SCOPE)
        endif()

        if (NOT SUIL_PROJECT_SCC_OUTDIR)
            set(SUIL_PROJECT_SCC_OUTDIR ${CMAKE_BINARY_DIR}/scc)
        endif()
        set(SUIL_PROJECT_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR} PARENT_SCOPE)

        if (SUIL_PROJECT_SCC_SOURCES)
            set(SUIL_PROJECT_SCC_PUB  ${SUIL_PROJECT_SCC_OUTDIR}/public PARENT_SCOPE)
            if (SUIL_PROJECT_SCC_SUFFIX)
                set(SUIL_PROJECT_SCC_PUB ${SUIL_PROJECT_SCC_PUB}/${SUIL_PROJECT_SCC_SUFFIX})
            endif()
            SuilScc(${name}
                PROJECT  0N
                SOURCES       ${SUIL_PROJECT_SCC_SOURCES}
                LIB_PATH      ${SUIL_SCC_PLUGINS_DIR}
                OUTDIR        ${SUIL_PROJECT_SCC_PUB})
        endif()
        if (SUIL_PROJECT_PRIV_SCC_SOURCES)
            set(SUIL_PROJECT_SCC_PRIV ${SUIL_PROJECT_SCC_OUTDIR}/private PARENT_SCOPE)
            SuilScc(${name}
                    PROJECT  0N
                    PRIVATE  ON
                    SOURCES       ${SUIL_PROJECT_PRIV_SCC_SOURCES}
                    LIB_PATH      ${SUIL_SCC_PLUGINS_DIR}
                    OUTDIR        ${SUIL_PROJECT_SCC_PRIV})
        endif()

        set(NAMESPACE ${name}:: PARENT_SCOPE)
        set(TARGETS_EXPORT_NAME  ${name}Targets PARENT_SCOPE)

        if (SUIL_PROJECT_EXPORTS)
            include(CMakePackageConfigHelpers)

            set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
            set(VERSION_CONFIG ${GENERATED_DIR}/${name}ConfigVersion.cmake)
            set(PROJECT_CONFIG ${GENERATED_DIR}/${name}Config.cmake)

            # Install SCC public header files if any
            if (SUIL_PROJECT_SCC_SOURCES)
                install(DIRECTORY ${SUIL_PROJECT_SCC_OUTDIR}/public/
                        DESTINATION include/${name}
                        FILES_MATCHING PATTERN "*.hpp")
            endif()
            # Configure '<PROJECT-NAME>Config.cmake.in'
            configure_package_config_file(
                    ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
                    ${PROJECT_CONFIG}
                    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

            # Configure '<PROJECT-NAME>ConfigVersion.cmake'
            write_basic_package_version_file(
                    "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

            # Install version config
            install(FILES ${PROJECT_CONFIG} ${VERSION_CONFIG}
                    DESTINATION lib/cmake/${name})
        endif()

        set(SUIL_PROJECT_NAME ${name} PARENT_SCOPE)
        set(SUIL_PROJECT_CREATED ON PARENT_SCOPE)
    endfunction()

    function(SuilTarget name)
        if (NOT SUIL_PROJECT_CREATED)
            # Cannot add an app without creating project
            message(FATAL_ERROR "cannot create an app before creating a project")
        endif()
        string(REPLACE "-" "_" _name ${name})

        set(options DEBUG TEST)
        set(kvargs  SCC_OUTDIR LIBRARY)
        set(kvvargs DEPENDS INSTALL VERSION SOURCES SCC_SOURCES PRIV_SCC_SOURCES
                    LIBRARIES PRIV_LIBS INF_LIBS INCLUDES PRIV_INCS INF_INCS
                    DEFINES PRIV_DEFS INF_DEFS INST_HEADERS)
        cmake_parse_arguments(SUIL_TARGET "${options}" "${kvargs}" "${kvvargs}" ${ARGN})

        # Get the source files
        set(${_name}_SOURCES ${SUIL_TARGET_SOURCES})
        if (NOT SUIL_TARGET_SOURCES AND NOT SUIL_TARGET_TEST)
            file(GLOB_RECURSE ${_name}_SOURCES src/*.c src/*.cpp src/*.cc)
            # If target is a test, add test sources
            if (SUIL_TARGET_TEST)
                file(GLOB_RECURSE ${_name}_TEST_SOURCES test/*.c test/*.cpp test/*.cc)
                set(${_name}_SOURCES ${${_name}_SOURCES} ${${_name}_TEST_SOURCES})
            endif()
        endif()

        # Sources are required for this kind of target
        if (NOT ${_name}_SOURCES)
            message(FATAL_ERROR "adding application '${_name}' without sources")
        endif()

        # Get the application version
        set(${_name}_VERSION ${SUIL_TARGET_VERSION})
        if (NOT SUIL_TARGET_VERSION)
            set(${_name}_VERSION "${SUIL_PROJECT_VERSION}")
        endif()
        message(STATUS "version configured to: ${${_name}_VERSION}")

        # Create our targets
        if (SUIL_TARGET_LIBRARY)
            message(STATUS "Configuring target ${_name} as a ${SUIL_TARGET_LIBRARY} library")
            add_library(${name} ${SUIL_TARGET_LIBRARY} ${${_name}_SOURCES})
            target_compile_definitions(${name} PUBLIC "-DLIB_VERSION=\"${${_name}_VERSION}\"")
        else()
            # default is an executable target
            message(STATUS "Configuring target ${_name} an executable")
            add_executable(${name} ${${_name}_SOURCES})
            target_compile_definitions(${name} PRIVATE "-DAPP_VERSION=\"${${_name}_VERSION}\"")
            target_compile_definitions(${name} PRIVATE "-DAPP_NAME=\"${_name}\"")
        endif()

        # Configure suil compilable sources
        if (SUIL_TARGET_SCC_SOURCES OR SUIL_TARGET_SCC_PRIV_SOURCES)
            if (NOT SUIL_TARGET_SCC_OUTDIR)
                # default to project outdir
                set(SUIL_TARGET_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR}/${_name})
            endif()
            if (SUIL_TARGET_SCC_OUTDIR_PUB_SUFFIX)
                set(SUIL_TARGET_SCC_OUTDIR_PUB_SUFFIX "/${SUIL_TARGET_SCC_OUTDIR_PUB_SUFFIX}")
            endif()
            set(SUIL_${_name}_SCC_PUB  ${SUIL_TARGET_SCC_OUTDIR}/public${SUIL_TARGET_SCC_OUTDIR_PUB_SUFFIX} PARENT_SCOPE)
            set(SUIL_${_name}_SCC_PRIV ${SUIL_TARGET_SCC_OUTDIR}/private PARENT_SCOPE)

            SuilScc(${name}
                SOURCES      ${SUIL_TARGET_SCC_SOURCES}
                PRIV_SOURCES ${SUIL_TARGET_SCC_PRIV_SOURCES}
                LIB_PATH     ${SUIL_SCC_PLUGINS_DIR}
                OUTDIR       ${SUIL_TARGET_SCC_OUTDIR})
            target_include_directories(${name}
                    PRIVATE ${SUIL_TARGET_SCC_OUTDIR}/public
                            ${SUIL_TARGET_SCC_OUTDIR}/private)
        endif()

        if (SUIL_TARGET_TEST)
            # Add some compile definitions for test targets
            target_compile_definitions(${name} PRIVATE -DSUIL_TESTING -Dsuil_ut=:public)
        endif()

        if (SUIL_TARGET_DEFINES)
            target_compile_definitions(${name} PUBLIC ${SUIL_TARGET_DEFINES})
        endif()

        if (SUIL_TARGET_LIBRARIES)
            target_link_libraries(${name} PUBLIC ${SUIL_TARGET_LIBRARIES})
        endif()

        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        set(${_name}_INCLUDES ${SUIL_TARGET_INCLUDES})
        if (NOT SUIL_TARGET_INCLUDES)
            if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${_name})
                set(${_name}_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/${_name})
                target_include_directories(${name} PUBLIC
                        $<BUILD_INTERFACE:${${_name}_INCLUDES}>
                        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
            endif()
        else()
            target_include_directories(${name} PUBLIC ${${_name}_INCLUDES})
        endif()

        if (SUIL_TARGET_LIBRARY)
            if (SUIL_TARGET_PRIV_LIBS)
                target_link_libraries(${name} PRIVATE ${SUIL_TARGET_PRIV_LIBS})
            endif()

            if (SUIL_TARGET_INF_LIBS)
                target_link_libraries(${name} INTERFACE ${SUIL_TARGET_INF_LIBS})
            endif()

            if (SUIL_TARGET_PRIV_INCS)
                target_include_directories(${name} PRIVATE ${SUIL_TARGET_PRIV_INCS})
            endif()

            if (SUIL_TARGET_INF_INCS)
                target_include_directories(${name} INTERFACE ${SUIL_TARGET_INF_INCS})
            endif()

            if (SUIL_TARGET_PRIV_DEFS)
                target_compile_definitions(${name} PRIVATE ${SUIL_TARGET_PRIV_DEFS})
            endif()

            if (SUIL_TARGET_INF_DEFS)
                target_compile_definitions(${name} INTERFACE ${SUIL_TARGET_INF_DEFS})
            endif()
        endif()

        if (SUIL_TARGET_INSTALL)
            if (SUIL_TARGET_LIBRARY)
                if (NOT SUIL_TARGETS_EXPORTED)
                    # Install target exports
                    install(EXPORT      ${TARGETS_EXPORT_NAME}
                            NAMESPACE   ${NAMESPACE}
                            DESTINATION lib/cmake/${name})
                    set(SUIL_TARGETS_EXPORTED ON PARENT_SCOPE)
                endif()

                # Install library in exported target
                install(TARGETS ${name}
                        EXPORT  ${TARGETS_EXPORT_NAME}
                        ARCHIVE DESTINATION lib
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION bin)

                # Install headers for target
                if (SUIL_TARGET_INST_HEADERS AND ${_name}_INCLUDES)
                    install(DIRECTORY ${_name}_HEADERS
                            DESTINATION include/${_name}
                            FILES_MATCHING "${SUIL_TARGET_INST_HEADERS}")
                endif()

                # Install public SCC sources
                if (SUIL_TARGET_SCC_SOURCES)
                    install(DIRECTORY ${SUIL_TARGET_SCC_OUTDIR}/${_name}/public
                            DESTINATION include/${_name}
                            FILES_MATCHING "*.hpp")
                endif()
            else()
                # Install executable targets
                install(TARGETS ${name}
                        ARCHIVE DESTINATION lib
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION bin)
            endif()
        endif()
    endfunction()

    macro(SuilApp name)
        SuilTarget(${name}
            ${ARGN})
    endmacro()

    macro(SuilShared name)
        SuilTarget(${name}
                ${ARGN}
                LIBRARY SHARED)
    endmacro()

    macro(SuilStatic name)
        SuilTarget(${name}
                ${ARGN}
                LIBRARY STATIC)
    endmacro()

    macro(SuilTest name)
        SuilTarget(${name}-test
                ${ARGN}
                TEST    ON
                INSTALL OFF)
    endmacro()

endif()
