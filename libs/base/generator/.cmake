# Configure generator
include(SuilScc)

set(SBG_SOURCES
        generator/meta.cpp)

SuilSccGenerator(SuilBase-Generator
        OUTPUT_NAME sbg
        SOURCES ${SBG_SOURCES})

set_target_properties(SuilBase-Generator
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (ENABLE_EXAMPLES)
    add_executable(SuilBase-GeneratorExample
            generator/example/demo.scc.cpp
            generator/example/main.cpp)
    SuilScc(SuilBase-GeneratorExample
            SOURCES generator/example/demo.scc
            DEPENDS SuilBase-Generator
            LIB_PATH ${CMAKE_BINARY_DIR})
    target_link_libraries(SuilBase-GeneratorExample SuilBase mill)
    set_target_properties(SuilBase-GeneratorExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME base-generator-ex)
endif()