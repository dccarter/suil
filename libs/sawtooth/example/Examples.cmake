  if (ENABLE_EXAMPLES)
      add_executable(Sawtooth-IntKey
              ${CMAKE_CURRENT_SOURCE_DIR}/example/intkey/tp.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/example/intkey/main.cpp)
      set_target_properties(Sawtooth-IntKey
              PROPERTIES
              RUNTIME_OUTPUT_NAME sawtooth-intkey)
      target_link_libraries(Sawtooth-IntKey
              PRIVATE Suil::Sawtooth)
      target_include_directories(Sawtooth-IntKey
              PRIVATE ${CMAKE_BINARY_DIR}/scc/public)


      add_executable(Sawtooth-IntKey-Cli
              ${CMAKE_CURRENT_SOURCE_DIR}/example/intkey/cli/app.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/example/intkey/cli/main.cpp)
      set_target_properties(Sawtooth-IntKey-Cli
              PROPERTIES
              RUNTIME_OUTPUT_NAME sawtooth-intkey-cli)
      target_link_libraries(Sawtooth-IntKey-Cli
              PRIVATE Suil::Sawtooth)
      target_include_directories(Sawtooth-IntKey-Cli
              PRIVATE ${CMAKE_BINARY_DIR}/scc/public)
  endif()