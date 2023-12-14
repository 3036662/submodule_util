#pragma once
#include <git2.h>
#include <string>
#include <vector>
#include "submodule.hpp"

class Git2
{
private:
    git_repository* ptrRootRepo=nullptr;
    std::string repoPath;


public:
        Git2();
        ~Git2();

    bool open(const std::string& path);
    bool clone(const std::string& upstream, const std::string& path);

    // C callback for git_submodule_foreach
    static int SubmouduleForeachCallbackC(git_submodule *sm,  const char *name, void *payload);
    std::vector<Submodule> getSubmodules();
    std::vector<Submodule> getSubmodules( git_repository* ptrRepo);
};





