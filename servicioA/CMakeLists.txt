cmake_minimum_required(VERSION 3.16)
project(servicioa LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(servicioa
    src/main.cpp
    src/utils.cpp
    src/db_config.cpp
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
target_include_directories(servicioa PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_libraries(servicioa PRIVATE ${PQXX_LIBRARIES})
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(servicioa PRIVATE nlohmann_json::nlohmann_json)
find_package(OpenSSL REQUIRED)
target_link_libraries(servicioa PRIVATE OpenSSL::Crypto)
target_include_directories(servicioa PRIVATE ${CMAKE_SOURCE_DIR}/src/third_party)
target_compile_definitions(servicioa PRIVATE CPPHTTPLIB_MULTIPART_FORM_DATA)


enable_testing()

add_library(servicioa_objs
    src/utils.cpp
    src/db_config.cpp
)
target_include_directories(servicioa_objs PUBLIC src src/third_party)
target_link_libraries(servicioa_objs pqxx pq)

add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_utils PRIVATE servicioa_objs)
add_test(NAME test_utils COMMAND test_utils)

add_executable(test_conninfo tests/test_conninfo.cpp)
target_link_libraries(test_conninfo PRIVATE servicioa_objs)
add_test(NAME test_conninfo COMMAND test_conninfo)

add_executable(test_ingest_sample tests/test_ingest_sample.cpp)
target_link_libraries(test_ingest_sample PRIVATE servicioa_objs)
add_test(NAME test_ingest_sample COMMAND test_ingest_sample)
