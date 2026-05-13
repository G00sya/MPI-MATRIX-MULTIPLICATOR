set(_MSMPI_ARCH "x86")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_MSMPI_ARCH "x64")
endif()

file(TO_CMAKE_PATH "$ENV{ProgramFiles}" _PROGRAM_FILES_DIR)
get_filename_component(_PROGRAM_FILES_PARENT "${_PROGRAM_FILES_DIR}" DIRECTORY)
set(_PROGRAM_FILES_X86_DIR "${_PROGRAM_FILES_PARENT}/Program Files (x86)")

find_path(MSMPI_INCLUDE_DIR
    NAMES mpi.h
    HINTS
        ENV MSMPI_INC
        "${_PROGRAM_FILES_X86_DIR}/Microsoft SDKs/MPI/Include"
        "$ENV{ProgramFiles}/Microsoft SDKs/MPI/Include"
)

find_library(MSMPI_LIBRARY
    NAMES msmpi MSMPI
    HINTS
        ENV MSMPI_LIB${_MSMPI_ARCH}
        "${_PROGRAM_FILES_X86_DIR}/Microsoft SDKs/MPI/Lib/${_MSMPI_ARCH}"
        "$ENV{ProgramFiles}/Microsoft SDKs/MPI/Lib/${_MSMPI_ARCH}"
)

find_file(MSMPI_DLL
    NAMES msmpi.dll
    HINTS
        "$ENV{ProgramFiles}/Microsoft MPI/Bin"
        "$ENV{SystemRoot}/System32"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MSMPI
    REQUIRED_VARS
        MSMPI_INCLUDE_DIR
        MSMPI_LIBRARY
)

if(MSMPI_FOUND AND NOT TARGET MSMPI::MSMPI)
    add_library(MSMPI::MSMPI SHARED IMPORTED)
    set_target_properties(MSMPI::MSMPI PROPERTIES
        IMPORTED_IMPLIB "${MSMPI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MSMPI_INCLUDE_DIR}"
    )

    if(MSMPI_DLL)
        set_target_properties(MSMPI::MSMPI PROPERTIES IMPORTED_LOCATION "${MSMPI_DLL}")
    endif()
endif()

mark_as_advanced(MSMPI_INCLUDE_DIR MSMPI_LIBRARY MSMPI_DLL)
