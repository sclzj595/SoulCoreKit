#include <QCoreApplication>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Starting QCoreApplication..." << std::endl;
    QCoreApplication app(argc, argv);
    std::cout << "OK" << std::endl;
    return 0;
}