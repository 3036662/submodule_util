#include <git2.h>

#include <iostream>

#include "git2.hpp"
#include "submodule.hpp"

const char *upstream_url = "https://github.com/3036662/librum-reader_OPDS.git";
const char *path = "/home/oleg/gittest/";

int main() {
    Git2 git;
    if (!git.Open(path)) git.Clone(upstream_url, path);

    std::vector<Submodule> submodules = git.GetSubmodules();
    std::cout << "\nTotal submodules updated : " << git.total_submodules_updated
              << std::endl;
    std::cout << "Recursion level reached: " << git.recursion_depth_reached
              << std::endl;
    if (git.err_count)
        std::cerr << "!!! \nUpdate FAILED for " << git.err_count
                  << " submodules (see WARINGS) " << std::endl;
    // this strategy creating no mess in files but creating terrible mess in git
    git.CreateTags("0.10.2");

    return 0;
}
