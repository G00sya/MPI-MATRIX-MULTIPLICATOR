#include "benchmark.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <numeric>

namespace {

double compute_average(const std::vector<double>& values) {
    // Для отчёта удобнее использовать среднее время по нескольким запускам.
    const double total = std::accumulate(values.begin(), values.end(), 0.0);
    return total / static_cast<double>(values.size());
}

}

BenchmarkSummary build_benchmark_summary(const ProgramOptions& options,
                                         const Matrix& lhs,
                                         const Matrix& rhs,
                                         int processes,
                                         const std::vector<double>& measurements,
                                         double checksum,
                                         std::optional<double> max_error) {
    BenchmarkSummary summary;
    summary.lhs_rows = lhs.rows;
    summary.lhs_cols = lhs.cols;
    summary.rhs_rows = rhs.rows;
    summary.rhs_cols = rhs.cols;
    summary.processes = processes;
    summary.repetitions = options.repetitions;
    summary.average_seconds = compute_average(measurements);
    summary.min_seconds = *std::min_element(measurements.begin(), measurements.end());
    summary.max_seconds = *std::max_element(measurements.begin(), measurements.end());
    summary.checksum = checksum;
    summary.max_error = max_error;
    return summary;
}

void print_benchmark_summary(const BenchmarkSummary& summary) {
    fmt::print("Matrix A size: {} x {}\n", summary.lhs_rows, summary.lhs_cols);
    fmt::print("Matrix B size: {} x {}\n", summary.rhs_rows, summary.rhs_cols);
    fmt::print("MPI processes: {}\n", summary.processes);
    fmt::print("Repetitions: {}\n", summary.repetitions);
    fmt::print("Distributed time (avg/min/max): {:.6f} / {:.6f} / {:.6f} s\n",
               summary.average_seconds,
               summary.min_seconds,
               summary.max_seconds);
    fmt::print("Result checksum: {:.6f}\n", summary.checksum);

    if(summary.max_error.has_value()) {
        fmt::print("Verification: passed, max absolute error = {:.6e}\n", *summary.max_error);
    }

    // Отдельная строка RESULT нужна скрипту, который автоматически собирает статистику в CSV.
    fmt::print(
        "RESULT lhs_rows={} lhs_cols={} rhs_rows={} rhs_cols={} processes={} repetitions={} "
        "distributed_seconds={:.6f} min_seconds={:.6f} max_seconds={:.6f} checksum={:.6f} "
        "verified={} max_error={:.6e}\n",
        summary.lhs_rows,
        summary.lhs_cols,
        summary.rhs_rows,
        summary.rhs_cols,
        summary.processes,
        summary.repetitions,
        summary.average_seconds,
        summary.min_seconds,
        summary.max_seconds,
        summary.checksum,
        summary.max_error.has_value() ? 1 : 0,
        summary.max_error.value_or(0.0)
    );
}
