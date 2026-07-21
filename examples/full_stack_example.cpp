#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "soul/mq/mq_factory.h"
#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"
#include "soul/orm/entity.h"
#include "soul/orm/query_wrapper.h"
#include "soul/orm/sqlite_repository.h"
#include "soul/orm/entities.h"

using namespace sc;
using namespace sc::mq;
using namespace sc::orm;

void runFullStackDemo() {
    std::cout << "=== Full Stack Demo Starting ===" << std::endl;

    SQLiteRepository<User> userRepo("example.db");
    userRepo.executeSql("CREATE TABLE IF NOT EXISTS user (id TEXT PRIMARY KEY, username TEXT, email TEXT, password TEXT, role TEXT, create_time TEXT, update_time TEXT, deleted INTEGER DEFAULT 0)");

    User user;
    user.username = "demo_user";
    user.email = "demo@example.com";
    user.password = "encrypted_password";
    user.role = "user";
    auto saveResult = userRepo.save(user);
    std::cout << "User saved: id=" << saveResult.unwrap().id.toStdString() << std::endl;

    auto getResult = userRepo.getById(saveResult.unwrap().id);
    std::cout << "User retrieved: username=" << getResult.unwrap().username.toStdString() << std::endl;

    QueryWrapper query;
    query.eq("username", "demo_user").orderBy("create_time", true);
    auto listResult = userRepo.find(query);
    std::cout << "Users found: " << listResult.unwrap().size() << std::endl;

    auto connection = MQFactory::createConnection(MQType::RabbitMQ);
    ConnectionConfig mqConfig;
    mqConfig.host = "localhost";
    mqConfig.port = 5672;
    auto connectResult = connection->connect(mqConfig);
    std::cout << "MQ connection result: " << connectResult.isOk() << std::endl;

    auto producer = connection->createProducer();
    Message msg;
    msg.topic = "demo.topic";
    msg.body = "Hello from Full Stack Demo!";
    producer->send(msg);
    std::cout << "Message sent to topic: demo.topic" << std::endl;

    auto consumer = connection->createConsumer();
    consumer->subscribe("demo.topic", [](const ConsumeMessage& message) {
        std::cout << "Received message: " << QString(message.body).toStdString() << std::endl;
    });
    consumer->start();
    std::cout << "Consumer started" << std::endl;

    std::cout << "=== Full Stack Demo Completed ===" << std::endl;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QTimer::singleShot(0, []() {
        runFullStackDemo();
        QCoreApplication::exit(0);
    });

    return app.exec();
}