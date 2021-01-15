##
# Suil project helper functions
##
function(SuilProject name)
    if (SUIL_PROJECT_CREATED)
        message(FATAL_ERROR
                "SuilProject already invoked with name=${SUIL_PROJECT_NAME}")
    endif()

    set(kvargs  VERSION SCC_OUTDIR)
    set(kvvargs SUIL_LIBS SCC_SOURCES)
    cmake_parse_arguments(SUIL_PROJECT "" "${kvargs}" "${kvvargs}" ${ARGN})

    include(CheckSymbolExists)
    include(CheckFunctionExists)
    # Include suil CMake utilities
    include(SuilScc)
    include(SuilUtils)
    # Ensure that some libraries exist
    SuilCheckFunctions()
    # OpenSSL and threads are required to build a suil appp
    find_package(OpenSSL REQUIRED)
    add_compile_definitions(SSL_SUPPORTED)
    include_directories(${OpenSSL_INCLUDE_DIRS})
    find_package(Threads REQUIRED)
    # uuid library is also required to build a suil application
    SuilCheckLibrary(uuid
            INCLUDE uuid/uuid.h)
    # Third party dependencies required
    include(3rdParty.cmake)

    # Configure project version
    if (NOT SUIL_PROJECT_VERSION)
        set(SUIL_PROJECT_VERSION "0.1.0")
    endif()
    set(SUIL_PROJECT_VERSION ${SUIL_PROJECT_VERSION} PARENT_SCOPE)

    # Initialize project
    project(${name} VERSION ${SUIL_PROJECT_VERSION} LANGUAGES C CXX)

    set(SUIL_SCC_PLUGINS_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
    if (SUIL_BASE_PATH)
        link_directories(${SUIL_BASE_PATH}/lib)
        include_directories(${SUIL_BASE_PATH}/include)
        set(SUIL_SCC_PLUGINS_DIR ${SUIL_SCC_PLUGINS_DIR} ${SUIL_BASE_PATH}/lib PARENT_SCOPE)
    endif()

    if (NOT SUIL_PROJECT_SCC_OUTDIR)
        set(SUIL_PROJECT_SCC_OUTDIR ${CMAKE_BINARY_DIR}/scc/public)
    endif()
    set(SUIL_PROJECT_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR} PARENT_SCOPE)

    if (SUIL_PROJECT_SCC_SOURCES)
        SuilScc(${name}
            PROJECT  0N
            SOURCES  ${SUIL_PROJECT_SCC_SOURCES}
            LIB_PATH ${SUIL_SCC_PLUGINS_DIR}
            OUTDIR   ${SUIL_PROJECT_SCC_OUTDIR})
    endif()

    set(SUIL_PROJECT_NAME ${name} PARENT_SCOPE)
    set(SUIL_PROJECT_CREATED ON PARENT_SCOPE)
endfunction()

function(SuilTarget name)
    if (NOT SUIL_PROJECT_CREATED)
        # Cannot add an app without creating project
        message(FATAL_ERROR "cannot create an app before creating a project")
    endif()

    set(options DEBUG TEST)
    set(kvargs  SCC_OUTDIR LIBRARY)
    set(kvvargs DEPENDS INSTALL VERSION SOURCES TEST_SOURCES DEFINES SCC_SOURCES
                SUIL_LIBS LIBRARIES INCLUDES INSTALL_FILES INSTALL_DIRS INSTALL_INCDIR)
    cmake_parse_arguments(SUIL_TARGET "${options}" "${kvargs}" "${kvvargs}" ${ARGN})

    # Get the source files
    set(${name}_SOURCES ${SUIL_TARGET_SOURCES})
    if (NOT SUIL_TARGET_SOURCES)
        file(GLOB_RECURSE ${name}_SOURCES src/*.c src/*.cpp src/*.cc)
    endif()
    # If target is a test, add test sources
    if (SUIL_TARGET_TEST)
        set(${name}_SOURCES ${${name}_SOURCES} ${SUIL_TARGET_SOURCES})
    else()
        file(GLOB_RECURSE ${name}_TEST_SOURCES test/*.c test/*.cpp test/*.cc)
        set(${name}_SOURCES ${${name}_SOURCES} ${${name}_TEST_SOURCES})
    endif()
    # Sources are required for this kind of target
    if (NOT ${name}_SOURCES)
        message(FATAL_ERROR "adding application '${name}' without sources")
    endif()

    # Get the application version
    set(${name}_VERSION ${SUIL_TARGET_VERSION})
    if (NOT SUIL_TARGET_VERSION)
        set(${name}_VERSION "${SUIL_PROJECT_VERSION}")
    endif()
    message(STATUS "version configured to: ${${name}_VERSION}")

    # Create our targets
    if (SUIL_TARGET_LIBRARY)
        message(STATUS "Configuring target ${name} as a ${SUIL_TARGET_LIBRARY} library")
        add_library(${name} ${SUIL_TARGET_LIBRARY} ${${name}_SOURCES})
        target_compile_definitions(${name} PUBLIC "-DLIB_VERSION=\"${${name}_VERSION}\"")
    else()
        # default is an executable target
        message(STATUS "Configuring target ${name} an executable")
        add_executable(${name} ${${name}_SOURCES})
        target_compile_definitions(${name} PUBLIC "-DAPP_VERSION=\"${${name}_VERSION}\"")
        target_compile_definitions(${name} PUBLIC "-DAPP_NAME=\"${name}\"")
    endif()

    # Configure suil compilable sources
    if (SUIL_TARGET_SCC_SOURCES)
        if (NOT SUIL_TARGET_SCC_OUTDIR)
            # default to project outdir
            set(SUIL_TARGET_SCC_OUTDIR ${SUIL_PROJECT_SCC_OUTDIR}/${name})
        endif()
        SuilScc(${name}
            SOURCES  ${SUIL_TARGET_SCC_SOURCES}
            LIB_PATH ${SUIL_SCC_PLUGINS_DIR}
            OUTDIR   ${SUIL_TARGET_SCC_OUTDIR})
    endif()

    if (SUIL_TARGET_TEST)
        # Add some compile definitions for test targets
        target_compile_definitions(${name} -DSUIL_TESTING -Dsuil_ut=:public)
    endif()
endfunction()