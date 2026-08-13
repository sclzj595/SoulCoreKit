# mq.cmake — 消息队列模块 [v2.5.0]
# Layer: Extensions (E02) — 企业级扩展
# 依赖: core + logging + async
# 职责: MessageBus 抽象 + InMemory 核心实现 + RabbitMQ/Kafka/RocketMQ Adapter
#       CS 小型项目用 InMemory，BS 企业后端按需接入真实 MQ

add_library(soul_mq STATIC
    src/soul/mq/amqpcpp_backend.cpp
    src/soul/mq/inmemory_amqp_backend.cpp
    src/soul/mq/kafka_adapter.cpp
    src/soul/mq/module.cpp
    src/soul/mq/mq_factory.cpp
    src/soul/mq/rabbitmq/rabbitmq_connection.cpp
    src/soul/mq/rabbitmq/rabbitmq_consumer.cpp
    src/soul/mq/rabbitmq/rabbitmq_producer.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/mq/rabbitmq/rabbitmq_connection.h
    include/soul/mq/rabbitmq/rabbitmq_consumer.h
    include/soul/mq/rabbitmq/rabbitmq_producer.h
    include/soul/mq/amqpcpp_backend.h
)

target_include_directories(soul_mq PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_mq PUBLIC soul_core soul_logging soul_async)