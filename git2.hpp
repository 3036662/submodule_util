#pragma once
#include <git2.h>
#include <string>
#include <vector>
#include "submodule.hpp"
#include "tag.hpp"

class Git2
{
private:

    std::string repoPath;
    uint max_recursion_depth=20;
    std::vector<Submodule> submodules;

    void CreateTagsRecursive(const std::vector<Submodule>& vec, const std::string &version,
                             std::vector<Tag>& result,std::vector<Submodule>& subm_failed);
    bool FetchRemote(const Submodule& sm);
    bool CreateTag(const Submodule& sm, const std::string& version);
    bool MergeTag(const std::string& tag_name);

    void PrintLastError() const;
public:
    git_repository* ptrRootRepo=nullptr;
    uint err_count=0;

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
    std::vector<Submodule> getSubmodules( git_repository* ptrRepo, const std::string& parent_dir);
    bool createTags(const std::string& version);   

};





