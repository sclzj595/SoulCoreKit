#include <QTest>
#include <QApplication>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Starting (no soul libs)..." << std::endl;
    QApplication app(argc, argv);
    std::cout << "OK" << std::endl;
    return 0;
}