#include "unit_system.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <regex>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <vector>

namespace madola {

namespace {

long long gcd_ll(long long a, long long b) {
    a = std::llabs(a);
    b = std::llabs(b);
    while (b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a == 0 ? 1 : a;
}

std::string formatArchitecturalImperial(double totalInches) {
    const bool negative = totalInches < 0.0;
    const double absInches = std::abs(totalInches);
    const long long totalSixteenths = static_cast<long long>(std::llround(absInches * 16.0));
    const long long feet = totalSixteenths / (12 * 16);
    const long long remainderSixteenths = totalSixteenths % (12 * 16);
    const long long wholeInches = remainderSixteenths / 16;
    long long fracNum = remainderSixteenths % 16;
    long long fracDen = 16;

    if (fracNum != 0) {
        long long divisor = gcd_ll(fracNum, fracDen);
        fracNum /= divisor;
        fracDen /= divisor;
    }

    std::stringstream ss;
    if (negative && totalSixteenths != 0) {
        ss << "-";
    }

    if (feet > 0) {
        ss << feet << "'";
        if (fracNum == 0) {
            ss << wholeInches << "\"";
        } else {
            ss << wholeInches << "-" << fracNum << "/" << fracDen << "\"";
        }
        return ss.str();
    }

    if (fracNum == 0) {
        ss << wholeInches << "\"";
    } else if (wholeInches > 0) {
        ss << wholeInches << "-" << fracNum << "/" << fracDen << "\"";
    } else {
        ss << fracNum << "/" << fracDen << "\"";
    }

    return ss.str();
}

}

std::string UnitValue::toString() const {
    if (displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL && unit == "in") {
        return formatArchitecturalImperial(value);
    }

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
    if (displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL && unit == "in") {
        return "\\text{" + formatArchitecturalImperial(value) + "}";
    }

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

        // Formats a single unit token (e.g. "in", "in^4", or "in4") as LaTeX,
        // wrapping the base in \text{} and lifting any exponent outside it. The
        // exponent may be written with an explicit caret ("in^4") or as a bare
        // trailing digit run ("in4") — units are commonly stored in the latter
        // form after evaluation.
        auto formatUnitToken = [](const std::string& token) -> std::string {
            size_t caretPos = token.find('^');
            if (caretPos != std::string::npos) {
                return "\\text{" + token.substr(0, caretPos) + "}^{" + token.substr(caretPos + 1) + "}";
            }
            // Bare trailing-digit exponent, e.g. "in3".
            if (token.length() > 1 &&
                std::isdigit(static_cast<unsigned char>(token.back()))) {
                size_t numStart = token.length();
                while (numStart > 0 &&
                       std::isdigit(static_cast<unsigned char>(token[numStart - 1]))) {
                    numStart--;
                }
                if (numStart > 0) {
                    return "\\text{" + token.substr(0, numStart) + "}^{" +
                           token.substr(numStart) + "}";
                }
            }
            return "\\text{" + token + "}";
        };

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
            // Simple unit (possibly with an exponent, e.g. "in^4")
            ss << " " << formatUnitToken(" " + latexUnit);
        }
    }
    return ss.str();
}

UnitValue UnitValue::operator+(const UnitValue& other) const {
    auto& unitSys = UnitSystem::getInstance();

    if (isDimensionless() && other.isDimensionless()) {
        return UnitValue(value + other.value);
    }

    // A bare 0 is unambiguous regardless of the other operand's unit (unlike
    // e.g. "100 m + 3", where 3's implied unit is genuinely unclear) - this is
    // what lets "0 - f(x)" work as a negation idiom for unit-bearing function
    // bodies, since madola has no unary-minus-before-call syntax.
    if (isDimensionless() && value == 0.0) {
        return other;
    }
    if (other.isDimensionless() && other.value == 0.0) {
        return *this;
    }

    if (isDimensionless() || other.isDimensionless()) {
        throw std::runtime_error("Cannot add dimensionless value to value with units");
    }

    ParsedUnit a = unitSys.parseUnitString(unit);
    ParsedUnit b = unitSys.parseUnitString(other.unit);
    if (a.dims != b.dims) {
        throw std::runtime_error("Cannot add incompatible units: " + unit + " + " + other.unit);
    }

    // Convert to same unit (use first operand's unit)
    double otherValueConverted = other.value * b.factor / a.factor;
    if (displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL && other.displayStyle != UnitDisplayStyle::ARCHITECTURAL_IMPERIAL) {
        double lhsMeters = value * a.factor;
        double rhsMeters = other.value * b.factor;
        double totalMeters = lhsMeters + rhsMeters;
        double totalInches = totalMeters / unitSys.getConversionFactor("in");
        return UnitValue(totalInches, "in", UnitDisplayStyle::ARCHITECTURAL_IMPERIAL);
    }

    UnitDisplayStyle style = displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL
        ? displayStyle
        : other.displayStyle;
    return UnitValue(value + otherValueConverted, unit, style);
}

UnitValue UnitValue::operator-(const UnitValue& other) const {
    auto& unitSys = UnitSystem::getInstance();

    if (isDimensionless() && other.isDimensionless()) {
        return UnitValue(value - other.value);
    }

    // See operator+ above: a bare 0 is unambiguous regardless of unit, so
    // "0 - f(x)" works as a negation idiom for unit-bearing expressions.
    if (isDimensionless() && value == 0.0) {
        return -other;
    }
    if (other.isDimensionless() && other.value == 0.0) {
        return *this;
    }

    if (isDimensionless() || other.isDimensionless()) {
        throw std::runtime_error("Cannot subtract dimensionless value from value with units");
    }

    ParsedUnit a = unitSys.parseUnitString(unit);
    ParsedUnit b = unitSys.parseUnitString(other.unit);
    if (a.dims != b.dims) {
        throw std::runtime_error("Cannot subtract incompatible units: " + unit + " - " + other.unit);
    }

    // Convert to same unit (use first operand's unit)
    double otherValueConverted = other.value * b.factor / a.factor;
    if (displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL && other.displayStyle != UnitDisplayStyle::ARCHITECTURAL_IMPERIAL) {
        double lhsMeters = value * a.factor;
        double rhsMeters = other.value * b.factor;
        double totalMeters = lhsMeters - rhsMeters;
        double totalInches = totalMeters / unitSys.getConversionFactor("in");
        return UnitValue(totalInches, "in", UnitDisplayStyle::ARCHITECTURAL_IMPERIAL);
    }

    UnitDisplayStyle style = displayStyle == UnitDisplayStyle::ARCHITECTURAL_IMPERIAL
        ? displayStyle
        : other.displayStyle;
    return UnitValue(value - otherValueConverted, unit, style);
}

UnitValue UnitValue::operator*(const UnitValue& other) const {
    if (isDimensionless()) {
        return UnitValue(value * other.value, other.unit, other.displayStyle);
    }
    if (other.isDimensionless()) {
        return UnitValue(value * other.value, unit, displayStyle);
    }

    auto& unitSys = UnitSystem::getInstance();
    ParsedUnit a = unitSys.parseUnitString(unit);
    ParsedUnit b = unitSys.parseUnitString(other.unit);

    // Combine in SI-base terms so the numeric result is always physically
    // correct regardless of which display units the two operands used, then
    // pick a display unit from the operands' own dimensions/vocabulary.
    double resultSiValue = (value * a.factor) * (other.value * b.factor);
    DimVector resultDims = addDim(a.dims, b.dims);

    std::string displayUnit = unitSys.formatDims(resultDims, 1.0, {unit, other.unit});
    ParsedUnit displayParsed = unitSys.parseUnitString(displayUnit);
    double displayValue = displayParsed.factor != 0.0 ? resultSiValue / displayParsed.factor
                                                       : resultSiValue;
    return UnitValue(displayValue, displayUnit);
}

UnitValue UnitValue::operator/(const UnitValue& other) const {
    if (other.value == 0.0) {
        throw std::runtime_error("Division by zero");
    }

    auto& unitSys = UnitSystem::getInstance();

    if (isDimensionless()) {
        if (other.isDimensionless()) {
            return UnitValue(value / other.value);
        }
        // Dimensionless / unit = 1/unit
        ParsedUnit b = unitSys.parseUnitString(other.unit);
        double resultSiValue = value / (other.value * b.factor);
        DimVector resultDims = subDim(kDimensionlessVec, b.dims);
        std::string displayUnit = unitSys.formatDims(resultDims, 1.0, {"1/" + other.unit});
        ParsedUnit displayParsed = unitSys.parseUnitString(displayUnit);
        double displayValue = displayParsed.factor != 0.0 ? resultSiValue / displayParsed.factor
                                                           : resultSiValue;
        return UnitValue(displayValue, displayUnit);
    }
    if (other.isDimensionless()) {
        return UnitValue(value / other.value, unit, displayStyle);
    }

    ParsedUnit a = unitSys.parseUnitString(unit);
    ParsedUnit b = unitSys.parseUnitString(other.unit);

    double resultSiValue = (value * a.factor) / (other.value * b.factor);
    DimVector resultDims = subDim(a.dims, b.dims);

    // other.unit must be hinted as a denominator term (e.g. "kip"/"in" ->
    // hint "1/(in)"), not as a bare numerator token, or formatDims's compound
    // synthesis would add its exponent instead of subtracting it.
    std::string displayUnit = unitSys.formatDims(resultDims, 1.0, {unit, "1/(" + other.unit + ")"});
    ParsedUnit displayParsed = unitSys.parseUnitString(displayUnit);
    double displayValue = displayParsed.factor != 0.0 ? resultSiValue / displayParsed.factor
                                                       : resultSiValue;
    return UnitValue(displayValue, displayUnit);
}

UnitValue UnitValue::operator^(const UnitValue& other) const {
    if (!other.isDimensionless()) {
        throw std::runtime_error("Exponent must be dimensionless");
    }

    if (isDimensionless()) {
        return UnitValue(std::pow(value, other.value));
    }

    auto& unitSys = UnitSystem::getInstance();
    ParsedUnit a = unitSys.parseUnitString(unit);
    int exp = static_cast<int>(other.value);

    // Raise the SI-base value by the exponent, then convert back into the
    // *original* display unit raised to the same exponent - e.g. (20 ft)^4
    // must scale the conversion factor to the 4th power too, not just relabel
    // the unit string. This is the fix for the historical bug where L^4 with
    // L in ft silently kept the raw ft-numbered value.
    double siValue = std::pow(value * a.factor, other.value);
    DimVector resultDims = scaleDim(a.dims, exp);

    std::string displayUnit = unit + (exp == 1 ? "" : "^" + std::to_string(exp));
    ParsedUnit displayParsed = unitSys.parseUnitString(displayUnit);
    double displayValue = displayParsed.factor != 0.0 ? siValue / displayParsed.factor : siValue;
    (void)resultDims;
    return UnitValue(displayValue, displayUnit);
}

UnitValue UnitValue::operator-() const {
    return UnitValue(-value, unit, displayStyle);
}

UnitSystem& UnitSystem::getInstance() {
    static UnitSystem instance;
    return instance;
}

namespace {

DimVector dimOf(DimIndex idx, int exp = 1) {
    DimVector v{};
    v[static_cast<size_t>(idx)] = exp;
    return v;
}

const DimVector kLengthDim = dimOf(DimIndex::LENGTH);
const DimVector kMassDim = dimOf(DimIndex::MASS);
const DimVector kTimeDim = dimOf(DimIndex::TIME);
const DimVector kTempDim = dimOf(DimIndex::TEMPERATURE);
const DimVector kForceDim = dimOf(DimIndex::FORCE);
// Pressure = force / length^2
const DimVector kPressureDim = subDim(kForceDim, scaleDim(kLengthDim, 2));
const DimVector kAreaDim = scaleDim(kLengthDim, 2);
const DimVector kVolumeDim = scaleDim(kLengthDim, 3);

} // namespace

void UnitSystem::initializeBuiltinUnits() {
    // Length units (base: meter)
    unitDefinitions.emplace("m", UnitDefinition("m", UnitDimension::LENGTH, 1.0, "m", "", kLengthDim));
    unitDefinitions.emplace("mm", UnitDefinition("mm", UnitDimension::LENGTH, 0.001, "m", "", kLengthDim));
    unitDefinitions.emplace("cm", UnitDefinition("cm", UnitDimension::LENGTH, 0.01, "m", "", kLengthDim));
    unitDefinitions.emplace("km", UnitDefinition("km", UnitDimension::LENGTH, 1000.0, "m", "", kLengthDim));
    unitDefinitions.emplace("in", UnitDefinition("in", UnitDimension::LENGTH, 0.0254, "m", "", kLengthDim));
    unitDefinitions.emplace("ft", UnitDefinition("ft", UnitDimension::LENGTH, 0.3048, "m", "", kLengthDim));
    unitDefinitions.emplace("yd", UnitDefinition("yd", UnitDimension::LENGTH, 0.9144, "m", "", kLengthDim));
    unitDefinitions.emplace("mi", UnitDefinition("mi", UnitDimension::LENGTH, 1609.34, "m", "", kLengthDim));

    // Mass units (base: kilogram)
    unitDefinitions.emplace("kg", UnitDefinition("kg", UnitDimension::MASS, 1.0, "kg", "", kMassDim));
    unitDefinitions.emplace("g", UnitDefinition("g", UnitDimension::MASS, 0.001, "kg", "", kMassDim));
    unitDefinitions.emplace("mg", UnitDefinition("mg", UnitDimension::MASS, 0.000001, "kg", "", kMassDim));
    unitDefinitions.emplace("lb", UnitDefinition("lb", UnitDimension::MASS, 0.453592, "kg", "", kMassDim));
    unitDefinitions.emplace("oz", UnitDefinition("oz", UnitDimension::MASS, 0.0283495, "kg", "", kMassDim));
    unitDefinitions.emplace("ton", UnitDefinition("ton", UnitDimension::MASS, 907.185, "kg", "", kMassDim));

    // Force units (base: Newton)
    unitDefinitions.emplace("N", UnitDefinition("N", UnitDimension::FORCE, 1.0, "N", "", kForceDim));
    unitDefinitions.emplace("kN", UnitDefinition("kN", UnitDimension::FORCE, 1000.0, "N", "", kForceDim));
    unitDefinitions.emplace("lbf", UnitDefinition("lbf", UnitDimension::FORCE, 4.44822, "N", "", kForceDim));
    unitDefinitions.emplace("kip", UnitDefinition("kip", UnitDimension::FORCE, 4448.22, "N", "", kForceDim));

    // Pressure/Stress units (base: Pascal). compositeForm kept only as a
    // display fallback; dims is the actual force/length^2 vector so ksi/psi
    // cancel and combine with any other force/length unit, not just their
    // literal composite-form string.
    unitDefinitions.emplace("Pa", UnitDefinition("Pa", UnitDimension::PRESSURE, 1.0, "Pa", "", kPressureDim));
    unitDefinitions.emplace("kPa", UnitDefinition("kPa", UnitDimension::PRESSURE, 1000.0, "Pa", "", kPressureDim));
    unitDefinitions.emplace("MPa", UnitDefinition("MPa", UnitDimension::PRESSURE, 1e6, "Pa", "", kPressureDim));
    unitDefinitions.emplace("GPa", UnitDefinition("GPa", UnitDimension::PRESSURE, 1e9, "Pa", "", kPressureDim));
    unitDefinitions.emplace("psi", UnitDefinition("psi", UnitDimension::PRESSURE, 6894.76, "Pa", "lbf/in^2", kPressureDim));
    unitDefinitions.emplace("ksi", UnitDefinition("ksi", UnitDimension::PRESSURE, 6.89476e6, "Pa", "kip/in^2", kPressureDim));

    // Area units (base: square meter)
    unitDefinitions.emplace("m2", UnitDefinition("m2", UnitDimension::AREA, 1.0, "m2", "", kAreaDim));
    unitDefinitions.emplace("mm2", UnitDefinition("mm2", UnitDimension::AREA, 1e-6, "m2", "", kAreaDim));
    unitDefinitions.emplace("cm2", UnitDefinition("cm2", UnitDimension::AREA, 1e-4, "m2", "", kAreaDim));
    unitDefinitions.emplace("in2", UnitDefinition("in2", UnitDimension::AREA, 0.00064516, "m2", "", kAreaDim));
    unitDefinitions.emplace("ft2", UnitDefinition("ft2", UnitDimension::AREA, 0.092903, "m2", "", kAreaDim));

    // Volume units (base: cubic meter)
    unitDefinitions.emplace("m3", UnitDefinition("m3", UnitDimension::VOLUME, 1.0, "m3", "", kVolumeDim));
    unitDefinitions.emplace("mm3", UnitDefinition("mm3", UnitDimension::VOLUME, 1e-9, "m3", "", kVolumeDim));
    unitDefinitions.emplace("cm3", UnitDefinition("cm3", UnitDimension::VOLUME, 1e-6, "m3", "", kVolumeDim));
    unitDefinitions.emplace("in3", UnitDefinition("in3", UnitDimension::VOLUME, 1.63871e-5, "m3", "", kVolumeDim));
    unitDefinitions.emplace("ft3", UnitDefinition("ft3", UnitDimension::VOLUME, 0.0283168, "m3", "", kVolumeDim));
    unitDefinitions.emplace("L", UnitDefinition("L", UnitDimension::VOLUME, 0.001, "m3", "", kVolumeDim));
    unitDefinitions.emplace("gal", UnitDefinition("gal", UnitDimension::VOLUME, 0.00378541, "m3", "", kVolumeDim));

    // Time units (base: second)
    unitDefinitions.emplace("s", UnitDefinition("s", UnitDimension::TIME, 1.0, "s", "", kTimeDim));
    unitDefinitions.emplace("ms", UnitDefinition("ms", UnitDimension::TIME, 0.001, "s", "", kTimeDim));
    unitDefinitions.emplace("min", UnitDefinition("min", UnitDimension::TIME, 60.0, "s", "", kTimeDim));
    unitDefinitions.emplace("h", UnitDefinition("h", UnitDimension::TIME, 3600.0, "s", "", kTimeDim));

    // Temperature units (base: Kelvin). NOTE: C/F conversionFactor here models
    // a *temperature difference* (ratio only, no offset) - this preserves the
    // pre-existing (known, deferred) limitation that absolute C/F points are
    // not converted with their zero-offset. Not addressed by this rewrite.
    unitDefinitions.emplace("K", UnitDefinition("K", UnitDimension::TEMPERATURE, 1.0, "K", "", kTempDim));
    unitDefinitions.emplace("C", UnitDefinition("C", UnitDimension::TEMPERATURE, 1.0, "K", "", kTempDim));
    unitDefinitions.emplace("F", UnitDefinition("F", UnitDimension::TEMPERATURE, 0.555556, "K", "", kTempDim));
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

namespace {

// Small recursive-descent parser for unit expression strings: handles a
// product/quotient chain of unit tokens (each optionally raised to an
// integer power), with '*'/'-' as multiplication separators (the codebase's
// existing convention is that '-' means '*' in an already-simplified unit
// string, e.g. "kip-in") and parenthesized groups. This replaces simplifyUnit's
// flat single-pass tokenizer with something that actually respects nesting,
// which the dimension-vector rewrite needs since parsed units now compose by
// vector arithmetic rather than by re-stringifying and re-scanning.
struct UnitStringParser {
    const std::string& s;
    size_t pos = 0;
    const std::unordered_map<std::string, UnitDefinition>& defs;

    UnitStringParser(const std::string& str, const std::unordered_map<std::string, UnitDefinition>& d)
        : s(str), defs(d) {}

    void skipSpaces() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) pos++;
    }

    // term := ('(' expr ')' | identifier) ('^' ['-'] digits)?
    ParsedUnit parseTerm() {
        skipSpaces();
        ParsedUnit result;
        if (pos < s.size() && s[pos] == '(') {
            pos++; // consume '('
            result = parseExpr();
            skipSpaces();
            if (pos < s.size() && s[pos] == ')') pos++; // consume ')'
        } else if (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
            // Bare numeral term, e.g. the "1" in "1/in" (dimensionless numerator
            // convention used throughout for an inverse-unit display string).
            size_t start = pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
            result.display = s.substr(start, pos - start);
        } else {
            size_t start = pos;
            while (pos < s.size() && (std::isalpha(static_cast<unsigned char>(s[pos])))) pos++;
            size_t letterEnd = pos;
            std::string letters = s.substr(start, letterEnd - start);

            // A trailing digit run directly after the letters may be either a
            // registry key with a baked-in exponent (e.g. "in2", "m3" - no
            // caret) or the start of an outer "^n" already handled below; the
            // registry key form takes priority whenever it actually resolves
            // (matches simplifyUnit's historical "in3 -> in^3" normalization).
            size_t digitEnd = letterEnd;
            while (digitEnd < s.size() && std::isdigit(static_cast<unsigned char>(s[digitEnd]))) digitEnd++;
            std::string withDigits = s.substr(start, digitEnd - start);

            auto itWithDigits = (digitEnd > letterEnd) ? defs.find(withDigits) : defs.end();
            if (itWithDigits != defs.end()) {
                pos = digitEnd;
                result.dims = itWithDigits->second.dims;
                result.factor = itWithDigits->second.conversionFactor;
                result.display = withDigits;
            } else {
                auto it = defs.find(letters);
                if (it != defs.end()) {
                    result.dims = it->second.dims;
                    result.factor = it->second.conversionFactor;
                }
                if (digitEnd > letterEnd) {
                    // No registry entry for the whole run (e.g. "in4", "ft5" -
                    // the registry only special-cases ^2/^3): treat the digit
                    // suffix as an implicit exponent on the base unit rather
                    // than silently dropping it, matching the grammar's own
                    // "value + unit + exponentDigits" concatenation for any
                    // power (ast_builder_expressions.cpp::buildUnitExpression).
                    pos = digitEnd;
                    int impliedExp = std::stoi(s.substr(letterEnd, digitEnd - letterEnd));
                    result.dims = scaleDim(result.dims, impliedExp);
                    result.factor = std::pow(result.factor, impliedExp);
                    result.display = letters + "^" + std::to_string(impliedExp);
                } else {
                    pos = letterEnd;
                    result.display = letters;
                }
            }
        }

        skipSpaces();
        if (pos < s.size() && s[pos] == '^') {
            pos++; // consume '^'
            skipSpaces();
            bool neg = false;
            if (pos < s.size() && s[pos] == '-') { neg = true; pos++; }
            size_t numStart = pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
            int exp = numStart < pos ? std::stoi(s.substr(numStart, pos - numStart)) : 1;
            if (neg) exp = -exp;
            result.dims = scaleDim(result.dims, exp);
            result.factor = std::pow(result.factor, exp);
            result.display += "^" + std::to_string(exp);
        }
        return result;
    }

    // expr := term (('*' | '-') term | '/' term)*
    ParsedUnit parseExpr() {
        ParsedUnit acc = parseTerm();
        for (;;) {
            skipSpaces();
            if (pos >= s.size()) break;
            char op = s[pos];
            if (op != '*' && op != '/' && op != '-') break;
            pos++;
            ParsedUnit rhs = parseTerm();
            if (op == '/') {
                acc.dims = subDim(acc.dims, rhs.dims);
                acc.factor = rhs.factor != 0.0 ? acc.factor / rhs.factor : acc.factor;
                acc.display += "/" + rhs.display;
            } else {
                acc.dims = addDim(acc.dims, rhs.dims);
                acc.factor *= rhs.factor;
                acc.display += "*" + rhs.display;
            }
        }
        return acc;
    }
};

} // namespace

ParsedUnit UnitSystem::parseUnitString(const std::string& unitStr) const {
    if (unitStr.empty()) return ParsedUnit{};
    UnitStringParser parser(unitStr, unitDefinitions);
    ParsedUnit result = parser.parseExpr();
    result.display = unitStr;
    return result;
}

namespace {

// Canonical base-unit name (SI) for a dimension vector, used only when no
// display hint is usable. Only the combinations the registry can produce are
// covered; anything else falls back to a generic "u1^e1*u2^e2" rendering.
std::string canonicalNameForDims(const DimVector& dims) {
    static const std::vector<std::pair<DimVector, std::string>> kKnown = {
        {dimOf(DimIndex::LENGTH), "m"},
        {dimOf(DimIndex::MASS), "kg"},
        {dimOf(DimIndex::TIME), "s"},
        {dimOf(DimIndex::TEMPERATURE), "K"},
        {dimOf(DimIndex::FORCE), "N"},
        {kPressureDim, "Pa"},
        {kAreaDim, "m^2"},
        {kVolumeDim, "m^3"},
    };
    for (const auto& pair : kKnown) {
        if (pair.first == dims) return pair.second;
    }

    static const char* kAxisNames[] = {"m", "kg", "s", "K", "N"};
    std::string num, den;
    for (size_t i = 0; i < dims.size(); ++i) {
        int e = dims[i];
        if (e == 0) continue;
        std::string term = kAxisNames[i];
        if (std::abs(e) != 1) term += "^" + std::to_string(std::abs(e));
        if (e > 0) {
            if (!num.empty()) num += "*";
            num += term;
        } else {
            if (!den.empty()) den += "*";
            den += term;
        }
    }
    if (num.empty()) num = "1";
    return den.empty() ? num : num + "/" + den;
}

}

namespace {

// Extract each top-level unit token from a hint string as a (base-name,
// exponent) pair. A unit's exponent may be written either with an explicit
// caret ("in^2") or as a registry key with a baked-in digit suffix ("in2",
// "m3") - the latter takes priority whenever the whole run resolves in the
// registry, matching the grammar's own encoding of e.g. "3 in^2" as unit
// string "in2" (ast_builder_expressions.cpp::buildUnitExpression). Ignores
// grouping symbols - good enough since hints here are always single-term or
// simple product/quotient chains produced by this same code, never
// arbitrarily deep nested expressions.
std::vector<std::pair<std::string, int>> tokenizeHint(const std::string& hint,
                                                        const std::unordered_map<std::string, UnitDefinition>& defs) {
    std::vector<std::pair<std::string, int>> tokens;
    size_t i = 0;
    bool inDenominator = false;
    while (i < hint.size()) {
        char c = hint[i];
        if (c == '/') { inDenominator = true; i++; continue; }
        if (c == '*' || c == '-' || c == '(' || c == ')') { i++; continue; }
        if (std::isalpha(static_cast<unsigned char>(c))) {
            size_t start = i;
            while (i < hint.size() && std::isalpha(static_cast<unsigned char>(hint[i]))) i++;
            size_t letterEnd = i;
            std::string letters = hint.substr(start, letterEnd - start);

            size_t digitEnd = letterEnd;
            while (digitEnd < hint.size() && std::isdigit(static_cast<unsigned char>(hint[digitEnd]))) digitEnd++;
            std::string withDigits = hint.substr(start, digitEnd - start);

            std::string name = letters;
            int bakedExp = 1;
            if (digitEnd > letterEnd && defs.find(withDigits) != defs.end()) {
                i = digitEnd;
                name = letters; // base symbol without the digit suffix
                bakedExp = std::stoi(hint.substr(letterEnd, digitEnd - letterEnd));
            } else if (digitEnd > letterEnd) {
                // No registry entry for the whole run (e.g. "in4" beyond the
                // registry's ^2/^3 special cases): the digit suffix is still
                // an implicit exponent on the base unit, not noise to skip.
                i = digitEnd;
                bakedExp = std::stoi(hint.substr(letterEnd, digitEnd - letterEnd));
            } else {
                i = letterEnd;
            }

            int exp = bakedExp;
            if (i < hint.size() && hint[i] == '^') {
                i++;
                size_t numStart = i;
                bool neg = false;
                if (i < hint.size() && hint[i] == '-') { neg = true; i++; numStart = i; }
                while (i < hint.size() && std::isdigit(static_cast<unsigned char>(hint[i]))) i++;
                if (i > numStart) exp = bakedExp * std::stoi(hint.substr(numStart, i - numStart));
                if (neg) exp = -exp;
            }
            int signedExp = inDenominator ? -exp : exp;

            // If this unit is a named composite (e.g. ksi = kip/in^2), expand
            // it into its constituent tokens scaled by signedExp rather than
            // keeping it opaque - otherwise "ksi * in^3" can't cancel down to
            // "kip-in" the way "kip/in^2 * in^3" obviously would.
            auto it = defs.find(name);
            if (it != defs.end() && !it->second.compositeForm.empty()) {
                for (auto sub : tokenizeHint(it->second.compositeForm, defs)) {
                    tokens.emplace_back(sub.first, sub.second * signedExp);
                }
            } else {
                tokens.emplace_back(name, signedExp);
            }
        } else {
            i++;
        }
    }
    return tokens;
}

}

std::string UnitSystem::formatDims(const DimVector& dims, double /*factor*/,
                                    const std::vector<std::string>& displayHints) const {
    if (isZeroDim(dims)) return "";

    // Prefer a display hint whose own dimension matches the result exactly -
    // this is what lets "5 kip * 2 in^2 -> 10 kip" collapse to a plain unit
    // name via the registry, and "10 kip / 2 in -> 5 kip/in" keep the
    // operand's own compound spelling instead of a re-derived one.
    for (const auto& hint : displayHints) {
        if (hint.empty()) continue;
        ParsedUnit parsed = parseUnitString(hint);
        if (parsed.dims == dims) return hint;
    }

    // Try to name the result from a single registered unit with this exact
    // dimension (prefers units appearing in the hints' vocabulary first, then
    // any known unit, so e.g. a pressure result prefers ksi/psi over Pa if
    // the source expression used US units).
    for (const auto& hint : displayHints) {
        for (const auto& tok : tokenizeHint(hint, unitDefinitions)) {
            auto it = unitDefinitions.find(tok.first);
            if (it != unitDefinitions.end() && it->second.dims == dims) return tok.first;
        }
    }
    for (const auto& pair : unitDefinitions) {
        if (pair.second.dims == dims) return pair.first;
    }

    // No single registered unit has this exact dimension - synthesize a
    // compound name from the operands' own unit tokens (not generic SI axis
    // names), mirroring the pre-existing "kip-in" / "kip/in^2" style: multiply
    // together every token contributed by every hint, then re-derive
    // numerator/denominator groupings from the combined exponents.
    std::unordered_map<std::string, int> tokenExponents;
    std::vector<std::string> order;
    for (const auto& hint : displayHints) {
        for (const auto& tok : tokenizeHint(hint, unitDefinitions)) {
            if (tokenExponents.find(tok.first) == tokenExponents.end()) order.push_back(tok.first);
            tokenExponents[tok.first] += tok.second;
        }
    }

    std::string numerator, denominator;
    for (const auto& name : order) {
        int exp = tokenExponents[name];
        if (exp == 0) continue;
        std::string term = name;
        if (std::abs(exp) != 1) term += "^" + std::to_string(std::abs(exp));
        if (exp > 0) {
            if (!numerator.empty()) numerator += "-";
            numerator += term;
        } else {
            if (!denominator.empty()) denominator += "-";
            denominator += term;
        }
    }
    if (!numerator.empty() && !denominator.empty()) return numerator + "/" + denominator;
    if (!numerator.empty()) return numerator;
    if (!denominator.empty()) return "1/" + denominator;

    return canonicalNameForDims(dims);
}

// simplifyUnit()/areUnitsCompatible() (the string-regex-based unit
// simplifier and its per-unit dimension-enum compatibility check) were
// removed here: no caller remained once operator*/operator//operator^/+/-
// moved to dimension-vector arithmetic via parseUnitString()/formatDims().

} // namespace madola
