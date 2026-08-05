# mq.cmake — 消息队列模块 [v2.5.0]
# 依赖: core + logging + async

add_library(soul_mq STATIC
    src/soul/mq/amqpcpp_backend.cpp
    src/soul/mq/inmemory_amqp_backend.cpp
    src/soul/mq/module.cpp
    src/soul/mq/mq_factory.cpp
    src/soul/mq/rabbitmq/rabbitmq_connection.cpp
    src/soul/mq/rabbitmq/rabbitmq_consumer.cpp
    src/soul/mq/rabbitmq/rabbitmq_producer.cpp
)

target_include_directories(soul_mq PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_mq PUBLIC soul_core soul_logging soul_async)