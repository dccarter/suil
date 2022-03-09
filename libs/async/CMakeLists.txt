cmake_minimum_required(VERSION 3.16)

# The version of the library, configurable from command line
if (NOT SUIL_VERSION)
    set(SUIL_ASYNC_VERSION "0.1.0" CACHE STRING "The version of suil async library")
else()
    set(SUIL_ASYNC_VERSION ${SUIL_VERSION})
endif()

project(Async
        VERSION ${SUIL_ASYNC_VERSION}
        LANGUAGES C CXX)

if (LINUX)
    set(SUIL_PLATFORM_SOURCES
            )
elseif(UNIX)
    set(SUIL_PLATFORM_SOURCES
            src/kqueue.cpp)
endif()

set(SUIL_ASYNC_SOURCES
        ${SUIL_PLATFORM_SOURCES}
        src/event.cpp
        src/delay.cpp
        src/fdwait.cpp
        src/fdops.cpp
        src/addr.cpp
        src/mutex.cpp
        src/scheduler.cpp
        src/socket.cpp
        src/sync.cpp
        src/tcp.cpp
        src/thread.cpp
        src/workqueue.cpp
        src/dns/dns.c)

# Add and configure utils library
SuilAddLibrary(Async
        KIND STATIC
        SOURCES      ${SUIL_ASYNC_SOURCES}
        RENAME       SuilAsync
        LIBS         Suil::Utils Threads::Threads)

if (ENABLE_EXAMPLES)
    SuilExample(Async-TcpEchoServer
            SOURCES         examples/tcpecho.cpp
            LIBS            Suil::Async
            INCLUDES        include
            RENAME          SuilAsyncTcpEchoSvr
            DEFINITIONS     "-DSYX_TCP_ECHO_SVR")

    SuilExample(Async-TcpEchoClient
            SOURCES         examples/tcpecho.cpp
            LIBS            Suil::Async
            INCLUDES        include
            RENAME          SuilAsyncTcpEchoCli)
endif()

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    SuilUnitTest(Async-UnitTest
            SOURCES
                ${SUIL_ASYNC_SOURCES}
                test/main.cpp
            LIBS Threads::Threads
            INCLUDES include
            RENAME  SuilAsyncUt
            )
endif()