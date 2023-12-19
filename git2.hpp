#pragma once
#include <git2.h>

#include <string>
#include <vector>

#include "submodule.hpp"
#include "tag.hpp"

class Git2 {
   private:
    std::string repo_path;
    uint max_recursion_depth = 20;
    std::vector<Submodule> submodules;

    void CreateTagsRecursive(const std::vector<Submodule>& vec,
                             const std::string& version,
                             std::vector<Tag>& result,
                             std::vector<Submodule>& subm_failed);
    bool FetchRemote(const Submodule& sm);
    bool CreateTag(const Submodule& sm, const std::string& version);
    bool MergeTag(const std::string& tag_name);

    void PrintLastError() const;

   public:
    git_repository* ptr_root_repo = nullptr;
    uint err_count = 0;
    uint curr_recursion_depth = 0;
    uint recursion_depth_reached = 0;
    uint total_submodules_updated = 0;
    std::string git_commiter_name;
    std::string git_commiter_email;

    Git2();
    ~Git2();

    bool OverrideCommiter(
        const std::pair<std::string, std::string>& commiter_data);
    bool Open(const std::string& path);
    bool Clone(const std::string& upstream, const std::string& path);

    std::vector<Submodule> GetSubmodules(
        const std::vector<std::string>& excludes);
    std::vector<Submodule> GetSubmodules(
        git_repository* ptrRepo, const std::string& parent_dir,
        const std::vector<std::string>& excludes);
    bool CreateTags(const std::string& version);

    // C callback for git_submodule_foreach
    static int SubmouduleForeachCallbackC(git_submodule* sm, const char* name,
                                          void* payload);
};
