#pragma once
#include <git2.h>
#include <string>
#include <vector>
#include "submodule.hpp"

class Git2
{
public:
        Git2();
        ~Git2();

    bool open(const std::string& path);
    bool clone(const std::string& upstream, const std::string& path);

    // C callback for git_submodule_foreach
    static int SubmouduleForeachCallbackC(git_submodule *sm,  const char *name, void *payload);
    std::vector<Submodule> getSubmodules();

private:
        git_repository* ptrRepo=nullptr;
        std::string repoPath;


};





