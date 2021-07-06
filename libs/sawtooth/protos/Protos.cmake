set(Protobuf_DEBUG ON)
set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf QUIET)

if(Protobuf_VERSION)
    message(STATUS "Using Protocol Buffers ${Protobuf_VERSION}")
endif()

set(SAWTOOTH_PROTOS
        protos/authorization.proto
        protos/batch.proto
        protos/block.proto
        protos/block_info.proto
        protos/client_batch.proto
        protos/client_batch_submit.proto
        protos/client_block.proto
        protos/client_event.proto
        protos/client_list_control.proto
        protos/client_peers.proto
        protos/client_receipt.proto
        protos/client_state.proto
        protos/client_status.proto
        protos/client_transaction.proto
        protos/consensus.proto
        protos/events.proto
        protos/genesis.proto
        protos/identity.proto
        protos/merkle.proto
        protos/network.proto
        protos/processor.proto
        protos/setting.proto
        protos/state_context.proto
        protos/transaction.proto
        protos/transaction_receipt.proto
        protos/validator.proto)
protobuf_generate_cpp(SAWTOOTH_PROTO_SRCS SAWTOOTH_PROTO_HDRS ${SAWTOOTH_PROTOS})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sawtooth)
set(SAWTOOTH_PROTOS_ALL ${CMAKE_CURRENT_BINARY_DIR}/protos.hpp)
set(SAWTOOTH_PROTOS_DEV ${CMAKE_CURRENT_BINARY_DIR}/suil/sawtooth/protos.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(WRITE ${SAWTOOTH_PROTOS_ALL} "#ifndef __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")
file(APPEND ${SAWTOOTH_PROTOS_ALL} "#define __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")

file(WRITE ${SAWTOOTH_PROTOS_DEV} "#ifndef __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")
file(APPEND ${SAWTOOTH_PROTOS_DEV} "#define __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")
foreach(HEADER ${SAWTOOTH_PROTO_HDRS})
    get_filename_component(HEADER_NAME ${HEADER} NAME)
    file(APPEND ${SAWTOOTH_PROTOS_ALL} "#include <suil/sawtooth/protos/${HEADER_NAME}>\n")
    file(APPEND ${SAWTOOTH_PROTOS_DEV} "#include \"../../${HEADER_NAME}\"\n")
endforeach()
file(APPEND ${SAWTOOTH_PROTOS_ALL} "#endif // __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")
file(APPEND ${SAWTOOTH_PROTOS_DEV} "#endif // __SUIL_SAWTOOTH_PROTOS_AMALGAMATE_H_\n")

install(FILES ${SAWTOOTH_PROTO_HDRS}
        DESTINATION include/suil/sawtooth/protos)
install(FILES ${SAWTOOTH_PROTOS_ALL}
        DESTINATION include/suil/sawtooth/)