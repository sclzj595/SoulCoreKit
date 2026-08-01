#include <QTest>
#include <QApplication>
#include <iostream>

// Test 1: Just QApplication
int main(int argc, char* argv[]) {
    std::cout << "Starting..." << std::endl;
    QApplication app(argc, argv);
    std::cout << "QApplication created" << std::endl;
    return 0;
}