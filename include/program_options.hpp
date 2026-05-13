#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Хранит параметры запуска программы, полученные из командной строки.
struct ProgramOptions {
    std::size_t matrix_size = 1024;
    std::uint64_t seed = 42;
    int repetitions = 1;
    std::string matrix_a_path;
    std::string matrix_b_path;
    bool verify = false;
    bool show_help = false;
};

// Разбирает аргументы командной строки и формирует структуру параметров.
ProgramOptions parse_program_options(int argc, char** argv);

// Формирует текст справки по доступным параметрам запуска.
std::string build_usage_message(const char* program_name);
