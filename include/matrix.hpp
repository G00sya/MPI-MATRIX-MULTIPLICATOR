#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Хранит плотную матрицу в виде одномерного массива по строкам.
struct Matrix {
    std::size_t rows = 0;
    std::size_t cols = 0;
    std::vector<double> values;

    Matrix() = default;
    Matrix(std::size_t row_count, std::size_t col_count);

    // Возвращает доступ к элементу матрицы по индексам строки и столбца.
    double& operator()(std::size_t row, std::size_t col);

    // Возвращает значение элемента матрицы по индексам строки и столбца.
    double operator()(std::size_t row, std::size_t col) const;

    // Возвращает общее количество элементов в матрице.
    std::size_t element_count() const;
};

// Создаёт матрицу заданного размера и заполняет её псевдослучайными числами.
Matrix generate_random_matrix(std::size_t rows, std::size_t cols, std::uint64_t seed);

// Загружает матрицу из текстового файла.
// В первой строке должны быть указаны размеры: rows cols.
Matrix load_matrix_from_file(const std::string& file_path);

// Выполняет обычное последовательное умножение матриц без MPI.
Matrix multiply_sequential(const Matrix& lhs, const Matrix& rhs);

// Находит максимальное по модулю различие между двумя матрицами одинакового размера.
double max_absolute_difference(const Matrix& lhs, const Matrix& rhs);

// Вычисляет сумму всех элементов матрицы для простой проверки результата.
double matrix_checksum(const Matrix& matrix);
