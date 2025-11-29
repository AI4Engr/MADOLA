#include "evaluator.h"
#include <stdexcept>
#include <cmath>
#include <Eigen/Dense>

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

    // Convert to Eigen types
    Eigen::MatrixXd eigenMatrix(matrixRows, matrixCols);
    for (size_t i = 0; i < matrixRows; ++i) {
        for (size_t j = 0; j < matrixCols; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    Eigen::VectorXd eigenVector(vectorSize);
    for (size_t i = 0; i < vectorSize; ++i) {
        eigenVector(i) = vector.elements[i];
    }

    // Perform multiplication using Eigen
    Eigen::VectorXd result = eigenMatrix * eigenVector;

    // Convert back to std::vector
    std::vector<double> resultElements(result.data(), result.data() + result.size());

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

    // Convert to Eigen types
    Eigen::RowVectorXd eigenVector(vectorSize);
    for (size_t i = 0; i < vectorSize; ++i) {
        eigenVector(i) = vector.elements[i];
    }

    Eigen::MatrixXd eigenMatrix(matrixRows, matrixCols);
    for (size_t i = 0; i < matrixRows; ++i) {
        for (size_t j = 0; j < matrixCols; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Perform multiplication using Eigen
    Eigen::RowVectorXd result = eigenVector * eigenMatrix;

    // Convert back to std::vector
    std::vector<double> resultElements(result.data(), result.data() + result.size());

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

    // Convert to Eigen types
    Eigen::VectorXd eigenVector1(size1);
    Eigen::VectorXd eigenVector2(size2);

    for (size_t i = 0; i < size1; ++i) {
        eigenVector1(i) = vector1.elements[i];
        eigenVector2(i) = vector2.elements[i];
    }

    // Perform dot product using Eigen
    double result = eigenVector1.dot(eigenVector2);

    return result; // Return as scalar
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

    // Convert to Eigen types
    Eigen::MatrixXd eigenMatrix1(m, n);
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix1(i, j) = matrix1.matrixRows[i][j];
        }
    }

    Eigen::MatrixXd eigenMatrix2(n2, p);
    for (size_t i = 0; i < n2; ++i) {
        for (size_t j = 0; j < p; ++j) {
            eigenMatrix2(i, j) = matrix2.matrixRows[i][j];
        }
    }

    // Perform matrix multiplication using Eigen
    Eigen::MatrixXd eigenResult = eigenMatrix1 * eigenMatrix2;

    // Convert back to std::vector<std::vector<double>>
    std::vector<std::vector<double>> result(m, std::vector<double>(p));
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < p; ++j) {
            result[i][j] = eigenResult(i, j);
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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Compute determinant using Eigen
    double det = eigenMatrix.determinant();

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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Compute trace using Eigen
    double trace = eigenMatrix.trace();

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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Compute transpose using Eigen
    Eigen::MatrixXd eigenTransposed = eigenMatrix.transpose();

    // Convert back to std::vector<std::vector<double>>
    std::vector<std::vector<double>> transposed(cols, std::vector<double>(rows));
    for (size_t i = 0; i < cols; ++i) {
        for (size_t j = 0; j < rows; ++j) {
            transposed[i][j] = eigenTransposed(i, j);
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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Check if determinant is zero
    double det = eigenMatrix.determinant();
    if (std::abs(det) < 1e-12) {
        throw std::runtime_error("Matrix is singular (determinant is zero), cannot compute inverse");
    }

    // Compute inverse using Eigen
    Eigen::MatrixXd eigenInverse = eigenMatrix.inverse();

    // Convert back to std::vector<std::vector<double>>
    std::vector<std::vector<double>> inverse(n, std::vector<double>(n));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            inverse[i][j] = eigenInverse(i, j);
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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Compute eigenvalues using Eigen's EigenSolver
    Eigen::EigenSolver<Eigen::MatrixXd> solver(eigenMatrix);

    // Check if eigenvalues are real
    if (solver.eigenvalues().imag().norm() > 1e-10) {
        throw std::runtime_error("Complex eigenvalues not yet supported");
    }

    // Extract real eigenvalues
    Eigen::VectorXd eigenvalues = solver.eigenvalues().real();

    // Convert to std::vector
    std::vector<double> result(eigenvalues.data(), eigenvalues.data() + eigenvalues.size());

    return ArrayValue(result, false);
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

    // Convert to Eigen type
    Eigen::MatrixXd eigenMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            eigenMatrix(i, j) = matrix.matrixRows[i][j];
        }
    }

    // Compute eigenvectors using Eigen's EigenSolver
    Eigen::EigenSolver<Eigen::MatrixXd> solver(eigenMatrix);

    // Check if eigenvectors are real
    if (solver.eigenvectors().imag().norm() > 1e-10) {
        throw std::runtime_error("Complex eigenvectors not yet supported");
    }

    // Extract real eigenvectors (as columns)
    Eigen::MatrixXd eigenvectors = solver.eigenvectors().real();

    // Convert to std::vector<std::vector<double>> (transpose to get row format)
    std::vector<std::vector<double>> result(n, std::vector<double>(n));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            result[i][j] = eigenvectors(i, j);
        }
    }

    return ArrayValue(result);
}

} // namespace madola