# Configure generator
include(SuilScc)

set(SBG_SOURCES
        generator/meta.cpp)

SuilSccGenerator(Base-Generator
        OUTPUT_NAME sbg
        SOURCES ${SBG_SOURCES})

set_target_properties(Base-Generator
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (ENABLE_EXAMPLES)
    add_executable(Base-GeneratorExample
            generator/example/demo.scc.cpp
            generator/example/main.cpp)
    SuilScc(Base-GeneratorExample
            SOURCES generator/example/demo.scc
            DEPENDS Base-Generator
            LIB_PATH ${CMAKE_BINARY_DIR})
    target_link_libraries(Base-GeneratorExample
            PRIVATE Suil::Base)
    set_target_properties(Base-GeneratorExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME base-generator-ex)
endif()