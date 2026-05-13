#include "benchmark.hpp"
#include "distributed_multiplier.hpp"
#include "matrix.hpp"
#include "program_options.hpp"

#include <fmt/format.h>

#include <mpi.h>

#include <cstdio>
#include <exception>
#include <optional>
#include <vector>

int main(int argc, char** argv) {
    const int init_status = MPI_Init(&argc, &argv);
    if(init_status != MPI_SUCCESS) {
        std::fprintf(stderr, "MPI error in MPI_Init: status code %d\n", init_status);
        MPI_Abort(MPI_COMM_WORLD, init_status);
        return 1;
    }

    if(MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN) != MPI_SUCCESS) {
        // Переводим MPI в режим, где функции возвращают код ошибки, а не завершают программу сами.
        std::fprintf(stderr, "MPI error in MPI_Comm_set_errhandler\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    int rank = 0;
    int process_count = 0;
    if(MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
        std::fprintf(stderr, "MPI error in MPI_Comm_rank\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    if(MPI_Comm_size(MPI_COMM_WORLD, &process_count) != MPI_SUCCESS) {
        std::fprintf(stderr, "MPI error in MPI_Comm_size\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    try {
        const ProgramOptions options = parse_program_options(argc, argv);
        if(options.show_help) {
            if(rank == 0) {
                fmt::print("{}", build_usage_message(argv[0]));
            }
            MPI_Finalize();
            return 0;
        }

        Matrix lhs;
        Matrix rhs;
        const bool use_input_files = !options.matrix_a_path.empty() || !options.matrix_b_path.empty();

        if(rank == 0) {
            if(use_input_files) {
                if(options.matrix_a_path.empty() || options.matrix_b_path.empty()) {
                    throw std::invalid_argument("Both --matrix-a and --matrix-b must be provided together.");
                }

                // В файловом режиме матрицы загружаются с диска, а не генерируются случайно.
                lhs = load_matrix_from_file(options.matrix_a_path);
                rhs = load_matrix_from_file(options.matrix_b_path);
            } else {
                // В обычном режиме полные исходные матрицы создаются только на корневом процессе.
                lhs = generate_random_matrix(options.matrix_size, options.matrix_size, options.seed);
                rhs = generate_random_matrix(options.matrix_size, options.matrix_size, options.seed + 1);
            }
        }

        std::vector<double> measurements;
        measurements.reserve(static_cast<std::size_t>(options.repetitions));

        Matrix final_result;
        for(int repetition = 0; repetition < options.repetitions; ++repetition) {
            // Один запуск может содержать несколько повторов, чтобы усреднять время вычислений.
            const DistributedMultiplyResult distributed = multiply_distributed(lhs, rhs, MPI_COMM_WORLD);
            measurements.push_back(distributed.elapsed_seconds);

            if(rank == 0) {
                final_result = distributed.result_on_root;
            }
        }

        if(rank == 0) {
            // На корневом процессе при необходимости дополнительно считаем результат обычным способом.
            std::optional<double> max_error;
            if(options.verify) {
                // Проверка нужна для демонстрации корректности MPI-реализации относительно обычного алгоритма.
                const Matrix sequential = multiply_sequential(lhs, rhs);
                max_error = max_absolute_difference(final_result, sequential);
            }

            const BenchmarkSummary summary =
                build_benchmark_summary(options,
                                        lhs,
                                        rhs,
                                        process_count,
                                        measurements,
                                        matrix_checksum(final_result),
                                        max_error);
            print_benchmark_summary(summary);
        }

        MPI_Finalize();
        return 0;
    } catch(const std::exception& error) {
        if(rank == 0) {
            // Сообщение об ошибке выводим только один раз, чтобы не дублировать его на всех процессах.
            fmt::print(stderr, "Error: {}\n", error.what());
        }

        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
}
