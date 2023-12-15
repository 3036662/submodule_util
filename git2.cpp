#include "git2.hpp"
#include <iostream>


Git2::Git2(){
         git_libgit2_init();
}

Git2::~Git2(){
    if (ptrRootRepo){
        git_repository_free(ptrRootRepo);
    }
    git_libgit2_shutdown();
}


bool Git2::open(const std::string& path){
    if (path.empty()) return false;
    repoPath =path;
    int error = git_repository_open(&ptrRootRepo, repoPath.c_str());
    if (error){
        std::cerr<< std::endl << "Failed opening repo at  " <<path << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    return true;
}

bool Git2::clone(const std::string& upstreamUrl, const std::string& path){
    if (upstreamUrl.empty() || path.empty()) return false;
    int error= git_clone(&ptrRootRepo, upstreamUrl.c_str(), path.c_str(), NULL);
    if  (error){
        std::cerr << "error while performing git clone "<< upstreamUrl  << std::endl;
         std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    return true;
}

std::vector<Submodule> Git2::getSubmodules(){
    return getSubmodules(ptrRootRepo);
}

std::vector<Submodule> Git2::getSubmodules(git_repository* ptrRepo){
    std::vector<Submodule> res;
    int error =git_submodule_foreach(ptrRepo,Git2::SubmouduleForeachCallbackC,&res);
    if (error){
        std::cerr << "Error getting submodules"<< std::endl;
         std::cerr << git_error_last()->message << std::endl;
    }
    for (auto it=res.begin(); it !=res.end(); ++it){
        it->owner=ptrRepo;
    }
    for (auto it_sm=res.begin();it_sm != res.end(); ++it_sm){
        std::cout << *it_sm;
        it_sm->init();
        if (it_sm->update())
           ++total_submodules_updated;
        // recursive call - put submoduler submodule.subvec
        if (curr_recursion_depth<max_recursion_depth){
            ++curr_recursion_depth;
            it_sm->subvec = getSubmodules(it_sm->getRepo());
            --curr_recursion_depth;
            if (!it_sm->subvec.empty())
                ++recursion_depth_reached;
        }
        it_sm->freeRepo();
    }
    return res;
}

// C callback for git_submodule_foreach
int Git2::SubmouduleForeachCallbackC(git_submodule *sm,  const char *name_, void *ptrResultVector){
    if (!sm || ! ptrResultVector || !name_) return -1;
    std::vector<Submodule>* ptrRes=static_cast<std::vector<Submodule>*> (ptrResultVector);

    //head oid or NULL
    const git_oid* head_oid=git_submodule_head_id(sm);
    char buffer[100]={0};
    // head hash 40 symbols
    std::string head_oid_Str;
    if (head_oid){
        head_oid_Str=git_oid_tostr(buffer,100,head_oid);
    }
    // name
    std::string name(name_);
    // path
    std::string path;
    const char* ptr_path=git_submodule_path(sm);
    if (ptr_path){
        path=ptr_path;
    }
    // url
    std::string url;
    const char* ptr_url = git_submodule_url(sm);
    if (ptr_url){
        url=ptr_url;
    }
    ptrRes->emplace_back(sm,head_oid,std::move(head_oid_Str),std::move(name),std::move(path),std::move(url));
    return 0;
}
