#include <iostream>
#include "soul/core/uuid.h"

int main() {
    std::cout << "=== Simple Test ===" << std::endl;
    
    auto uuid = sc::Uuid::generate();
    std::cout << "Generated UUID: " << uuid << std::endl;
    
    std::cout << "Test passed!" << std::endl;
    return 0;
}