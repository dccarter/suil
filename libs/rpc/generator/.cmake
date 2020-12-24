# Configure generator
set(RPC_GENERATOR_SOURCES
        generator/jrpc.cpp
        generator/srpc.cpp
        generator/rpc.cpp)

SuilSccGenerator(SuilRpc-Generator
        OUTPUT_NAME rpc
        SOURCES ${RPC_GENERATOR_SOURCES})

set_target_properties(SuilRpc-Generator
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (ENABLE_EXAMPLES)
    add_executable(SuilRpc-Example
            generator/example/kvstore.cpp
            ${CMAKE_BINARY_DIR}/scc/suil/rpc/generator/kvstore.scc.cpp
            generator/example/main.cpp)

    SuilScc(SuilRpc-Example
            SOURCES  generator/example/kvstore.scc
            DEPENDS  SuilRpc-Generator
            OUTDIR   ${CMAKE_BINARY_DIR}/scc/suil/rpc/generator
            LIB_PATH ${CMAKE_BINARY_DIR})
    set_target_properties(SuilRpc-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME rpc-ex)
    target_link_libraries(SuilRpc-Example SuilRpc SuilNet SuilBase)
    target_include_directories(SuilRpc-Example PRIVATE generator/example)
endif()