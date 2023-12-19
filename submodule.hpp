#pragma once
#include <git2.h>

#include <iostream>
#include <string>
#include <vector>

class Submodule {
    git_submodule* ptr_submodule = nullptr;
    git_repository* ptr_repo = nullptr;

   public:
    const git_oid* ptr_head_oid_binary;
    std::string head_oid;
    std::string name;
    std::string path;
    std::string absolute_path;
    std::string url;
    std::string absolute_url;
    git_repository* owner = nullptr;
    std::vector<Submodule> subvec;
    bool excluded = false;

    Submodule() = delete;
    Submodule(git_submodule* sm_, const git_oid* hOid,
              const std::string& headOidStr_, const std::string& name_,
              const std::string& path_, const std::string& url_)
        : ptr_submodule(sm_),
          ptr_head_oid_binary(hOid),
          head_oid(headOidStr_),
          name(name_),
          path(path_),
          url(url_) {}

    bool Init(bool override = false);
    bool Update(bool init = false);
    git_repository* GetRepo();
    void FreeRepo();

   private:
    void UpdatePtrSM();
    void FreePtrSM();
    void PrintLastError() const;
};

std::ostream& operator<<(std::ostream& os, const Submodule& sm);
