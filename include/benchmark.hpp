#pragma once

#include "matrix.hpp"
#include "program_options.hpp"

#include <cstddef>
#include <optional>
#include <vector>

// Хранит сводную статистику по времени выполнения и проверке результата.
struct BenchmarkSummary {
    std::size_t lhs_rows = 0;
    std::size_t lhs_cols = 0;
    std::size_t rhs_rows = 0;
    std::size_t rhs_cols = 0;
    int processes = 1;
    int repetitions = 1;
    double average_seconds = 0.0;
    double min_seconds = 0.0;
    double max_seconds = 0.0;
    double checksum = 0.0;
    std::optional<double> max_error;
};

// Строит итоговую статистику по нескольким измерениям времени и размерам исходных матриц.
BenchmarkSummary build_benchmark_summary(const ProgramOptions& options,
                                         const Matrix& lhs,
                                         const Matrix& rhs,
                                         int processes,
                                         const std::vector<double>& measurements,
                                         double checksum,
                                         std::optional<double> max_error);

// Печатает сводную статистику в удобном для отчёта и скриптов виде.
void print_benchmark_summary(const BenchmarkSummary& summary);
