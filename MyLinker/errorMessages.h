#pragma once
#include <optional>
#include <iostream>
#include <string>

std::nullopt_t errorMessageOpt(const std::string& message) {
    std::cerr << "Error: " << message << '\n';
    return std::nullopt;
}
bool errorMessageBool(const std::string& message) {
    std::cerr << "Error: " << message << '\n';
    return false;
}
void warningMessage(const std::string& message) {
    std::cerr << "Warning: " << message << '\n';
}