cmake_minimum_required(VERSION 3.22)

project(stm32_workspace LANGUAGES C ASM)

# Keep the CubeMX firmware in its own directory while allowing VS Code to open
# and configure the repository root as a normal CMake project.
add_subdirectory(CBoard_Basics_Complete)

# Keep the ELF beside the root-level CMake cache for a predictable output path.
set_target_properties(CBoard_Basics_Complete PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
