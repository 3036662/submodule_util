#include "git2.hpp"
#include <iostream>
#include <memory>
#include <cstring>
#include <iomanip>
#include <boost/algorithm/string/split.hpp>



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
    return getSubmodules(ptrRootRepo,"");
}

std::vector<Submodule> Git2::getSubmodules(git_repository* ptrRepo, const std::string& parent_dir){
    std::vector<Submodule> res;
    err_count=0;
    if (!ptrRepo){return res;}
    int error =git_submodule_foreach(ptrRepo,Git2::SubmouduleForeachCallbackC,&res);
    if (error){
        std::cerr << "Error getting submodules"<< std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }
    // resolve urls and paths
    for (auto it=res.begin(); it !=res.end(); ++it){
        it->owner=ptrRepo;
        it->absolute_path=parent_dir;
        if (!it->absolute_path.empty())
            it->absolute_path+="/";
        it->absolute_path+=it->path;
        git_buf gitBuf{0,0,0};
        error=git_submodule_resolve_url(&gitBuf,ptrRepo,it->url.c_str());
        if (!error){
            it->absolute_url = gitBuf.ptr;
            git_buf_dispose(&gitBuf);
        }
        else
            it->absolute_url=it->url;
    }
    // init + update
    for (auto it_sm=res.begin();it_sm != res.end(); ++it_sm){
        std::cout << *it_sm;
        it_sm->init();
        if (it_sm->update(false))
           ++total_submodules_updated;
        else{
            std::cerr << "[WARINIG] Couldn't update " << it_sm->name << std::endl;
            ++err_count;
        }
        // recursive call - put submoduler submodule.subvec
        if (curr_recursion_depth<max_recursion_depth){
            ++curr_recursion_depth;
            it_sm->subvec = getSubmodules(it_sm->getRepo(),it_sm->path);
            --curr_recursion_depth;
            if (!it_sm->subvec.empty())
                ++recursion_depth_reached;
        }
        it_sm->freeRepo();
    }
    submodules=res;
    return res;
}


void Git2::CreateTagsRecursive(const std::vector<Submodule>& vec, const std::string& version,
                               std::vector<Tag>& result, std::vector<Submodule>& subm_failed){
    for (auto it=vec.begin();it != vec.end(); ++it){

        // recursive call
        if (!it->subvec.empty()){
            CreateTagsRecursive(it->subvec,version,result, subm_failed);
        }
        // fetch
        if (!FetchRemote(*it)){
            ++err_count;
            subm_failed.push_back(*it);
            continue;
        }
        // tag
        std::string tag_name=it->absolute_path+"/"+version;
        if (!CreateTag(*it,tag_name)){
            ++err_count;
            subm_failed.push_back(*it);
            continue;
        }
        // merge
        if (MergeTag(tag_name))
            result.emplace_back(tag_name,it->absolute_path);
        else
          subm_failed.push_back(*it);
    }
}


bool Git2::createTags(const std::string& version){
    // for each submodule create tag
    std::vector<Tag> res;
    std::vector<Submodule> subm_failed;
    err_count=0;

    // start recurrent calls
    CreateTagsRecursive(submodules,version,res,subm_failed);

    if(err_count)
        std::cerr<< std::endl <<"WARNING" << err_count <<" errors found while proccessing tags" <<std::endl;
    else
        std::cout << "Processing tags is finished.";
    if (!res.empty()){
        std::cout << std::endl << std::setw(50) << std::setfill('_') <<"\n";
        std::cout << "Updates should be commited for tags:\n";
        for (const Tag& tag : res){
            std::cout << tag.name << std::endl;
        }
        std::cout << std::endl << std::setw(50) << std::setfill('_') <<"\n";
        std::cout << "Following gear rules might be needed:" <<std::endl;
        for (const Tag& tag : res){
            std::vector<std::string> split;
            boost::split(split,tag.name,[](const char ch){return ch=='/';});
            std::string suffix;
            if (!split.empty()){
                suffix=split[split.size()-1];
            }
            else {
                suffix=tag.name;
            }
            std::cout << "tar:"
                      << tag.path
                      << "/@version@:. name=@name@-@version@-" << suffix
                      << " base=" << tag.path << std::endl;
        }
    }

    if (!subm_failed.empty()){
        std::cerr << "\nLIST OF FAILED SUBMODULES:" <<std::endl;
        for (const Submodule& fail : subm_failed){
            std::cout << fail << std::endl;
        }
    }
    return err_count ? false :true;
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

// -------------- private --------------


bool Git2::FetchRemote(const Submodule& sm){
    if (sm.absolute_url.empty()){
        std::cerr << "Empty URL for submodule " << sm.name;
        return false;
    }
    git_remote* tmpRemote = nullptr;
    std::string remote_name= sm.name;
    int error =git_remote_create(&tmpRemote,ptrRootRepo,remote_name.c_str(),sm.absolute_url.c_str());
    if (error){
        std::cerr << "Error creating remote for url " << sm.absolute_url << std::endl;
        PrintLastError();
        return false;
    }
    // fetch remote
    std::cout << "Fetching " << sm.absolute_url << std::endl;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    fetch_opts.download_tags=GIT_REMOTE_DOWNLOAD_TAGS_ALL;
    error= git_remote_fetch(tmpRemote,nullptr,&fetch_opts,(std::string("fetch_")+sm.name).c_str());
    if (error) {
        std::cerr << "Error fetching remote for url " << sm.absolute_url << std::endl;
        PrintLastError();
        return false;
    }
    // detete remote
    git_remote_free(tmpRemote);
    error = git_remote_delete(ptrRootRepo,remote_name.c_str());
    if (error){
        std::cerr << "Can't delete remote "<<sm.name << std::endl;
        PrintLastError();
        return false;
    }
    return true;
}

bool Git2::CreateTag(const Submodule& sm, const std::string& tag_name){
    //find oid for hsa
    git_oid oid;
    int error = git_oid_fromstr(&oid,sm.headOid.c_str());
    if (error){
      std::cerr << "Can't get oid from sha "<<sm.headOid << std::endl;
      PrintLastError();
      return 0;
    }
    // find object by oid
    git_object* ptr_tmp_obj=nullptr;
    error = git_object_lookup (&ptr_tmp_obj,ptrRootRepo,&oid,GIT_OBJECT_ANY);
    if (error){
        std::cerr << "Can't find object "<<sm.name << std::endl;
        PrintLastError();
        return 0;
    }
    // create tag
    git_oid& created_tag_oid=oid ;
    error = git_tag_create_lightweight (&created_tag_oid, ptrRootRepo,tag_name.c_str(),ptr_tmp_obj,1);
    if (error){
        std::cerr << "Can't create tag "<< tag_name << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        git_object_free(ptr_tmp_obj);
        return false;
    }
    git_object_free(ptr_tmp_obj);
    return true;
}

bool Git2::MergeTag(const std::string& tag_name){
    // reference to tag
    git_reference * tag_reference;
    int error = git_reference_lookup(&tag_reference,ptrRootRepo,
                                     (std::string("refs/tags/") + tag_name).c_str());
    if (error){
      std::cerr << "Can't find refference to tag" << std::endl;
      PrintLastError();
    }
    // annotated commit
    git_annotated_commit* tag_commit;
    error = git_annotated_commit_from_ref(&tag_commit, ptrRootRepo, tag_reference);
    if (error){
        std::cerr << "Can't create annotated commit from refference to tag" << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    // perfom merge
    std::cout << "Started merging " << tag_name << std::endl;
    git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
    merge_opts.flags = GIT_MERGE_NO_RECURSIVE ;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy =GIT_CHECKOUT_USE_OURS | GIT_CHECKOUT_DONT_UPDATE_INDEX;
    // merge_opts.ancestor_label = "ours";
    git_merge(ptrRootRepo,(const git_annotated_commit **) &tag_commit, 1, &merge_opts, &checkout_opts);
    if (error){
      std::cerr << "Can't  merge " << tag_name << std::endl;
      PrintLastError();
      git_annotated_commit_free(tag_commit);
      return false;
    }
    git_annotated_commit_free(tag_commit);

    return true;
}




void Git2::PrintLastError() const{
    std::cerr << git_error_last()->message << std::endl;
}
