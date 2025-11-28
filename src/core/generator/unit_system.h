#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace madola {

// Unit dimension types (for dimensional analysis)
enum class UnitDimension {
    LENGTH,
    MASS,
    TIME,
    TEMPERATURE,
    FORCE,
    PRESSURE,
    AREA,
    VOLUME,
    DIMENSIONLESS
};

// Unit definition with conversion factors and dimensions
struct UnitDefinition {
    std::string symbol;
    UnitDimension dimension;
    double conversionFactor; // Factor to convert to base unit
    std::string baseUnit;    // Base unit for this dimension
    std::string compositeForm; // Composite form for simplification (e.g., "kip/in2" for ksi)

    UnitDefinition(const std::string& sym, UnitDimension dim, double factor, const std::string& base, const std::string& composite = "")
        : symbol(sym), dimension(dim), conversionFactor(factor), baseUnit(base), compositeForm(composite) {}
};

// A value with units - stores both numeric value and unit information
class UnitValue {
public:
    double value;
    std::string unit;

    UnitValue(double val, const std::string& unit_str = "")
        : value(val), unit(unit_str) {}

    // Check if this is a dimensionless value
    bool isDimensionless() const { return unit.empty(); }

    // Convert to string representation
    std::string toString() const;

    // Convert to LaTeX string representation
    std::string toLatex() const;

    // Unit operations
    UnitValue operator+(const UnitValue& other) const;
    UnitValue operator-(const UnitValue& other) const;
    UnitValue operator*(const UnitValue& other) const;
    UnitValue operator/(const UnitValue& other) const;
    UnitValue operator^(const UnitValue& other) const;
    UnitValue operator-() const; // Unary minus
};

// Unit system manager
class UnitSystem {
public:
    static UnitSystem& getInstance();

    // Register built-in units
    void initializeBuiltinUnits();

    // Unit validation and conversion
    bool isValidUnit(const std::string& unit) const;
    UnitDimension getUnitDimension(const std::string& unit) const;
    double getConversionFactor(const std::string& unit) const;
    std::string getBaseUnit(const std::string& unit) const;

    // Unit simplification
    std::string simplifyUnit(const std::string& unit) const;

    // Unit compatibility checking
    bool areUnitsCompatible(const std::string& unit1, const std::string& unit2) const;

private:
    UnitSystem() { initializeBuiltinUnits(); }
    std::unordered_map<std::string, UnitDefinition> unitDefinitions;
};

} // namespace madola