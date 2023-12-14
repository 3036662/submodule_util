cmake_minimum_required(VERSION 3.5)

project(submodule_util LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

add_compile_options(-Wall -Wextra)

add_executable(submodule_util main.cpp
    submodule.hpp
    git2.hpp git2.cpp
    submodule.cpp)
target_link_libraries(submodule_util PRIVATE PkgConfig::LIBGIT2)


include(GNUInstallDirs)
install(TARGETS submodule_util
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
