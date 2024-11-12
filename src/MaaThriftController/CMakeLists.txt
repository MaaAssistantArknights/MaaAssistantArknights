GENERATE_THRIFT_LIB(MaaThriftController
                    ${PROJECT_SOURCE_DIR}/include/interfaces/ThriftController.thrift
                    ${CMAKE_CURRENT_BINARY_DIR}/MaaThriftController maa_thrift_controller_src)

source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${maa_thrift_controller_src})
