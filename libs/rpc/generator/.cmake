# Configure generator
set(RPC_GENERATOR_SOURCES
        generator/jrpc.cpp
        generator/srpc.cpp
        generator/rpc.cpp)

SuilSccGenerator(Rpc-Generator
        OUTPUT_NAME rpc
        SOURCES ${RPC_GENERATOR_SOURCES})

set_target_properties(Rpc-Generator
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (ENABLE_EXAMPLES)
    add_executable(Rpc-Example
            generator/example/kvstore.cpp
            ${CMAKE_BINARY_DIR}/scc/private/suil/rpc/generator/kvstore.scc.cpp
            generator/example/main.cpp)

    SuilScc(Rpc-Example
            SOURCES  generator/example/kvstore.scc
            DEPENDS  Rpc-Generator scc-bin
            OUTDIR   ${CMAKE_BINARY_DIR}/scc/private/suil/rpc/generator
            LIB_PATH ${CMAKE_BINARY_DIR})
    set_target_properties(Rpc-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME rpc-ex)
    target_link_libraries(Rpc-Example Suil::Rpc Suil::Scc)
    target_include_directories(Rpc-Example
            PRIVATE generator/example
                    ${CMAKE_BINARY_DIR}/scc/public
                    ${CMAKE_BINARY_DIR}/scc/private)
endif()