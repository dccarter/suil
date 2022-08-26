
add_executable(Http-Benchmark
        tfb/benchmark/src/main.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/http/benchmark/app.scc.cpp)

target_link_libraries(Http-Benchmark Suil::HttpServer)
target_include_directories(Http-Benchmark PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public
        ${CMAKE_BINARY_DIR}/scc/public/suil/http/benchmark)

target_compile_definitions(Http-Benchmark PRIVATE -DSUIL_BENCH_DEV)

SuilScc(Http-Benchmark
        SOURCES  tfb/benchmark/src/app.scc
        LIB_PATH ${CMAKE_BINARY_DIR}
        DEPENDS  Base-Generator
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/http/benchmark)
