#include "unit_system.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <regex>
#include <cmath>
#include <algorithm>
#include <vector>

namespace madola {

std::string UnitValue::toString() const {
    std::stringstream ss;
    // Format as integer if it's a whole number
    if (value == static_cast<int>(value)) {
        ss << static_cast<int>(value);
    } else {
        // Format with 3 decimal places, removing trailing zeros
        ss << std::fixed << std::setprecision(3) << value;
        std::string numStr = ss.str();
        numStr.erase(numStr.find_last_not_of('0') + 1, std::string::npos);
        if (numStr.back() == '.') numStr.pop_back();
        ss.str("");
        ss << numStr;
    }
    if (!unit.empty()) {
        ss << " " << unit;
    }
    return ss.str();
}

std::string UnitValue::toLatex() const {
    std::stringstream ss;
    // Format as integer if it's a whole number
    if (value == static_cast<int>(value)) {
        ss << static_cast<int>(value);
    } else {
        // Format with 3 decimal places, removing trailing zeros
        ss << std::fixed << std::setprecision(3) << value;
        std::string numStr = ss.str();
        numStr.erase(numStr.find_last_not_of('0') + 1, std::string::npos);
        if (numStr.back() == '.') numStr.pop_back();
        ss.str("");
        ss << numStr;
    }
    if (!unit.empty()) {
        // Format compound units properly for LaTeX
        std::string latexUnit = unit;

        // Handle division (fractions) first - e.g., "m/s" or "kg/m^3"
        size_t divPos = latexUnit.find('/');
        if (divPos != std::string::npos) {
            std::string numerator = latexUnit.substr(0, divPos);
            std::string denominator = latexUnit.substr(divPos + 1);

            // Format numerator
            std::string numLatex;
            if (numerator.find('*') != std::string::npos) {
                // Multiple units in numerator
                std::stringstream numSs;
                std::string currentUnit;
                for (char c : numerator) {
                    if (c == '*') {
                        if (!currentUnit.empty()) {
                            numSs << "\\text{" << currentUnit << "} \\cdot ";
                            currentUnit.clear();
                        }
                    } else {
                        currentUnit += c;
                    }
                }
                if (!currentUnit.empty()) {
                    numSs << "\\text{" << currentUnit << "}";
                }
                numLatex = numSs.str();
            } else {
                numLatex = "\\text{" + numerator + "}";
            }

            // Format denominator
            std::string denomLatex;
            if (denominator.find('*') != std::string::npos || denominator.find('^') != std::string::npos) {
                // Multiple units or exponents in denominator
                std::stringstream denomSs;
                std::string currentUnit;
                for (size_t i = 0; i < denominator.length(); ++i) {
                    char c = denominator[i];
                    if (c == '*') {
                        if (!currentUnit.empty()) {
                            denomSs << "\\text{" << currentUnit << "} \\cdot ";
                            currentUnit.clear();
                        }
                    } else if (c == '^') {
                        // Handle exponent
                        if (!currentUnit.empty()) {
                            denomSs << "\\text{" << currentUnit << "}";
                            currentUnit.clear();
                        }
                        denomSs << "^";
                        // Get the exponent number
                        if (i + 1 < denominator.length()) {
                            denomSs << denominator[++i];
                        }
                    } else {
                        currentUnit += c;
                    }
                }
                if (!currentUnit.empty()) {
                    denomSs << "\\text{" << currentUnit << "}";
                }
                denomLatex = denomSs.str();
            } else {
                denomLatex = "\\text{" + denominator + "}";
            }

            ss << " \\dfrac{" << numLatex << "}{" << denomLatex << "}";
        } else if (latexUnit.find('*') != std::string::npos) {
            // Handle compound units like "mm*cm" (no division)
            std::stringstream unitSs;
            std::stringstream currentUnit;

            for (char c : latexUnit) {
                if (c == '*') {
                    if (currentUnit.tellp() > 0) {
                        unitSs << "\\text{ " << currentUnit.str() << "} \\cdot ";
                        currentUnit.str("");
                        currentUnit.clear();
                    }
                } else {
                    currentUnit << c;
                }
            }

            // Add the last unit
            if (currentUnit.tellp() > 0) {
                unitSs << "\\text{ " << currentUnit.str() << "}";
            }

            latexUnit = unitSs.str();
            ss << " " << latexUnit;
        } else {
            // Simple unit
            latexUnit = "\\text{ " + latexUnit + "}";
            ss << " " << latexUnit;
        }
    }
    return ss.str();
}

UnitValue UnitValue::operator+(const UnitValue& other) const {
    auto& unitSys = UnitSystem::getInstance();

    if (isDimensionless() && other.isDimensionless()) {
        return UnitValue(value + other.value);
    }

    if (isDimensionless() || other.isDimensionless()) {
        throw std::runtime_error("Cannot add dimensionless value to value with units");
    }

    if (!unitSys.areUnitsCompatible(unit, other.unit)) {
        throw std::runtime_error("Cannot add incompatible units: " + unit + " + " + other.unit);
    }

    // Convert to same unit (use first operand's unit)
    double otherValueConverted = other.value * unitSys.getConversionFactor(other.unit) / unitSys.getConversionFactor(unit);
    return UnitValue(value + otherValueConverted, unit);
}

UnitValue UnitValue::operator-(const UnitValue& other) const {
    auto& unitSys = UnitSystem::getInstance();

    if (isDimensionless() && other.isDimensionless()) {
        return UnitValue(value - other.value);
    }

    if (isDimensionless() || other.isDimensionless()) {
        throw std::runtime_error("Cannot subtract dimensionless value from value with units");
    }

    if (!unitSys.areUnitsCompatible(unit, other.unit)) {
        throw std::runtime_error("Cannot subtract incompatible units: " + unit + " - " + other.unit);
    }

    // Convert to same unit (use first operand's unit)
    double otherValueConverted = other.value * unitSys.getConversionFactor(other.unit) / unitSys.getConversionFactor(unit);
    return UnitValue(value - otherValueConverted, unit);
}

UnitValue UnitValue::operator*(const UnitValue& other) const {
    if (isDimensionless()) {
        return UnitValue(value * other.value, other.unit);
    }
    if (other.isDimensionless()) {
        return UnitValue(value * other.value, unit);
    }

    // Multiply units and simplify
    std::string resultUnit = unit + "*" + other.unit;
    resultUnit = UnitSystem::getInstance().simplifyUnit(resultUnit);
    return UnitValue(value * other.value, resultUnit);
}

UnitValue UnitValue::operator/(const UnitValue& other) const {
    if (other.value == 0.0) {
        throw std::runtime_error("Division by zero");
    }

    if (isDimensionless()) {
        // Dimensionless / unit = 1/unit
        std::string resultUnit = other.isDimensionless() ? "" : "1/" + other.unit;
        return UnitValue(value / other.value, resultUnit);
    }
    if (other.isDimensionless()) {
        return UnitValue(value / other.value, unit);
    }

    // Divide units and simplify
    std::string resultUnit = unit + "/" + other.unit;
    resultUnit = UnitSystem::getInstance().simplifyUnit(resultUnit);
    return UnitValue(value / other.value, resultUnit);
}

UnitValue UnitValue::operator^(const UnitValue& other) const {
    if (!other.isDimensionless()) {
        throw std::runtime_error("Exponent must be dimensionless");
    }

    double result = std::pow(value, other.value);

    if (isDimensionless()) {
        return UnitValue(result);
    }

    // For now, represent power units as unit^power
    std::string resultUnit = unit + "^" + std::to_string(static_cast<int>(other.value));
    return UnitValue(result, resultUnit);
}

UnitValue UnitValue::operator-() const {
    return UnitValue(-value, unit);
}

UnitSystem& UnitSystem::getInstance() {
    static UnitSystem instance;
    return instance;
}

void UnitSystem::initializeBuiltinUnits() {
    // Length units (base: meter)
    unitDefinitions.emplace("m", UnitDefinition("m", UnitDimension::LENGTH, 1.0, "m"));
    unitDefinitions.emplace("mm", UnitDefinition("mm", UnitDimension::LENGTH, 0.001, "m"));
    unitDefinitions.emplace("cm", UnitDefinition("cm", UnitDimension::LENGTH, 0.01, "m"));
    unitDefinitions.emplace("km", UnitDefinition("km", UnitDimension::LENGTH, 1000.0, "m"));
    unitDefinitions.emplace("in", UnitDefinition("in", UnitDimension::LENGTH, 0.0254, "m"));
    unitDefinitions.emplace("ft", UnitDefinition("ft", UnitDimension::LENGTH, 0.3048, "m"));
    unitDefinitions.emplace("yd", UnitDefinition("yd", UnitDimension::LENGTH, 0.9144, "m"));
    unitDefinitions.emplace("mi", UnitDefinition("mi", UnitDimension::LENGTH, 1609.34, "m"));

    // Mass units (base: kilogram)
    unitDefinitions.emplace("kg", UnitDefinition("kg", UnitDimension::MASS, 1.0, "kg"));
    unitDefinitions.emplace("g", UnitDefinition("g", UnitDimension::MASS, 0.001, "kg"));
    unitDefinitions.emplace("mg", UnitDefinition("mg", UnitDimension::MASS, 0.000001, "kg"));
    unitDefinitions.emplace("lb", UnitDefinition("lb", UnitDimension::MASS, 0.453592, "kg"));
    unitDefinitions.emplace("oz", UnitDefinition("oz", UnitDimension::MASS, 0.0283495, "kg"));
    unitDefinitions.emplace("ton", UnitDefinition("ton", UnitDimension::MASS, 907.185, "kg"));

    // Force units (base: Newton)
    unitDefinitions.emplace("N", UnitDefinition("N", UnitDimension::FORCE, 1.0, "N"));
    unitDefinitions.emplace("kN", UnitDefinition("kN", UnitDimension::FORCE, 1000.0, "N"));
    unitDefinitions.emplace("lbf", UnitDefinition("lbf", UnitDimension::FORCE, 4.44822, "N"));
    unitDefinitions.emplace("kip", UnitDefinition("kip", UnitDimension::FORCE, 4448.22, "N"));

    // Pressure/Stress units (base: Pascal)
    unitDefinitions.emplace("Pa", UnitDefinition("Pa", UnitDimension::PRESSURE, 1.0, "Pa"));
    unitDefinitions.emplace("kPa", UnitDefinition("kPa", UnitDimension::PRESSURE, 1000.0, "Pa"));
    unitDefinitions.emplace("MPa", UnitDefinition("MPa", UnitDimension::PRESSURE, 1e6, "Pa"));
    unitDefinitions.emplace("GPa", UnitDefinition("GPa", UnitDimension::PRESSURE, 1e9, "Pa"));
    unitDefinitions.emplace("psi", UnitDefinition("psi", UnitDimension::PRESSURE, 6894.76, "Pa", "lbf/in^2"));
    unitDefinitions.emplace("ksi", UnitDefinition("ksi", UnitDimension::PRESSURE, 6.89476e6, "Pa", "kip/in^2"));

    // Area units (base: square meter)
    unitDefinitions.emplace("m2", UnitDefinition("m2", UnitDimension::AREA, 1.0, "m2"));
    unitDefinitions.emplace("mm2", UnitDefinition("mm2", UnitDimension::AREA, 1e-6, "m2"));
    unitDefinitions.emplace("cm2", UnitDefinition("cm2", UnitDimension::AREA, 1e-4, "m2"));
    unitDefinitions.emplace("in2", UnitDefinition("in2", UnitDimension::AREA, 0.00064516, "m2"));
    unitDefinitions.emplace("ft2", UnitDefinition("ft2", UnitDimension::AREA, 0.092903, "m2"));

    // Volume units (base: cubic meter)
    unitDefinitions.emplace("m3", UnitDefinition("m3", UnitDimension::VOLUME, 1.0, "m3"));
    unitDefinitions.emplace("mm3", UnitDefinition("mm3", UnitDimension::VOLUME, 1e-9, "m3"));
    unitDefinitions.emplace("cm3", UnitDefinition("cm3", UnitDimension::VOLUME, 1e-6, "m3"));
    unitDefinitions.emplace("in3", UnitDefinition("in3", UnitDimension::VOLUME, 1.63871e-5, "m3"));
    unitDefinitions.emplace("ft3", UnitDefinition("ft3", UnitDimension::VOLUME, 0.0283168, "m3"));
    unitDefinitions.emplace("L", UnitDefinition("L", UnitDimension::VOLUME, 0.001, "m3"));
    unitDefinitions.emplace("gal", UnitDefinition("gal", UnitDimension::VOLUME, 0.00378541, "m3"));

    // Time units (base: second)
    unitDefinitions.emplace("s", UnitDefinition("s", UnitDimension::TIME, 1.0, "s"));
    unitDefinitions.emplace("ms", UnitDefinition("ms", UnitDimension::TIME, 0.001, "s"));
    unitDefinitions.emplace("min", UnitDefinition("min", UnitDimension::TIME, 60.0, "s"));
    unitDefinitions.emplace("h", UnitDefinition("h", UnitDimension::TIME, 3600.0, "s"));

    // Temperature units (base: Kelvin)
    unitDefinitions.emplace("K", UnitDefinition("K", UnitDimension::TEMPERATURE, 1.0, "K"));
    unitDefinitions.emplace("C", UnitDefinition("C", UnitDimension::TEMPERATURE, 1.0, "K"));
    unitDefinitions.emplace("F", UnitDefinition("F", UnitDimension::TEMPERATURE, 0.555556, "K"));
}

bool UnitSystem::isValidUnit(const std::string& unit) const {
    if (unit.empty()) return true; // Dimensionless
    return unitDefinitions.find(unit) != unitDefinitions.end();
}

UnitDimension UnitSystem::getUnitDimension(const std::string& unit) const {
    auto it = unitDefinitions.find(unit);
    if (it != unitDefinitions.end()) {
        return it->second.dimension;
    }
    return UnitDimension::DIMENSIONLESS;
}

double UnitSystem::getConversionFactor(const std::string& unit) const {
    auto it = unitDefinitions.find(unit);
    if (it != unitDefinitions.end()) {
        return it->second.conversionFactor;
    }
    return 1.0;
}

std::string UnitSystem::getBaseUnit(const std::string& unit) const {
    auto it = unitDefinitions.find(unit);
    if (it != unitDefinitions.end()) {
        return it->second.baseUnit;
    }
    return "";
}

std::string UnitSystem::simplifyUnit(const std::string& unit) const {
    if (unit.empty()) return "";

    // First, normalize numeric suffixes and expand composite units
    std::string normalizedUnit = unit;

    // Expand composite units (e.g., ksi -> kip/in^2)
    for (const auto& pair : unitDefinitions) {
        const std::string& symbol = pair.first;
        const UnitDefinition& def = pair.second;

        if (!def.compositeForm.empty()) {
            // Replace all occurrences of this unit with its composite form
            size_t pos = 0;
            while ((pos = normalizedUnit.find(symbol, pos)) != std::string::npos) {
                // Check if this is a complete unit (not part of a larger word)
                bool isComplete = true;
                if (pos > 0 && std::isalnum(normalizedUnit[pos - 1])) {
                    isComplete = false;
                }
                if (pos + symbol.length() < normalizedUnit.length()) {
                    char nextChar = normalizedUnit[pos + symbol.length()];
                    if (std::isalnum(nextChar) && nextChar != '^' && nextChar != '*' && nextChar != '/') {
                        isComplete = false;
                    }
                }

                if (isComplete) {
                    normalizedUnit.replace(pos, symbol.length(), "(" + def.compositeForm + ")");
                    pos += def.compositeForm.length() + 2; // +2 for parentheses
                } else {
                    pos += symbol.length();
                }
            }
        }
    }

    // Normalize numeric suffixes (in3 -> in^3, mm2 -> mm^2, etc.)
    std::string expanded = "";
    size_t i = 0;
    while (i < normalizedUnit.length()) {
        char ch = normalizedUnit[i];

        // Check if we're starting a unit name (sequence of letters)
        if (std::isalpha(ch)) {
            // Collect all consecutive letters
            std::string letters = "";
            size_t letterEnd = i;
            while (letterEnd < normalizedUnit.length() && std::isalpha(normalizedUnit[letterEnd])) {
                letters += normalizedUnit[letterEnd];
                letterEnd++;
            }

            expanded += letters;

            // Check if followed by digits (numeric suffix)
            if (letterEnd < normalizedUnit.length() && std::isdigit(normalizedUnit[letterEnd])) {
                std::string digitSeq = "";
                while (letterEnd < normalizedUnit.length() && std::isdigit(normalizedUnit[letterEnd])) {
                    digitSeq += normalizedUnit[letterEnd];
                    letterEnd++;
                }
                expanded += "^" + digitSeq;
            }

            i = letterEnd;
        } else {
            expanded += ch;
            i++;
        }
    }

    normalizedUnit = expanded;

    // Parse the unit expression and count each base unit
    std::unordered_map<std::string, int> unitCounts;

    // Split by * and / to get terms
    std::string currentUnit = normalizedUnit;
    std::stringstream ss(currentUnit);
    std::string term;
    bool inNumerator = true;

    // Simple parser for units like "kg*m/s^2" or "(kip/in^2)*in^3"
    size_t pos = 0;
    int parenDepth = 0;
    while (pos < currentUnit.length()) {
        if (currentUnit[pos] == '(') {
            parenDepth++;
            pos++;
            continue;
        } else if (currentUnit[pos] == ')') {
            parenDepth--;
            // When exiting parentheses that were in denominator, reset to numerator for next term
            if (parenDepth == 0) {
                inNumerator = true;
            }
            pos++;
            continue;
        } else if (currentUnit[pos] == '*') {
            // When we hit * at depth 0 and we were in denominator, don't reset
            // but when at depth > 0, keep the current state
            if (parenDepth == 0 && !inNumerator) {
                inNumerator = true; // Reset after finishing denominator terms
            }
            pos++;
            continue;
        } else if (currentUnit[pos] == '/') {
            inNumerator = false;
            pos++;
            continue;
        }

        // Parse unit term (could have ^power)
        size_t termStart = pos;
        while (pos < currentUnit.length() &&
               currentUnit[pos] != '*' &&
               currentUnit[pos] != '/' &&
               currentUnit[pos] != '(' &&
               currentUnit[pos] != ')') {
            pos++;
        }

        std::string unitTerm = currentUnit.substr(termStart, pos - termStart);

        // Skip empty terms
        if (unitTerm.empty()) {
            continue;
        }

        // Check for power (e.g., "m^2")
        int power = 1;
        size_t caretPos = unitTerm.find('^');
        if (caretPos != std::string::npos) {
            std::string baseUnit = unitTerm.substr(0, caretPos);
            std::string powerStr = unitTerm.substr(caretPos + 1);
            power = std::stoi(powerStr);
            unitTerm = baseUnit;
        }

        if (!inNumerator) power = -power;

        unitCounts[unitTerm] += power;
    }

    // Build simplified unit string with proper ordering
    // Sort units: force units (kip, lbf, N) before length units (in, ft, m)
    auto getUnitOrder = [](const std::string& unit) -> int {
        // Force units come first
        if (unit == "kip" || unit == "lbf" || unit == "N" || unit == "kN") return 0;
        // Length units
        if (unit == "in" || unit == "ft" || unit == "m" || unit == "mm" || unit == "cm") return 1;
        // Everything else
        return 2;
    };

    // Collect units into vectors for sorting
    std::vector<std::pair<std::string, int>> numeratorUnits, denominatorUnits;

    for (const auto& pair : unitCounts) {
        const std::string& baseUnit = pair.first;
        int count = pair.second;

        if (count > 0) {
            numeratorUnits.push_back({baseUnit, count});
        } else if (count < 0) {
            denominatorUnits.push_back({baseUnit, -count});
        }
    }

    // Sort by unit order
    std::sort(numeratorUnits.begin(), numeratorUnits.end(),
              [&getUnitOrder](const auto& a, const auto& b) {
                  return getUnitOrder(a.first) < getUnitOrder(b.first);
              });
    std::sort(denominatorUnits.begin(), denominatorUnits.end(),
              [&getUnitOrder](const auto& a, const auto& b) {
                  return getUnitOrder(a.first) < getUnitOrder(b.first);
              });

    // Build strings
    std::stringstream numerator, denominator;
    bool hasNumerator = false, hasDenominator = false;

    for (const auto& pair : numeratorUnits) {
        if (hasNumerator) numerator << "*";
        if (pair.second == 1) {
            numerator << pair.first;
        } else {
            numerator << pair.first << "^" << pair.second;
        }
        hasNumerator = true;
    }

    for (const auto& pair : denominatorUnits) {
        if (hasDenominator) denominator << "*";
        if (pair.second == 1) {
            denominator << pair.first;
        } else {
            denominator << pair.first << "^" << pair.second;
        }
        hasDenominator = true;
    }

    std::string result;
    if (hasNumerator) {
        result = numerator.str();
    }
    if (hasDenominator) {
        if (hasNumerator) {
            result += "/" + denominator.str();
        } else {
            result = "1/" + denominator.str();
        }
    }

    // Convert * to - for engineering convention (e.g., kip*in -> kip-in for moment units)
    // This is the standard notation in structural engineering
    size_t starPos = 0;
    while ((starPos = result.find('*', starPos)) != std::string::npos) {
        result[starPos] = '-';
        starPos++;
    }

    return result.empty() ? "" : result;
}

bool UnitSystem::areUnitsCompatible(const std::string& unit1, const std::string& unit2) const {
    if (unit1.empty() && unit2.empty()) return true;
    if (unit1.empty() || unit2.empty()) return false;

    return getUnitDimension(unit1) == getUnitDimension(unit2);
}

} // namespace madola