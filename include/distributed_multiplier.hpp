#pragma once

#include "matrix.hpp"

#include <mpi.h>

// Хранит результат MPI-умножения и время выполнения параллельного алгоритма.
struct DistributedMultiplyResult {
    Matrix result_on_root;
    double elapsed_seconds = 0.0;
};

// Выполняет параллельное умножение матриц с помощью MPI.
// Полные матрицы должны находиться на процессе 0.
// Итоговая матрица возвращается только на процессе 0.
DistributedMultiplyResult multiply_distributed(const Matrix& lhs_on_root,
                                               const Matrix& rhs_on_root,
                                               MPI_Comm communicator);
