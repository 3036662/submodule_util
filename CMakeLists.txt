cmake_minimum_required(VERSION 3.5)

project(submodule_util LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)


find_package(Boost REQUIRED  COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})


add_compile_options(-Wall -Wextra)

add_executable(submodule_util main.cpp
    submodule.hpp
    git2.hpp git2.cpp
    submodule.cpp
    tag.hpp
    options.hpp options.cpp)
target_link_libraries(submodule_util PRIVATE PkgConfig::LIBGIT2)
target_link_libraries(submodule_util PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY})

include(GNUInstallDirs)
install(TARGETS submodule_util
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
