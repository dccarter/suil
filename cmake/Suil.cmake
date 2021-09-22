if (NOT SUIL_INCLUDED)
    set(SUIL_INCLUDED ON)

    # Configure project for C++20 support
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Force cmake to look for modules on the provided path
    if (SUIL_BASE_PATH)
        set(CMAKE_PREFIX_PATH ${SUIL_BASE_PATH}/lib/cmake)
        set(SUIL_SCC_DEFAULT_BINARY ${SUIL_BASE_PATH}/bin/scc)
        set(SUIL_SCC_DEFAULT_LIB_PATH ${${SUIL_BASE_PATH}/lib})
    endif()

    ##
    # Suil project helper functions
    ##
    function(SuilStartProject name)
        if (SUIL_PROJECT_CREATED)
            message(FATAL_ERROR
                    "SuilProject already invoked with name=${SUIL_PROJECT_NAME}")
        endif()

        set(options EXPORTS)
        set(kvargs  SCC_OUTDIR)
        set(kvvargs SCC_SOURCES PRIV_SCC_SOURCES)
        cmake_parse_arguments(SUIL_PROJECT "${options}" "${kvargs}" "${kvvargs}" ${ARGN})

        include(CheckSymbolExists)
        include(CheckFunctionExists)
        # Include suil CMake utilities
        include(SuilScc)
        include(SuilUtils)

        # Find find all libraries required to create a Suil application/library
        set(Protobuf_DEBUG ON)
        set(Protobuf_USE_STATIC_LIBS ON)
        find_package(Protobuf REQUIRED QUIET)

        find_package(OpenSSL REQUIRED)
        find_package(Threads REQUIRED)
        find_package(LuaS REQUIRED)
        find_package(Uuid REQUIRED)
        find_package(Dl REQUIRED)
        find_package(PostgreSQL REQUIRED)
        find_package(Libmill REQUIRED)
        find_package(Iod REQUIRED)
        find_package(Scc REQUIRED)
        find_package(Zmq REQUIRED)
        find_package(Scc REQUIRED)
        find_package(Secp256k1 REQUIRED)
        find_package(Suil)

        # Configure project version
        set(SUIL_PROJECT_VERSION ${CMAKE_PROJECT_VERSION} PARENT_SCOPE)

        set(SUIL_SCC_PLUGINS_DIR ${CMAKE_CURRENT_BINARY_DIR})
        if (SUIL_BASE_PATH)
            set(SUIL_SCC_PLUGINS_DIR "${SUIL_SCC_PLUGINS_DIR}:${SUIL_BASE_PATH}/lib")
        endif()

        if (NOT SUIL_PROJECT_SCC_OUTDIR)
            set(SUIL_PROJECT_SCC_OUTDIR ${CMAKE_BINARY_DIR}/scc)
        endif()
        set(SUIL_PROJECT_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR} PARENT_SCOPE)

        if (SUIL_PROJECT_SCC_SOURCES)
            set(SUIL_PROJECT_SCC_PUB  ${SUIL_PROJECT_SCC_OUTDIR}/public)
            include_directories(${SUIL_PROJECT_SCC_PUB})
            if (SUIL_PROJECT_SCC_SUFFIX)
                set(SUIL_PROJECT_SCC_PUB ${SUIL_PROJECT_SCC_PUB}/${SUIL_PROJECT_SCC_SUFFIX})
            endif()
            SuilScc(${name}
                PROJECT  ON
                SOURCES       ${SUIL_PROJECT_SCC_SOURCES}
                LIB_PATH      ${SUIL_SCC_PLUGINS_DIR}
                OUTDIR        ${SUIL_PROJECT_SCC_PUB})
        endif()
        if (SUIL_PROJECT_PRIV_SCC_SOURCES)
            set(SUIL_PROJECT_SCC_PRIV ${SUIL_PROJECT_SCC_OUTDIR}/private)
            message(STATUS ${SUIL_PROJECT_SCC_PRIV})
            include_directories(${SUIL_PROJECT_SCC_PRIV})
            SuilScc(${name}
                    PROJECT  ON
                    PRIVATE  ON
                    SOURCES       ${SUIL_PROJECT_PRIV_SCC_SOURCES}
                    LIB_PATH      ${SUIL_SCC_PLUGINS_DIR}
                    OUTDIR        ${SUIL_PROJECT_SCC_PRIV})
        endif()

        set(SUIL_PROJECT_NAMESPACE ${name}:: PARENT_SCOPE)
        set(TARGETS_EXPORT_NAME    ${name}Targets PARENT_SCOPE)
        if (SUIL_PROJECT_EXPORTS)
            include(CMakePackageConfigHelpers)
            set(SUIL_PROJECT_GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
            set(SUIL_PROJECT_VERSION_CONFIG ${SUIL_PROJECT_GENERATED_DIR}/${name}ConfigVersion.cmake PARENT_SCOPE)
            set(SUIL_PROJECT_PROJECT_CONFIG ${SUIL_PROJECT_GENERATED_DIR}/${name}Config.cmake PARENT_SCOPE)
            set(SUIL_PROJECT_EXPORTS ON PARENT_SCOPE)
            set(SUIL_PROJECT_GENERATED_DIR ${SUIL_PROJECT_GENERATED_DIR} PARENT_SCOPE)
        endif()

        set(SUIL_PROJECT_SCC_PUB  ${SUIL_PROJECT_SCC_PUB} PARENT_SCOPE)
        set(SUIL_PROJECT_SCC_PRIV ${SUIL_PROJECT_SCC_PRIV} PARENT_SCOPE)
        set(SUIL_SCC_PLUGINS_DIR  ${SUIL_SCC_PLUGINS_DIR} PARENT_SCOPE)
        set(SUIL_PROJECT_NAME ${name} PARENT_SCOPE)
        set(SUIL_PROJECT_CREATED ON PARENT_SCOPE)
        set(SUIL_PROJECT_NAMESPACE ${name}:: PARENT_SCOPE)
    endfunction()

    function(SuilEndProject)
        if (SUIL_PROJECT_EXPORTS)
            # Install SCC public header files if any
            if (SUIL_PROJECT_SCC_PUB)
                install(DIRECTORY ${SUIL_PROJECT_SCC_PUB}/
                        DESTINATION include/${PROJECT_NAME}
                        FILES_MATCHING PATTERN "*.hpp")
            endif()
            # Configure '<PROJECT-NAME>Config.cmake.in'
            configure_package_config_file(
                    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
                    ${SUIL_PROJECT_PROJECT_CONFIG}
                    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

            # Configure '<PROJECT-NAME>ConfigVersion.cmake'
            write_basic_package_version_file(
                    "${SUIL_PROJECT_VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

            # Install version config
            install(FILES ${SUIL_PROJECT_PROJECT_CONFIG} ${SUIL_PROJECT_VERSION_CONFIG}
                    DESTINATION lib/cmake/${PROJECT_NAME})
            if (SUIL_PROJECT_HAS_EXPORTS)
                install(
                    EXPORT      ${TARGETS_EXPORT_NAME}
                    NAMESPACE   ${SUIL_PROJECT_NAMESPACE}
                    DESTINATION lib/cmake/${PROJECT_NAME})
            endif()
        endif()
    endfunction()

    function(SuilTarget name)
        if (NOT SUIL_PROJECT_CREATED)
            # Cannot add an app without creating project
            message(FATAL_ERROR "cannot create an app before creating a project")
        endif()
        string(REPLACE "-" "_" _name ${name})

        set(options DEBUG TEST)
        set(kvargs  SCC_OUTDIR SCC_OPS LIBRARY)
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

        if (SUIL_TARGET_SCC_SOURCES OR SUIL_TARGET_SCC_PRIV_SOURCES)
            if (NOT SUIL_TARGET_SCC_OUTDIR)
                # default to project outdir
                set(SUIL_TARGET_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR}/${_name})
            endif()
            if (SUIL_TARGET_SCC_OPS)
                set(SUIL_TARGET_SCC_OPS "/${SUIL_TARGET_SCC_OPS}")
            endif()
            set(SUIL_${_name}_SCC_PUB  ${SUIL_TARGET_SCC_OUTDIR}/public${SUIL_TARGET_SCC_OPS})
            set(SUIL_${_name}_SCC_PRIV ${SUIL_TARGET_SCC_OUTDIR}/private)
        endif()

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
        if (SUIL_TARGET_SCC_SOURCES)
            SuilScc(${name}
                SOURCES      ${SUIL_TARGET_SCC_SOURCES}
                LIB_PATH     ${SUIL_SCC_PLUGINS_DIR}
                OUTDIR       ${SUIL_${_name}_SCC_PUB})
            target_include_directories(${name}
                    PRIVATE ${SUIL_${_name}_SCC_PUB})
        endif()

        if (SUIL_TARGET_SCC_PRIV_SOURCES)
            SuilScc(${name}
                PRIVATE      ON
                SOURCES      ${SUIL_TARGET_PRIV_SCC_SOURCES}
                LIB_PATH     ${SUIL_SCC_PLUGINS_DIR}
                OUTDIR       ${SUIL_${_name}_SCC_PRIV})
            target_include_directories(${name}
                    PRIVATE ${SUIL_${_name}_SCC_PRIV})
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

        if (SUIL_TARGET_DEPENDS)
            add_dependencies(${name} ${SUIL_TARGET_DEPENDS})
        endif()

        if (SUIL_TARGET_INSTALL)
            if (SUIL_TARGET_LIBRARY)
                set(SUIL_PROJECT_HAS_EXPORTS ON PARENT_SCOPE)
                # Install library in exported target
                install(TARGETS ${name}
                        EXPORT  ${TARGETS_EXPORT_NAME}
                        ARCHIVE DESTINATION lib
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION bin)

                # Install headers for target
                if (SUIL_TARGET_INST_HEADERS AND ${_name}_INCLUDES)
                    install(DIRECTORY ${${_name}_INCLUDES}
                            DESTINATION include/${_name}
                            FILES_MATCHING PATTERN "${SUIL_TARGET_INST_HEADERS}")
                endif()

                # Install public SCC sources
                if (SUIL_TARGET_SCC_SOURCES)
                    install(DIRECTORY ${SUIL_${_name}_SCC_PUB}/
                            DESTINATION include/${_name}
                            FILES_MATCHING PATTERN "*.hpp")
                endif()
            else()
                # Install executable targets
                install(TARGETS ${name}
                        ARCHIVE DESTINATION lib
                        LIBRARY DESTINATION lib
                        RUNTIME DESTINATION bin)
            endif()
        endif()
        set(SUIL_${_name}_SCC_PUB  ${SUIL_${_name}_SCC_PUB} PARENT_SCOPE)
        set(SUIL_${_name}_SCC_PRIV ${SUIL_${_name}_SCC_PRIV} PARENT_SCOPE)
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
