cmake_minimum_required(VERSION 3.20)

project(ga_sp_topology
    VERSION 0.1.0
    DESCRIPTION "CAD Macro Placement using Sequence Pair and Genetic Algorithm"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Строгие флаги компилятора согласно AGENTS.md (DoD: Zero Warnings)
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Подключение nlohmann/json v3.11.3 через FetchContent
include(FetchContent)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Исполняемый файл САПР
add_executable(ga_sp_topology
    src/main.cpp
)
target_include_directories(ga_sp_topology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ga_sp_topology PRIVATE core_lib io_lib verifier_lib)
if(WIN32)
    target_link_libraries(ga_sp_topology PRIVATE psapi)
endif()

# Тестовая инфраструктура (CTest)
enable_testing()

add_executable(test_skeleton
    tests/test_skeleton.cpp
)
target_include_directories(test_skeleton PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_skeleton PRIVATE nlohmann_json::nlohmann_json)

add_test(NAME test_skeleton COMMAND test_skeleton)

add_executable(test_data_formats
    tests/test_data_formats.cpp
)
target_include_directories(test_data_formats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_test(NAME test_data_formats COMMAND test_data_formats)

# Слой независимого верификатора
add_library(verifier_lib STATIC
    src/verifier/VerificationReport.cpp
    src/verifier/PlacementVerifier.cpp
)
target_include_directories(verifier_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_verifier
    tests/test_verifier.cpp
)
target_link_libraries(test_verifier PRIVATE verifier_lib)

add_test(NAME test_verifier COMMAND test_verifier)

# Слой ввода-вывода JSON и SVG
add_library(io_lib STATIC
    src/io/JsonIO.cpp
    src/io/SvgExporter.cpp
)
target_include_directories(io_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(io_lib PUBLIC nlohmann_json::nlohmann_json)

add_executable(test_json_io
    tests/test_json_io.cpp
)
target_link_libraries(test_json_io PRIVATE io_lib)

add_test(NAME test_json_io COMMAND test_json_io)
set_tests_properties(test_json_io PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Вычислительное ядро САПР
find_package(OpenMP)

add_library(core_lib STATIC
    src/core/DagPlacementDecoder.cpp
    src/core/StandardCostEvaluator.cpp
    src/core/GARunner.cpp
)
target_include_directories(core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(OpenMP_CXX_FOUND)
    target_link_libraries(core_lib PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(test_placement_decoder
    tests/test_placement_decoder.cpp
)
target_link_libraries(test_placement_decoder PRIVATE core_lib verifier_lib)

add_test(NAME test_placement_decoder COMMAND test_placement_decoder)

add_executable(test_cost_evaluator
    tests/test_cost_evaluator.cpp
)
target_link_libraries(test_cost_evaluator PRIVATE core_lib)

add_test(NAME test_cost_evaluator COMMAND test_cost_evaluator)

add_executable(test_ga_engine
    tests/test_ga_engine.cpp
)
target_link_libraries(test_ga_engine PRIVATE core_lib io_lib verifier_lib)

add_test(NAME test_ga_engine COMMAND test_ga_engine)
set_tests_properties(test_ga_engine PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
