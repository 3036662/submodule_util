#include <git2.h>

#include <iostream>

#include "git2.hpp"
#include "options.hpp"
#include "submodule.hpp"

int main(int argc, char* argv[]) {
    /* ------------------------------------
     * Parse programm options
     *  ------------------------------------ */

    Options opts(argc, argv);
    // wrong parameters or --help
    if (opts.wrong_params) return -1;
    if (opts.Help()) return 0;

    // path - required
    std::string path = opts.GetPath();
    if (path.empty()) return 0;

    // url - optional
    std::string url = opts.GetUrl();

    // exclude list if false - error oppening exclude list - exit
    std::optional<std::vector<std::string>> excludes = opts.GetExludes();
    if (!excludes) return -1;

    // package version - required
    std::string pkg_version = opts.GetVersion();
    if (pkg_version.empty()) return 0;

    Git2 git;

    // override commiter if passed
    std::optional<std::pair<std::string, std::string>> override_committer =
        opts.GetCommitter();
    if (override_committer) {
        if (!git.OverrideCommiter(override_committer.value())) {
            std::cerr << "Error occured. Commiter data override failed"
                      << std::endl;
        }
    }

    //-------------------------------------------------
    // open repo or git clone if url is defined

    if (!git.Open(path)) {
        if (!url.empty())
            git.Clone(url, path);
        else {
            std::cerr << "Can't open repo at path " << path << std::endl
                      << "Can't clone because you didn't pass a git URL"
                      << std::endl;
            return 0;
        }
    }

    // -------------------------------------------------
    // Init and update submodules

    git.GetSubmodules(excludes.value());
    std::cout << "\nTotal submodules updated : " << git.total_submodules_updated
              << std::endl;
    std::cout << "Recursion level reached: " << git.recursion_depth_reached
              << std::endl;
    if (git.err_count)
        std::cerr << "!!! \nUpdate FAILED for " << git.err_count
                  << " submodules (see WARINGS) " << std::endl;

    // --------------------------------------------------
    // Create tags and merge
    // this strategy creating no mess in files but creating terrible mess in git
    if (!opts.var_map.count("only-update")) {
        git.CreateTags(pkg_version);
    }

    return 0;
}
