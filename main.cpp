#include <iostream>
#include  <git2.h>
#include "submodule.hpp"
#include "git2.hpp"


const char* upstream_url ="https://github.com/3036662/librum-reader_OPDS.git";
const char *path = "/home/oleg/gittest/";

int main()
{
    Git2 git;
    if (!git.open(path))
            git.clone(upstream_url,path);

    std::vector<Submodule> submodules=  git.getSubmodules();

    std::cout << "Total submodules updated : " << git.total_submodules_updated << std::endl;
    std::cout << "Recursion level reached: " << git.recursion_depth_reached << std::endl;

     //  clone submodules to branches
    /*
     1. create branch
     2. fetch remote
     3. checkout hash
     4. merge to root
     */

   Submodule& sm= submodules[0];

    // create remote
   git_remote* tmpRemote = nullptr;
   std::string remote_name= sm.name;
    int error =git_remote_create(&tmpRemote,git.ptrRootRepo,remote_name.c_str(),sm.url.c_str());
    if (error){
        std::cerr << "Error creating remote for url " << sm.url << std::endl;
        return 0;
    }
    // fetch remote
    error= git_remote_fetch(tmpRemote,nullptr,nullptr,"fetchSubmodule1");
    std::cout << "Fetching " << sm.url << std::endl;
    if (error) {
        std::cerr << "Error fetching remote for url " << sm.url << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }
    // detete remote
    git_remote_free(tmpRemote);
    error = git_remote_delete(git.ptrRootRepo,remote_name.c_str());
    if (error){
        std::cerr << "Can't delete remote "<<sm.name << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }

    // add tag

    //find obj for hsa
    git_oid oid;
    error = git_oid_fromstr(&oid,sm.headOid.c_str());
    if (error){
        std::cerr << "Can't get oid from sha "<<sm.headOid << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return 0;
    }
    git_object* tmpObj=nullptr;
    error = git_object_lookup (&tmpObj,git.ptrRootRepo,&oid,GIT_OBJECT_ANY);
    if (error){
        std::cerr << "Can't find object "<<sm.name << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return 0;
    }
    // create tag
    git_oid created_tag_oid=oid ;
    std:: string tagName = sm.path; // TODO tag name = path from root && pkg verion
    error = git_tag_create_lightweight (&created_tag_oid, git.ptrRootRepo,tagName.c_str(),tmpObj,1);
    if (error){
        std::cerr << "Can't create tag "<< sm.path << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }
    git_object_free(tmpObj);

    //----------------------------
    // merge

    // 1. need annotated commit
    // 2. git_annotated_commit_from_ref to create annotated commit -> need git_reference *
    // 3.

    // reference to tag
    git_reference * tagReference;
    error =git_reference_lookup(&tagReference,git.ptrRootRepo,(std::string("refs/tags/") + tagName).c_str());
    if (error){
        std::cerr << "Can't find refference to tag" << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }



     // annotated commit
    git_annotated_commit * tag_commit;
    error = git_annotated_commit_from_ref(&tag_commit, git.ptrRootRepo, tagReference);
    if (error){
        std::cerr << "Can't create annotated commit from refference to tag" << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }



    // perfom merge
     std::cout << "Started merging " << tagName << std::endl;
     git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
     merge_opts.flags = GIT_MERGE_NO_RECURSIVE ;
     git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
     checkout_opts.checkout_strategy =GIT_CHECKOUT_USE_OURS | GIT_CHECKOUT_DONT_UPDATE_INDEX;
    // merge_opts.ancestor_label = "ours";
     git_merge(git.ptrRootRepo,(const git_annotated_commit **) &tag_commit, 1, &merge_opts, &checkout_opts);
     if (error){
         std::cerr << "Can't  merge " << tagName << std::endl;
         std::cerr << git_error_last()->message << std::endl;
     }
     git_annotated_commit_free(tag_commit);

    return 0;
}
