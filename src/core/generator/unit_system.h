#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace madola {

enum class UnitDisplayStyle {
    STANDARD,
    ARCHITECTURAL_IMPERIAL
};

// Unit dimension types (kept for backward-compat callers; a registered unit's
// "primary" dimension for isValidUnit()/getUnitDimension() lookups)
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

// Exponent vector over the base physical dimensions. FORCE is carried as its
// own base axis (rather than derived as MASS*LENGTH/TIME^2) because the
// registry's conversion factors are calibrated against N/lbf directly, not
// against kg*m/s^2 - keeping it a base axis avoids a second factor system.
enum class DimIndex : size_t {
    LENGTH = 0,
    MASS = 1,
    TIME = 2,
    TEMPERATURE = 3,
    FORCE = 4,
    NUM_DIMS = 5
};

using DimVector = std::array<int, static_cast<size_t>(DimIndex::NUM_DIMS)>;

constexpr DimVector kDimensionlessVec{};

inline DimVector addDim(const DimVector& a, const DimVector& b) {
    DimVector r{};
    for (size_t i = 0; i < r.size(); ++i) r[i] = a[i] + b[i];
    return r;
}

inline DimVector subDim(const DimVector& a, const DimVector& b) {
    DimVector r{};
    for (size_t i = 0; i < r.size(); ++i) r[i] = a[i] - b[i];
    return r;
}

inline DimVector scaleDim(const DimVector& a, int n) {
    DimVector r{};
    for (size_t i = 0; i < r.size(); ++i) r[i] = a[i] * n;
    return r;
}

inline bool isZeroDim(const DimVector& a) {
    for (int e : a) if (e != 0) return false;
    return true;
}

// Unit definition with conversion factors and dimensions
struct UnitDefinition {
    std::string symbol;
    UnitDimension dimension;
    double conversionFactor; // Factor to convert to base unit
    std::string baseUnit;    // Base unit for this dimension
    std::string compositeForm; // Composite form, e.g. "kip/in^2" for ksi (display fallback only)
    DimVector dims;             // Exponents over {length, mass, time, temperature, force}

    UnitDefinition(const std::string& sym, UnitDimension dim, double factor, const std::string& base,
                   const std::string& composite, DimVector dimVec)
        : symbol(sym), dimension(dim), conversionFactor(factor), baseUnit(base),
          compositeForm(composite), dims(dimVec) {}
};

// A parsed unit string: dimension vector + numeric factor to convert 1 unit
// to its SI base, plus the original string for display purposes.
struct ParsedUnit {
    DimVector dims{};
    double factor = 1.0;       // multiply a value in this unit by factor to get SI-base value
    std::string display;       // the unit string as written by the user (for display fallback)
};

// A value with units - stores both a numeric value and unit information.
// Public API (constructor signature, isDimensionless(), toString()/toLatex(),
// the six operators) is unchanged from the string-based implementation so
// that the ~3 external call sites (grammar-level unit literals, WASM bridge)
// do not need to change. Internally, the unit string is parsed once into a
// dimension vector + SI factor so arithmetic is vector math, not string ops.
class UnitValue {
public:
    double value;
    std::string unit;
    UnitDisplayStyle displayStyle;

    UnitValue(double val, const std::string& unit_str = "", UnitDisplayStyle style = UnitDisplayStyle::STANDARD)
        : value(val), unit(unit_str), displayStyle(style) {}

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

    // Parse a unit expression string (e.g. "kip/in^2", "(kip)*(in)") into its
    // dimension vector and SI-base conversion factor.
    ParsedUnit parseUnitString(const std::string& unitStr) const;

    // Render a dimension vector + factor back to a display string, preferring
    // the given hint units (from the operands) when they match the dimension.
    std::string formatDims(const DimVector& dims, double factor,
                            const std::vector<std::string>& displayHints) const;

    const std::unordered_map<std::string, UnitDefinition>& definitions() const { return unitDefinitions; }

private:
    UnitSystem() { initializeBuiltinUnits(); }
    std::unordered_map<std::string, UnitDefinition> unitDefinitions;
};

} // namespace madola
