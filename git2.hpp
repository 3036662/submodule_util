#pragma once
#include <git2.h>
#include <string>
#include <vector>
#include "submodule.hpp"

class Git2
{
private:

    std::string repoPath;
    uint max_recursion_depth=20;


public:
    git_repository* ptrRootRepo=nullptr;

    uint curr_recursion_depth=0;
    uint recursion_depth_reached=0;
    uint total_submodules_updated=0;

          Git2();
        ~Git2();

    bool open(const std::string& path);
    bool clone(const std::string& upstream, const std::string& path);

    // C callback for git_submodule_foreach
    static int SubmouduleForeachCallbackC(git_submodule *sm,  const char *name, void *payload);
    std::vector<Submodule> getSubmodules();
    std::vector<Submodule> getSubmodules( git_repository* ptrRepo);
};





