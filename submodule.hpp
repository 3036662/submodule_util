#pragma once
#include <iostream>
#include <string>
#include <vector>

#include <git2.h>

class  Submodule
{
    git_submodule* ptrSubmodule = nullptr;
    git_repository*  ptrRepo = nullptr;
public:
    const git_oid* ptrheadOidBinary;
    std::string headOid;
    std::string name;
    std::string path;
    std::string absolute_path;
    std::string url;
    std::string absolute_url;
    git_repository* owner=nullptr;
    std::vector<Submodule> subvec;

    Submodule() =delete;
    Submodule(git_submodule* sm_,
                        const git_oid* hOid,
                        const std::string& headOidStr_,
                        const std::string& name_,
                        const std::string& path_,
                        const std::string& url_)
        : ptrSubmodule(sm_), ptrheadOidBinary(hOid),headOid(headOidStr_), name(name_), path(path_),url(url_){}

    bool init(bool override=true);
    bool update(bool init=false);
    git_repository* getRepo();
    void freeRepo();

private:
    void updatePtrSM();
    void freePtrSM();
    void printLastError() const;

};


std::ostream& operator << (std::ostream& os, const Submodule& sm);
