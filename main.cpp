#include <git2.h>
#include <iostream>
#include "git2.hpp"
#include "submodule.hpp"
#include "options.hpp"

int main(int argc, char* argv[]) {
    Options opts(argc,argv);
    if (opts.wrong_params){
        return -1;
    }
    if (opts.Help()) return 0;
    std::string path =opts.GetPath();
    if (path.empty()){
        std::cerr << "You need to specify path to repository" << std::endl;
        return 0;
    }
    std::string url =opts.GetUrl();
    std::optional<std::vector<std::string>> excludes=opts.GetExludes();
    if (!excludes) return -1;


    Git2 git;
    // open repo or git clone if url is defined
    if (!git.Open(path)){
        if (!url.empty())
            git.Clone(url, path);
    }

    git.GetSubmodules();
    std::cout << "\nTotal submodules updated : "
              << git.total_submodules_updated
              << std::endl;
    std::cout << "Recursion level reached: "
              << git.recursion_depth_reached
              << std::endl;
    if (git.err_count)
        std::cerr << "!!! \nUpdate FAILED for " << git.err_count
                  << " submodules (see WARINGS) " << std::endl;

    // this strategy creating no mess in files but creating terrible mess in git
    git.CreateTags("0.10.2");

    return 0;
}
