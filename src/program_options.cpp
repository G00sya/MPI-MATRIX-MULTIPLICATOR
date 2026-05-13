#include "program_options.hpp"

#include <fmt/format.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

std::size_t parse_size_value(const std::string& value, const char* option_name) {
    // Размер матрицы должен быть положительным, иначе запуск не имеет смысла.
    const unsigned long long parsed = std::stoull(value);
    if(parsed == 0ULL) {
        throw std::invalid_argument(fmt::format("{} must be greater than zero.", option_name));
    }

    return static_cast<std::size_t>(parsed);
}

int parse_int_value(const std::string& value, const char* option_name) {
    // Число повторов тоже должно быть положительным.
    const int parsed = std::stoi(value);
    if(parsed <= 0) {
        throw std::invalid_argument(fmt::format("{} must be greater than zero.", option_name));
    }

    return parsed;
}

std::uint64_t parse_seed_value(const std::string& value, const char*) {
    const auto parsed = std::stoull(value);
    return static_cast<std::uint64_t>(parsed);
}

}

ProgramOptions parse_program_options(int argc, char** argv) {
    ProgramOptions options;

    // Аргументы разбираются вручную, потому что у программы короткий и фиксированный набор флагов.
    for(int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if(argument == "--help" || argument == "-h") {
            options.show_help = true;
            return options;
        }

        if(argument == "--verify") {
            options.verify = true;
            continue;
        }

        if(index + 1 >= argc) {
            throw std::invalid_argument(fmt::format("Missing value after '{}'.", argument));
        }

        const std::string value = argv[++index];

        if(argument == "--size") {
            options.matrix_size = parse_size_value(value, "--size");
        } else if(argument == "--seed") {
            options.seed = parse_seed_value(value, "--seed");
        } else if(argument == "--repetitions") {
            options.repetitions = parse_int_value(value, "--repetitions");
        } else if(argument == "--matrix-a") {
            options.matrix_a_path = value;
        } else if(argument == "--matrix-b") {
            options.matrix_b_path = value;
        } else {
            throw std::invalid_argument(fmt::format("Unknown argument '{}'.", argument));
        }
    }

    return options;
}

std::string build_usage_message(const char* program_name) {
    return fmt::format(
        "Usage:\n"
        "  {} [--size N] [--seed S] [--repetitions R] [--verify]\n"
        "  {} --matrix-a FILE --matrix-b FILE [--repetitions R] [--verify]\n\n"
        "Options:\n"
        "  --size N          Matrix size for NxN multiplication (default: 1024)\n"
        "  --seed S          Seed for pseudo-random matrix generation (default: 42)\n"
        "  --matrix-a FILE   Path to the first matrix file\n"
        "  --matrix-b FILE   Path to the second matrix file\n"
        "  --repetitions R   Number of repeated MPI runs inside one launch (default: 1)\n"
        "  --verify          Compare the distributed result against a sequential multiplication\n"
        "  --help, -h        Print this help message\n",
        program_name,
        program_name
    );
}
