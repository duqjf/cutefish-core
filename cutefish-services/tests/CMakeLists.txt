cmake_minimum_required(VERSION 3.22)
project(services_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(CutefishToolkit REQUIRED)
find_package(GTest REQUIRED)
include(GoogleTest)
enable_testing()

set(SERVICES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(crash-dialog-test crash_dialog_test.cpp ${SERVICES_DIR}/crash/crashdialog.cpp
               ${SERVICES_DIR}/crash/coredump.cpp)
target_include_directories(crash-dialog-test PRIVATE ${SERVICES_DIR}/crash)
target_link_libraries(crash-dialog-test PRIVATE Cutefish::Toolkit GTest::gtest)
add_test(NAME crash-dialog COMMAND crash-dialog-test)

add_executable(coredump-test coredump_test.cpp ${SERVICES_DIR}/crash/coredump.cpp)
target_include_directories(coredump-test PRIVATE ${SERVICES_DIR}/crash)
target_link_libraries(coredump-test PRIVATE Cutefish::Toolkit GTest::gtest_main)
add_test(NAME coredump COMMAND coredump-test)

add_executable(gtk-css-test gtk_css_test.cpp ${SERVICES_DIR}/appearance/gtksettings.cpp ${SERVICES_DIR}/log.cpp
               ${SERVICES_DIR}/../common/gsettings_keyfile.cpp)
target_include_directories(gtk-css-test PRIVATE ${SERVICES_DIR} ${SERVICES_DIR}/..)
target_compile_definitions(gtk-css-test PRIVATE CUTEFISH_GTK_CSS_DIR="${SERVICES_DIR}/appearance/gtk")
target_link_libraries(gtk-css-test PRIVATE Cutefish::Toolkit GTest::gtest_main)
add_test(NAME gtk-css COMMAND gtk-css-test)
