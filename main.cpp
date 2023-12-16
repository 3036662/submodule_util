#include <iostream>
#include  <git2.h>
#include "submodule.hpp"
#include "git2.hpp"


const char* upstream_url ="https://github.com/3036662/librum-reader_OPDS.git";
const char *path = "/home/oleg/gittest/";

int main()
{
    Git2 git;
    if (!git.open(path))
            git.clone(upstream_url,path);

    std::vector<Submodule> submodules=  git.getSubmodules();
    std::cout << "\nTotal submodules updated : " << git.total_submodules_updated << std::endl;
    std::cout << "Recursion level reached: " << git.recursion_depth_reached << std::endl;
    if (git.err_count)
        std::cerr <<"!!! \nUpdate FAILED for " << git.err_count << " submodules (see WARINGS) " << std::endl;
    // old strategy creating no mess in files but creating terrible mess in git log
    git.createTags("0.10.2");



    return 0;
}
