#include "distributed_multiplier.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace {

int checked_to_int(std::size_t value, const char* message) {
    // MPI во многих коллективных операциях использует int-счётчики, поэтому проверяем переполнение заранее.
    constexpr std::size_t int_max = static_cast<std::size_t>(2147483647);
    if(value > int_max) {
        throw std::overflow_error(message);
    }

    return static_cast<int>(value);
}

std::vector<int> build_row_distribution(std::size_t rows, int process_count) {
    const std::size_t base_rows = rows / static_cast<std::size_t>(process_count);
    const std::size_t remainder = rows % static_cast<std::size_t>(process_count);

    std::vector<int> distribution(static_cast<std::size_t>(process_count), 0);

    // Если число строк не делится нацело, первые процессы получают на одну строку больше.
    for(int rank = 0; rank < process_count; ++rank) {
        const std::size_t rows_for_rank = base_rows + (static_cast<std::size_t>(rank) < remainder ? 1U : 0U);
        distribution[static_cast<std::size_t>(rank)] =
            checked_to_int(rows_for_rank, "Too many matrix rows for MPI integer counters.");
    }

    return distribution;
}

std::vector<int> build_element_counts(const std::vector<int>& row_distribution, std::size_t column_count) {
    const int columns = checked_to_int(column_count, "Too many columns for MPI integer counters.");
    std::vector<int> counts = row_distribution;

    // Переводим число строк для каждого процесса в число элементов матрицы.
    for(int& count : counts) {
        count *= columns;
    }

    return counts;
}

std::vector<int> build_displacements(const std::vector<int>& counts) {
    std::vector<int> displacements(counts.size(), 0);

    // Смещения нужны для корректной раздачи и сборки блоков переменного размера.
    for(std::size_t index = 1; index < counts.size(); ++index) {
        displacements[index] = displacements[index - 1] + counts[index - 1];
    }

    return displacements;
}

}

DistributedMultiplyResult multiply_distributed(const Matrix& lhs_on_root,
                                               const Matrix& rhs_on_root,
                                               MPI_Comm communicator) {
    int rank = 0;
    int process_count = 0;
    if(MPI_Comm_rank(communicator, &rank) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    if(MPI_Comm_size(communicator, &process_count) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    unsigned long long dimensions[4] = {0ULL, 0ULL, 0ULL, 0ULL};
    if(rank == 0) {
        if(lhs_on_root.cols != rhs_on_root.rows) {
            throw std::invalid_argument("Matrix dimensions do not allow multiplication.");
        }

        // Только корневой процесс изначально владеет полными матрицами и рассылает размеры остальным.
        dimensions[0] = static_cast<unsigned long long>(lhs_on_root.rows);
        dimensions[1] = static_cast<unsigned long long>(lhs_on_root.cols);
        dimensions[2] = static_cast<unsigned long long>(rhs_on_root.rows);
        dimensions[3] = static_cast<unsigned long long>(rhs_on_root.cols);
    }

    if(MPI_Bcast(dimensions, 4, MPI_UNSIGNED_LONG_LONG, 0, communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    const std::size_t lhs_rows = static_cast<std::size_t>(dimensions[0]);
    const std::size_t lhs_cols = static_cast<std::size_t>(dimensions[1]);
    const std::size_t rhs_rows = static_cast<std::size_t>(dimensions[2]);
    const std::size_t rhs_cols = static_cast<std::size_t>(dimensions[3]);

    if(lhs_cols != rhs_rows) {
        throw std::invalid_argument("Broadcast dimensions are inconsistent for multiplication.");
    }

    const std::vector<int> row_distribution = build_row_distribution(lhs_rows, process_count);
    const std::vector<int> lhs_counts = build_element_counts(row_distribution, lhs_cols);
    const std::vector<int> result_counts = build_element_counts(row_distribution, rhs_cols);
    const std::vector<int> lhs_displacements = build_displacements(lhs_counts);
    const std::vector<int> result_displacements = build_displacements(result_counts);

    const int local_row_count = row_distribution[static_cast<std::size_t>(rank)];

    // Каждый процесс хранит только свой фрагмент матрицы A и соответствующий фрагмент результата.
    Matrix rhs(rhs_rows, rhs_cols);
    if(rank == 0) {
        rhs = rhs_on_root;
    }
    Matrix local_lhs(static_cast<std::size_t>(local_row_count), lhs_cols);
    Matrix local_result(static_cast<std::size_t>(local_row_count), rhs_cols);

    Matrix result_on_root;
    if(rank == 0) {
        result_on_root = Matrix(lhs_rows, rhs_cols);
    }

    // Барьер нужен, чтобы замер времени начинался после подготовки всех процессов.
    if(MPI_Barrier(communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }
    const double start_time = MPI_Wtime();

    // Матрица B нужна каждому процессу полностью, потому что каждая его строка результата
    // умножается на все столбцы B.
    if(MPI_Bcast(rhs.values.data(),
                 checked_to_int(rhs.element_count(), "Too many RHS elements for MPI integer counters."),
                 MPI_DOUBLE,
                 0,
                 communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    // Матрица A делится по строкам: каждый процесс получает только свой фрагмент.
    if(MPI_Scatterv(rank == 0 ? lhs_on_root.values.data() : nullptr,
                    lhs_counts.data(),
                    lhs_displacements.data(),
                    MPI_DOUBLE,
                    local_lhs.values.data(),
                    checked_to_int(local_lhs.element_count(),
                                   "Too many local LHS elements for MPI integer counters."),
                    MPI_DOUBLE,
                    0,
                    communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    // Каждый процесс независимо считает только свой блок строк результирующей матрицы.
    for(std::size_t row = 0; row < local_lhs.rows; ++row) {
        for(std::size_t shared = 0; shared < lhs_cols; ++shared) {
            const double lhs_value = local_lhs(row, shared);
            for(std::size_t col = 0; col < rhs_cols; ++col) {
                local_result(row, col) += lhs_value * rhs(shared, col);
            }
        }
    }

    // Частичные результаты собираются обратно на корневом процессе в одну матрицу.
    if(MPI_Gatherv(local_result.values.data(),
                   checked_to_int(local_result.element_count(),
                                  "Too many local result elements for MPI integer counters."),
                   MPI_DOUBLE,
                   rank == 0 ? result_on_root.values.data() : nullptr,
                   result_counts.data(),
                   result_displacements.data(),
                   MPI_DOUBLE,
                   0,
                   communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    const double local_elapsed = MPI_Wtime() - start_time;
    double global_elapsed = 0.0;

    // Реальное время параллельного алгоритма определяется самым медленным процессом.
    if(MPI_Reduce(&local_elapsed, &global_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    if(MPI_Bcast(&global_elapsed, 1, MPI_DOUBLE, 0, communicator) != MPI_SUCCESS) {
        MPI_Abort(communicator, 1);
    }

    // Итоговую матрицу получает только корневой процесс, но время выполнения полезно знать всем.
    return DistributedMultiplyResult{std::move(result_on_root), global_elapsed};
}
