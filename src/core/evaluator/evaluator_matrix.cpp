#include "evaluator.h"
#include <stdexcept>
#include <cmath>

namespace madola {

Value Evaluator::multiplyMatrixVector(const ArrayValue& matrix, const ArrayValue& vector) {
    // Verify dimensions: matrix rows x columns * vector elements
    size_t matrixRows = matrix.matrixRows.size();
    size_t matrixCols = matrix.matrixRows.empty() ? 0 : matrix.matrixRows[0].size();
    size_t vectorSize = vector.elements.size();

    if (matrixCols != vectorSize) {
        throw std::runtime_error("Matrix-vector multiplication dimension mismatch: " +
                               std::to_string(matrixRows) + "x" + std::to_string(matrixCols) +
                               " matrix cannot multiply " + std::to_string(vectorSize) + "x1 vector");
    }

    // Perform multiplication: result is a column vector with matrixRows elements
    std::vector<double> resultElements;
    resultElements.reserve(matrixRows);

    for (size_t i = 0; i < matrixRows; ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < matrixCols; ++j) {
            sum += matrix.matrixRows[i][j] * vector.elements[j];
        }
        resultElements.push_back(sum);
    }

    return ArrayValue(std::move(resultElements), true); // Return as column vector
}

Value Evaluator::multiplyVectorMatrix(const ArrayValue& vector, const ArrayValue& matrix) {
    // Row vector * Matrix: 1×n * n×m = 1×m (row vector result)
    size_t vectorSize = vector.elements.size();
    size_t matrixRows = matrix.matrixRows.size();
    size_t matrixCols = matrix.matrixRows.empty() ? 0 : matrix.matrixRows[0].size();

    if (vectorSize != matrixRows) {
        throw std::runtime_error("Vector-matrix multiplication dimension mismatch: 1x" +
                               std::to_string(vectorSize) + " vector cannot multiply " +
                               std::to_string(matrixRows) + "x" + std::to_string(matrixCols) + " matrix");
    }

    // Perform multiplication: result is a row vector with matrixCols elements
    std::vector<double> resultElements;
    resultElements.reserve(matrixCols);

    for (size_t j = 0; j < matrixCols; ++j) {
        double sum = 0.0;
        for (size_t i = 0; i < matrixRows; ++i) {
            sum += vector.elements[i] * matrix.matrixRows[i][j];
        }
        resultElements.push_back(sum);
    }

    return ArrayValue(std::move(resultElements), false); // Return as row vector
}

Value Evaluator::dotProduct(const ArrayValue& vector1, const ArrayValue& vector2) {
    // Row vector * Column vector = scalar (dot product)
    size_t size1 = vector1.elements.size();
    size_t size2 = vector2.elements.size();

    if (size1 != size2) {
        throw std::runtime_error("Dot product dimension mismatch: vectors must have same size (" +
                               std::to_string(size1) + " vs " + std::to_string(size2) + ")");
    }

    double sum = 0.0;
    for (size_t i = 0; i < size1; ++i) {
        sum += vector1.elements[i] * vector2.elements[i];
    }

    return sum; // Return as scalar
}

Value Evaluator::multiplyMatrixMatrix(const ArrayValue& matrix1, const ArrayValue& matrix2) {
    // Matrix1 (m×n) * Matrix2 (n×p) = Result (m×p)
    size_t m = matrix1.matrixRows.size();  // rows in matrix1
    size_t n = matrix1.matrixRows.empty() ? 0 : matrix1.matrixRows[0].size();  // cols in matrix1
    size_t n2 = matrix2.matrixRows.size(); // rows in matrix2
    size_t p = matrix2.matrixRows.empty() ? 0 : matrix2.matrixRows[0].size();  // cols in matrix2

    if (n != n2) {
        throw std::runtime_error("Matrix multiplication dimension mismatch: " +
                               std::to_string(m) + "x" + std::to_string(n) +
                               " matrix cannot multiply " + std::to_string(n2) + "x" + std::to_string(p) + " matrix");
    }

    // Perform matrix multiplication
    std::vector<std::vector<double>> result(m, std::vector<double>(p, 0.0));

    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < p; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < n; ++k) {
                sum += matrix1.matrixRows[i][k] * matrix2.matrixRows[k][j];
            }
            result[i][j] = sum;
        }
    }

    return ArrayValue(result);
}

Value Evaluator::matrixDeterminant(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("Det() can only be applied to matrices");
    }

    size_t n = matrix.matrixRows.size();
    if (n == 0) {
        throw std::runtime_error("Cannot compute determinant of empty matrix");
    }

    size_t m = matrix.matrixRows[0].size();
    if (n != m) {
        throw std::runtime_error("Det() requires a square matrix, got " + std::to_string(n) + "x" + std::to_string(m));
    }

    // Base cases
    if (n == 1) {
        return matrix.matrixRows[0][0];
    }

    if (n == 2) {
        return matrix.matrixRows[0][0] * matrix.matrixRows[1][1] -
               matrix.matrixRows[0][1] * matrix.matrixRows[1][0];
    }

    // For larger matrices, use cofactor expansion along first row
    double det = 0.0;
    for (size_t j = 0; j < n; ++j) {
        // Create minor matrix (remove row 0 and column j)
        std::vector<std::vector<double>> minor;
        for (size_t i = 1; i < n; ++i) {
            std::vector<double> row;
            for (size_t k = 0; k < n; ++k) {
                if (k != j) {
                    row.push_back(matrix.matrixRows[i][k]);
                }
            }
            minor.push_back(row);
        }

        ArrayValue minorMatrix(minor);
        double minorDet = std::get<double>(matrixDeterminant(minorMatrix));
        double cofactor = ((j % 2 == 0) ? 1 : -1) * matrix.matrixRows[0][j] * minorDet;
        det += cofactor;
    }

    return det;
}

Value Evaluator::matrixTrace(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("Tr() can only be applied to matrices");
    }

    size_t n = matrix.matrixRows.size();
    if (n == 0) {
        throw std::runtime_error("Cannot compute trace of empty matrix");
    }

    size_t m = matrix.matrixRows[0].size();
    if (n != m) {
        throw std::runtime_error("Tr() requires a square matrix, got " + std::to_string(n) + "x" + std::to_string(m));
    }

    double trace = 0.0;
    for (size_t i = 0; i < n; ++i) {
        trace += matrix.matrixRows[i][i];
    }

    return trace;
}

Value Evaluator::matrixTranspose(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("T() can only be applied to matrices");
    }

    size_t rows = matrix.matrixRows.size();
    if (rows == 0) {
        return ArrayValue(std::vector<std::vector<double>>());
    }

    size_t cols = matrix.matrixRows[0].size();

    // Create transposed matrix: cols x rows
    std::vector<std::vector<double>> transposed(cols, std::vector<double>(rows));

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            transposed[j][i] = matrix.matrixRows[i][j];
        }
    }

    return ArrayValue(transposed);
}

Value Evaluator::matrixInverse(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("Inv() can only be applied to matrices");
    }

    size_t n = matrix.matrixRows.size();
    if (n == 0) {
        throw std::runtime_error("Cannot compute inverse of empty matrix");
    }

    size_t m = matrix.matrixRows[0].size();
    if (n != m) {
        throw std::runtime_error("Inv() requires a square matrix, got " + std::to_string(n) + "x" + std::to_string(m));
    }

    // Check if determinant is zero
    double det = std::get<double>(matrixDeterminant(matrix));
    if (std::abs(det) < 1e-12) {
        throw std::runtime_error("Matrix is singular (determinant is zero), cannot compute inverse");
    }

    // For 1x1 matrix
    if (n == 1) {
        std::vector<std::vector<double>> inv = {{1.0 / matrix.matrixRows[0][0]}};
        return ArrayValue(inv);
    }

    // For 2x2 matrix
    if (n == 2) {
        double a = matrix.matrixRows[0][0];
        double b = matrix.matrixRows[0][1];
        double c = matrix.matrixRows[1][0];
        double d = matrix.matrixRows[1][1];

        std::vector<std::vector<double>> inv = {
            {d / det, -b / det},
            {-c / det, a / det}
        };
        return ArrayValue(inv);
    }

    // For larger matrices, use Gauss-Jordan elimination
    // Create augmented matrix [A | I]
    std::vector<std::vector<double>> augmented(n, std::vector<double>(2 * n));

    // Fill left side with original matrix
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            augmented[i][j] = matrix.matrixRows[i][j];
        }
    }

    // Fill right side with identity matrix
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            augmented[i][n + j] = (i == j) ? 1.0 : 0.0;
        }
    }

    // Perform Gauss-Jordan elimination
    for (size_t i = 0; i < n; ++i) {
        // Find pivot
        size_t pivotRow = i;
        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(augmented[k][i]) > std::abs(augmented[pivotRow][i])) {
                pivotRow = k;
            }
        }

        // Swap rows if needed
        if (pivotRow != i) {
            std::swap(augmented[i], augmented[pivotRow]);
        }

        // Check for zero pivot
        if (std::abs(augmented[i][i]) < 1e-12) {
            throw std::runtime_error("Matrix is singular, cannot compute inverse");
        }

        // Scale pivot row
        double pivot = augmented[i][i];
        for (size_t j = 0; j < 2 * n; ++j) {
            augmented[i][j] /= pivot;
        }

        // Eliminate column
        for (size_t k = 0; k < n; ++k) {
            if (k != i) {
                double factor = augmented[k][i];
                for (size_t j = 0; j < 2 * n; ++j) {
                    augmented[k][j] -= factor * augmented[i][j];
                }
            }
        }
    }

    // Extract inverse matrix from right side
    std::vector<std::vector<double>> inverse(n, std::vector<double>(n));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            inverse[i][j] = augmented[i][n + j];
        }
    }

    return ArrayValue(inverse);
}

Value Evaluator::matrixEigenvalues(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("Eigenvalues() can only be applied to matrices");
    }

    size_t n = matrix.matrixRows.size();
    if (n == 0) {
        throw std::runtime_error("Cannot compute eigenvalues of empty matrix");
    }

    size_t m = matrix.matrixRows[0].size();
    if (n != m) {
        throw std::runtime_error("Eigenvalues() requires a square matrix, got " + std::to_string(n) + "x" + std::to_string(m));
    }

    // Special case: 1x1 matrix
    if (n == 1) {
        return ArrayValue({matrix.matrixRows[0][0]}, false);
    }

    // Special case: 2x2 matrix - use analytical solution
    if (n == 2) {
        double a = matrix.matrixRows[0][0];
        double b = matrix.matrixRows[0][1];
        double c = matrix.matrixRows[1][0];
        double d = matrix.matrixRows[1][1];

        // Eigenvalues from characteristic equation: λ² - tr(A)λ + det(A) = 0
        double trace = a + d;
        double det = a * d - b * c;
        double discriminant = trace * trace - 4 * det;

        if (discriminant < 0) {
            throw std::runtime_error("Complex eigenvalues not yet supported");
        }

        double sqrtDisc = std::sqrt(discriminant);
        double lambda1 = (trace + sqrtDisc) / 2.0;
        double lambda2 = (trace - sqrtDisc) / 2.0;

        return ArrayValue({lambda1, lambda2}, false);
    }

    // For larger matrices, use QR algorithm with Householder transformations
    // First, copy the matrix (we'll modify it in place)
    std::vector<std::vector<double>> A = matrix.matrixRows;

    const int maxIterations = 1000;
    const double tolerance = 1e-10;

    // QR algorithm iteration
    for (int iter = 0; iter < maxIterations; ++iter) {
        // QR decomposition using Householder reflections
        std::vector<std::vector<double>> Q(n, std::vector<double>(n, 0.0));
        std::vector<std::vector<double>> R = A;

        // Initialize Q as identity
        for (size_t i = 0; i < n; ++i) {
            Q[i][i] = 1.0;
        }

        // Apply Householder transformations
        for (size_t k = 0; k < n - 1; ++k) {
            // Compute Householder vector for column k
            std::vector<double> x(n - k);
            for (size_t i = k; i < n; ++i) {
                x[i - k] = R[i][k];
            }

            double norm = 0.0;
            for (double val : x) {
                norm += val * val;
            }
            norm = std::sqrt(norm);

            if (norm < 1e-15) continue;

            double sign = (x[0] >= 0) ? 1.0 : -1.0;
            x[0] += sign * norm;

            double vnorm = 0.0;
            for (double val : x) {
                vnorm += val * val;
            }
            vnorm = std::sqrt(vnorm);

            if (vnorm < 1e-15) continue;

            for (double& val : x) {
                val /= vnorm;
            }

            // Apply Householder transformation to R
            for (size_t j = k; j < n; ++j) {
                double dot = 0.0;
                for (size_t i = 0; i < n - k; ++i) {
                    dot += x[i] * R[k + i][j];
                }
                for (size_t i = 0; i < n - k; ++i) {
                    R[k + i][j] -= 2.0 * dot * x[i];
                }
            }

            // Apply Householder transformation to Q
            for (size_t j = 0; j < n; ++j) {
                double dot = 0.0;
                for (size_t i = 0; i < n - k; ++i) {
                    dot += x[i] * Q[j][k + i];
                }
                for (size_t i = 0; i < n - k; ++i) {
                    Q[j][k + i] -= 2.0 * dot * x[i];
                }
            }
        }

        // Compute A = R * Q
        std::vector<std::vector<double>> newA(n, std::vector<double>(n, 0.0));
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                for (size_t k = 0; k < n; ++k) {
                    newA[i][j] += R[i][k] * Q[k][j];
                }
            }
        }

        // Check convergence (off-diagonal elements should be small)
        double offDiagSum = 0.0;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                if (i != j) {
                    offDiagSum += std::abs(newA[i][j]);
                }
            }
        }

        A = newA;

        if (offDiagSum < tolerance) {
            break;
        }
    }

    // Extract eigenvalues from diagonal
    std::vector<double> eigenvalues;
    for (size_t i = 0; i < n; ++i) {
        eigenvalues.push_back(A[i][i]);
    }

    return ArrayValue(eigenvalues, false);
}

Value Evaluator::matrixEigenvectors(const ArrayValue& matrix) {
    if (!matrix.isMatrix) {
        throw std::runtime_error("Eigenvectors() can only be applied to matrices");
    }

    size_t n = matrix.matrixRows.size();
    if (n == 0) {
        throw std::runtime_error("Cannot compute eigenvectors of empty matrix");
    }

    size_t m = matrix.matrixRows[0].size();
    if (n != m) {
        throw std::runtime_error("Eigenvectors() requires a square matrix, got " + std::to_string(n) + "x" + std::to_string(m));
    }

    // Special case: 1x1 matrix
    if (n == 1) {
        std::vector<std::vector<double>> eigenvectors = {{1.0}};
        return ArrayValue(eigenvectors);
    }

    // First, compute eigenvalues
    Value eigenvalResult = matrixEigenvalues(matrix);
    std::vector<double> eigenvalues = std::get<ArrayValue>(eigenvalResult).elements;

    // Compute eigenvectors using inverse iteration for each eigenvalue
    std::vector<std::vector<double>> eigenvectors;

    for (size_t eigIdx = 0; eigIdx < n; ++eigIdx) {
        double lambda = eigenvalues[eigIdx];

        // Create (A - λI)
        std::vector<std::vector<double>> A_minus_lambdaI = matrix.matrixRows;
        for (size_t i = 0; i < n; ++i) {
            A_minus_lambdaI[i][i] -= lambda;
        }

        // Find null space using Gaussian elimination
        std::vector<std::vector<double>> augmented = A_minus_lambdaI;

        // Forward elimination
        for (size_t col = 0; col < n && col < n; ++col) {
            // Find pivot
            size_t pivotRow = col;
            double maxVal = std::abs(augmented[col][col]);
            for (size_t row = col + 1; row < n; ++row) {
                if (std::abs(augmented[row][col]) > maxVal) {
                    maxVal = std::abs(augmented[row][col]);
                    pivotRow = row;
                }
            }

            // Swap rows if needed
            if (pivotRow != col) {
                std::swap(augmented[col], augmented[pivotRow]);
            }

            // Skip if pivot is too small (dependent row)
            if (std::abs(augmented[col][col]) < 1e-10) {
                continue;
            }

            // Eliminate below
            for (size_t row = col + 1; row < n; ++row) {
                double factor = augmented[row][col] / augmented[col][col];
                for (size_t c = col; c < n; ++c) {
                    augmented[row][c] -= factor * augmented[col][c];
                }
            }
        }

        // Back substitution to find eigenvector
        std::vector<double> eigenvector(n, 0.0);

        // Set the last component to 1 (or first free variable)
        bool foundFree = false;
        for (int i = n - 1; i >= 0; --i) {
            bool isZeroRow = true;
            for (size_t j = 0; j < n; ++j) {
                if (std::abs(augmented[i][j]) > 1e-10) {
                    isZeroRow = false;
                    break;
                }
            }
            if (isZeroRow || !foundFree) {
                eigenvector[i] = 1.0;
                foundFree = true;
                break;
            }
        }

        // Back substitute
        for (int i = n - 1; i >= 0; --i) {
            // Find leading coefficient
            size_t leadCol = n;
            for (size_t j = 0; j < n; ++j) {
                if (std::abs(augmented[i][j]) > 1e-10) {
                    leadCol = j;
                    break;
                }
            }

            if (leadCol < n && leadCol < eigenvector.size()) {
                double sum = 0.0;
                for (size_t j = leadCol + 1; j < n; ++j) {
                    sum += augmented[i][j] * eigenvector[j];
                }
                eigenvector[leadCol] = -sum / augmented[i][leadCol];
            }
        }

        // Normalize eigenvector
        double norm = 0.0;
        for (double val : eigenvector) {
            norm += val * val;
        }
        norm = std::sqrt(norm);

        if (norm > 1e-15) {
            for (double& val : eigenvector) {
                val /= norm;
            }
        }

        eigenvectors.push_back(eigenvector);
    }

    // Return eigenvectors as columns of a matrix (transpose to get row format)
    std::vector<std::vector<double>> result(n, std::vector<double>(n));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            result[i][j] = eigenvectors[j][i];
        }
    }

    return ArrayValue(result);
}

} // namespace madola