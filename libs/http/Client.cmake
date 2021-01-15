
set(SUIL_HTTP_CLIENT_SOURCES
        src/common.cpp
        src/cookie.cpp
        src/jwt.cpp
        src/offload.cpp
        src/parser.cpp
        src/passwd.cpp
        src/client/fileoffload.cpp
        src/client/form.cpp
        src/client/memoffload.cpp
        src/client/request.cpp
        src/client/response.cpp
        src/client/session.cpp
        src/llhttp/api.c
        src/llhttp/http.c
        src/llhttp/llhttp.c
        ${CMAKE_BINARY_DIR}/scc/public/suil/http/jwt.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/http/route.scc.cpp)

add_library(SuilHttpClient STATIC
        ${SUIL_HTTP_CLIENT_SOURCES})

target_link_libraries(SuilHttpClient
        PUBLIC SuilNet)

target_include_directories(SuilHttpClient PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>)
target_include_directories(SuilHttpClient PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilHttpClient-UnitTest
            SOURCES ${SUIL_HTTP_CLIENT_SOURCES} test/main.cpp
            LIBS  SuilNet)
    target_include_directories(SuilHttpClient-UnitTest
            PRIVATE include ${CMAKE_BINARY_DIR}/scc/public)
    add_dependencies(SuilHttpClient-UnitTest SuilHttp-scc)

    set_target_properties(SuilHttpClient-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME httpclient_unittest)
endif()