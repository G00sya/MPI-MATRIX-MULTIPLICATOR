#include "matrix.hpp"

#include <fstream>
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

Matrix::Matrix(std::size_t row_count, std::size_t col_count)
    : rows(row_count),
      cols(col_count),
      values(row_count * col_count, 0.0) {
}

double& Matrix::operator()(std::size_t row, std::size_t col) {
    return values[row * cols + col];
}

double Matrix::operator()(std::size_t row, std::size_t col) const {
    return values[row * cols + col];
}

std::size_t Matrix::element_count() const {
    return values.size();
}

Matrix generate_random_matrix(std::size_t rows, std::size_t cols, std::uint64_t seed) {
    Matrix matrix(rows, cols);

    std::mt19937_64 generator(seed);
    std::uniform_real_distribution<double> distribution(-1.0, 1.0);

    for(double& value : matrix.values) {
        value = distribution(generator);
    }

    return matrix;
}

Matrix load_matrix_from_file(const std::string& file_path) {
    std::ifstream input(file_path);
    if(!input) {
        throw std::runtime_error("Could not open matrix file: " + file_path);
    }

    std::size_t rows = 0;
    std::size_t cols = 0;
    if(!(input >> rows >> cols)) {
        throw std::runtime_error("Could not read matrix dimensions from file: " + file_path);
    }

    if(rows == 0 || cols == 0) {
        throw std::runtime_error("Matrix dimensions in file must be positive: " + file_path);
    }

    Matrix matrix(rows, cols);

    // После строки с размерами в файле должны идти rows * cols чисел.
    for(std::size_t row = 0; row < rows; ++row) {
        for(std::size_t col = 0; col < cols; ++col) {
            if(!(input >> matrix(row, col))) {
                throw std::runtime_error("Not enough matrix values in file: " + file_path);
            }
        }
    }

    return matrix;
}

Matrix multiply_sequential(const Matrix& lhs, const Matrix& rhs) {
    if(lhs.cols != rhs.rows) {
        throw std::invalid_argument("Matrix dimensions do not allow multiplication.");
    }

    Matrix result(lhs.rows, rhs.cols);

    // Последовательная версия нужна как базовый алгоритм и для проверки корректности MPI-результата.
    for(std::size_t row = 0; row < lhs.rows; ++row) {
        for(std::size_t shared = 0; shared < lhs.cols; ++shared) {
            const double lhs_value = lhs(row, shared);
            for(std::size_t col = 0; col < rhs.cols; ++col) {
                result(row, col) += lhs_value * rhs(shared, col);
            }
        }
    }

    return result;
}

double max_absolute_difference(const Matrix& lhs, const Matrix& rhs) {
    if(lhs.rows != rhs.rows || lhs.cols != rhs.cols) {
        throw std::invalid_argument("Matrix dimensions do not match for comparison.");
    }

    double difference = 0.0;

    // Сравниваем все элементы и оставляем максимальное отклонение.
    for(std::size_t index = 0; index < lhs.values.size(); ++index) {
        difference = std::max(difference, std::abs(lhs.values[index] - rhs.values[index]));
    }

    return difference;
}

double matrix_checksum(const Matrix& matrix) {
    double sum = 0.0;

    for(const double value : matrix.values) {
        sum += value;
    }

    return sum;
}
