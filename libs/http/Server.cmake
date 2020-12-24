
set(SUIL_HTTP_SERVER_SOURCES
        src/common.cpp
        src/cookie.cpp
        src/jwt.cpp
        src/offload.cpp
        src/parser.cpp
        src/passwd.cpp
        src/server/admin.cpp
        src/server/connection.cpp
        src/server/cors.cpp
        src/server/endpoint.cpp
        src/server/form.cpp
        src/server/initializer.cpp
        src/server/jwtauth.cpp
        src/server/jwtsession.cpp
        src/server/pgsqlmw.cpp
        src/server/qs.cpp
        src/server/redismw.cpp
        src/server/request.cpp
        src/server/response.cpp
        src/server/router.cpp
        src/server/routes.cpp
        src/server/rules.cpp
        src/server/sysattrs.cpp
        src/server/trie.cpp
        src/server/uploaded.cpp
        src/server/websock.cpp
        src/llhttp/api.c
        src/llhttp/http.c
        src/llhttp/llhttp.c
        ${CMAKE_BINARY_DIR}/scc/suil/http/jwt.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/http/server.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/http/route.scc.cpp)

add_library(SuilHttpServer STATIC
        ${SUIL_HTTP_SERVER_SOURCES})

target_link_libraries(SuilHttpServer
        SuilNet SuilDb SuilBase mill ${OPENSSL_LIBRARIES})

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilHttpServer-UnitTest
            SOURCES ${SUIL_HTTP_SERVER_SOURCES} test/main.cpp
            LIBS  SuilDb SuilNet SuilBase mill ssl crypto)
    add_dependencies(SuilHttpServer-UnitTest SuilHttp-scc)

    set_target_properties(SuilHttpServer-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME httpserver_unittest)
endif()