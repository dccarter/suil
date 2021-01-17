
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

add_library(HttpClient STATIC
        ${SUIL_HTTP_CLIENT_SOURCES})
add_library(Suil::HttpClient ALIAS HttpClient)
set_target_properties(HttpClient
    PROPERTIES
        OUTPUT_NAME SuilHttpClient)

target_link_libraries(HttpClient
        PUBLIC Suil::Net)

target_include_directories(HttpClient PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(HttpClient PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(HttpClient-UnitTest
            SOURCES ${SUIL_HTTP_CLIENT_SOURCES} test/main.cpp
            LIBS  Suil::Net)
    target_include_directories(HttpClient-UnitTest
            PRIVATE include ${CMAKE_BINARY_DIR}/scc/public)
    add_dependencies(HttpClient-UnitTest Http-scc)

    set_target_properties(HttpClient-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME httpclient_unittest)
endif()