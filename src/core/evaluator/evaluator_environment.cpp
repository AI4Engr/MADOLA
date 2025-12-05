#include "evaluator.h"
#include <stdexcept>

namespace madola {

void Environment::define(const std::string& name, const Value& value) {
    variables[name] = value;
}

Value Environment::get(const std::string& name) const {
    auto it = variables.find(name);
    if (it == variables.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return it->second;
}

bool Environment::exists(const std::string& name) const {
    return variables.find(name) != variables.end();
}

void Environment::remove(const std::string& name) {
    variables.erase(name);
}

void Environment::defineFunction(const std::string& name, const FunctionDeclaration& func) {
    functions[name] = &func;
}

const FunctionDeclaration* Environment::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it == functions.end()) {
        throw std::runtime_error("Undefined function: " + name);
    }
    return it->second;
}

bool Environment::functionExists(const std::string& name) const {
    return functions.find(name) != functions.end();
}

void Environment::defineAlias(const std::string& aliasName, const std::string& originalName) {
    aliases[aliasName] = originalName;
}

std::string Environment::resolveAlias(const std::string& name) const {
    auto it = aliases.find(name);
    return (it != aliases.end()) ? it->second : name;
}

void Environment::copyFunctionsFrom(const Environment& other) {
    for (const auto& funcPair : other.functions) {
        functions[funcPair.first] = funcPair.second;
    }
}

} // namespace madola