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
