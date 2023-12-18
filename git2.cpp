#include "git2.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>

Git2::Git2() { git_libgit2_init(); }

Git2::~Git2() {
    if (ptr_root_repo) {
        git_repository_free(ptr_root_repo);
    }
    git_libgit2_shutdown();
}

bool Git2::Open(const std::string& path) {
    if (path.empty()) return false;
    repo_path = path;
    int error = git_repository_open(&ptr_root_repo, repo_path.c_str());
    if (error) {
        std::cerr << std::endl
                  << "Failed opening repo at  " << path << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    return true;
}

bool Git2::Clone(const std::string& upstreamUrl, const std::string& path) {
    if (upstreamUrl.empty() || path.empty()) return false;
    int error =
        git_clone(&ptr_root_repo, upstreamUrl.c_str(), path.c_str(), NULL);
    if (error) {
        std::cerr << "error while performing git clone " << upstreamUrl
                  << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    return true;
}

std::vector<Submodule> Git2::GetSubmodules() {
    return GetSubmodules(ptr_root_repo, "");
}

std::vector<Submodule> Git2::GetSubmodules(git_repository* ptrRepo,
                                           const std::string& parent_dir) {
    std::vector<Submodule> res;
    err_count = 0;
    if (!ptrRepo) {
        return res;
    }
    int error =
        git_submodule_foreach(ptrRepo, Git2::SubmouduleForeachCallbackC, &res);
    if (error) {
        std::cerr << "Error getting submodules" << std::endl;
        std::cerr << git_error_last()->message << std::endl;
    }
    // resolve urls and paths
    for (auto it = res.begin(); it != res.end(); ++it) {
        it->owner = ptrRepo;
        it->absolute_path = parent_dir;
        if (!it->absolute_path.empty()) it->absolute_path += "/";
        it->absolute_path += it->path;
        git_buf gitBuf{0, 0, 0};
        error = git_submodule_resolve_url(&gitBuf, ptrRepo, it->url.c_str());
        if (!error) {
            it->absolute_url = gitBuf.ptr;
            git_buf_dispose(&gitBuf);
        } else
            it->absolute_url = it->url;
    }
    // init + update
    for (auto it_sm = res.begin(); it_sm != res.end(); ++it_sm) {
        std::cout << *it_sm;
        it_sm->Init();
        if (it_sm->Update(false))
            ++total_submodules_updated;
        else {
            std::cerr << "[WARINIG] Couldn't update " << it_sm->name
                      << std::endl;
            ++err_count;
        }
        // recursive call - put submoduler submodule.subvec
        if (curr_recursion_depth < max_recursion_depth) {
            ++curr_recursion_depth;
            it_sm->subvec =
                GetSubmodules(it_sm->GetRepo(), it_sm->absolute_path);
            --curr_recursion_depth;
            if (!it_sm->subvec.empty()) ++recursion_depth_reached;
        }
        it_sm->FreeRepo();
    }
    submodules = res;
    return res;
}

void Git2::CreateTagsRecursive(const std::vector<Submodule>& vec,
                               const std::string& version,
                               std::vector<Tag>& result,
                               std::vector<Submodule>& subm_failed) {
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        // recursive call
        if (!it->subvec.empty()) {
            CreateTagsRecursive(it->subvec, version, result, subm_failed);
        }
        // fetch
        if (!FetchRemote(*it)) {
            ++err_count;
            subm_failed.push_back(*it);
            continue;
        }
        // tag
        std::string tag_name = it->absolute_path + "/" + version;
        if (!CreateTag(*it, tag_name)) {
            ++err_count;
            subm_failed.push_back(*it);
            continue;
        }
        // merge
        if (MergeTag(tag_name))
            result.emplace_back(tag_name, it->absolute_path);
        else
            subm_failed.push_back(*it);
    }
}

bool Git2::CreateTags(const std::string& version) {
    // for each submodule create tag
    std::vector<Tag> res;
    std::vector<Submodule> subm_failed;
    err_count = 0;

    // start recurrent calls
    CreateTagsRecursive(submodules, version, res, subm_failed);

    if (err_count)
        std::cerr << std::endl
                  << "WARNING" << err_count
                  << " errors found while proccessing tags" << std::endl;
    else
        std::cout << "Processing tags is finished.";
    if (!res.empty()) {
        std::cout << std::endl << std::setw(50) << std::setfill('_') << "\n";
        std::cout << "Updates should be commited for tags:\n";
        for (const Tag& tag : res) {
            std::cout << tag.name << std::endl;
        }
        std::cout << std::endl << std::setw(50) << std::setfill('_') << "\n";
        std::cout << "Following gear rules might be needed:" << std::endl;
        // gear rules
        for (const Tag& tag : res) {
            std::string suffix = tag.path;
            boost::replace_all(suffix, "/", "-");
            std::cout << "tar:" << tag.path
                      << "/@version@:. name=@name@-@version@-" << suffix
                      << " base=" << tag.path << std::endl;
        }
        // spec
        std::cout << std::endl << "Spec rules for archives:" << std::endl;
        int arch_counter = 1;
        for (const Tag& tag : res) {
            std::string suffix = tag.path;
            boost::replace_all(suffix, "/", "-");
            std::cout << "Source" << arch_counter << ": "
                      << "%name-%version-" << suffix << ".tar" << std::endl;
            ++arch_counter;
        }
        std::cout << std::endl << "%setup:";
        int arch_counter2 = 0;
        while (arch_counter2 <= arch_counter) {
            std::cout << " -a" << arch_counter2;
            ++arch_counter2;
        }
    }

    if (!subm_failed.empty()) {
        std::cerr << "\nLIST OF FAILED SUBMODULES:" << std::endl;
        for (const Submodule& fail : subm_failed) {
            std::cout << fail << std::endl;
        }
    }
    return err_count ? false : true;
}

// C callback for git_submodule_foreach
int Git2::SubmouduleForeachCallbackC(git_submodule* sm, const char* name_,
                                     void* ptrResultVector) {
    if (!sm || !ptrResultVector || !name_) return -1;
    std::vector<Submodule>* ptrRes =
        static_cast<std::vector<Submodule>*>(ptrResultVector);
    // head oid or NULL
    const git_oid* head_oid = git_submodule_head_id(sm);
    char buffer[100] = {0};
    // head hash 40 symbols
    std::string head_oid_Str;
    if (head_oid) {
        head_oid_Str = git_oid_tostr(buffer, 99, head_oid);
    }
    // name
    std::string name(name_);
    // path
    std::string path;
    const char* ptr_path = git_submodule_path(sm);
    if (ptr_path) {
        path = ptr_path;
    }
    // url
    std::string url;
    const char* ptr_url = git_submodule_url(sm);
    if (ptr_url) {
        url = ptr_url;
    }
    ptrRes->emplace_back(nullptr, head_oid, std::move(head_oid_Str),
                         std::move(name), std::move(path), std::move(url));
    return 0;
}

// -------------- private --------------

bool Git2::FetchRemote(const Submodule& sm) {
    if (sm.absolute_url.empty()) {
        std::cerr << "Empty URL for submodule " << sm.name;
        return false;
    }
    git_remote* tmpRemote = nullptr;
    std::string remote_name = sm.name;
    int error = git_remote_create(&tmpRemote, ptr_root_repo,
                                  remote_name.c_str(), sm.absolute_url.c_str());
    if (error) {
        std::cerr << "Error creating remote for url " << sm.absolute_url
                  << std::endl;
        PrintLastError();
        return false;
    }
    // fetch remote
    std::cout << "Fetching " << sm.absolute_url << std::endl;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_ALL;
    error = git_remote_fetch(tmpRemote, nullptr, &fetch_opts,
                             (std::string("fetch_") + sm.name).c_str());
    if (error) {
        std::cerr << "Error fetching remote for url " << sm.absolute_url
                  << std::endl;
        PrintLastError();
    }
    // detete remote
    git_remote_free(tmpRemote);
    error = git_remote_delete(ptr_root_repo, remote_name.c_str());
    if (error) {
        std::cerr << "Can't delete remote " << sm.name << std::endl;
        PrintLastError();
        return false;
    }
    return true;
}

bool Git2::CreateTag(const Submodule& sm, const std::string& tag_name) {
    // find oid for hsa
    git_oid oid;
    int error = git_oid_fromstr(&oid, sm.head_oid.c_str());
    if (error) {
        std::cerr << "Can't get oid from sha " << sm.head_oid << std::endl;
        PrintLastError();
        return 0;
    }
    // find object by oid
    git_object* ptr_tmp_obj = nullptr;
    error =
        git_object_lookup(&ptr_tmp_obj, ptr_root_repo, &oid, GIT_OBJECT_ANY);
    if (error) {
        std::cerr << "Can't find object " << sm.name << std::endl;
        PrintLastError();
        return 0;
    }
    // create tag
    git_oid created_tag_oid;
    error = git_tag_create_lightweight(&created_tag_oid, ptr_root_repo,
                                       tag_name.c_str(), ptr_tmp_obj, 1);
    git_object_free(ptr_tmp_obj);
    if (error) {
        std::cerr << "Can't create tag " << tag_name << std::endl;
        PrintLastError();
        return false;
    }
    return true;
}

bool Git2::MergeTag(const std::string& tag_name) {
    // reference to tag
    git_reference* tag_reference = nullptr;
    int error =
        git_reference_lookup(&tag_reference, ptr_root_repo,
                             (std::string("refs/tags/") + tag_name).c_str());
    if (error) {
        std::cerr << "Can't find refference to tag" << std::endl;
        PrintLastError();
    }
    // annotated commit
    git_annotated_commit* tag_commit;
    error = git_annotated_commit_from_ref(&tag_commit, ptr_root_repo,
                                          tag_reference);  // FREE
    if (error) {
        std::cerr << "Can't create annotated commit from refference to tag"
                  << std::endl;
        std::cerr << git_error_last()->message << std::endl;
        return false;
    }
    git_reference_free(tag_reference);

    // perfom merge
    std::cout << "Started merging " << tag_name << std::endl;
    git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
    merge_opts.file_favor = GIT_MERGE_FILE_FAVOR_OURS;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy =
        GIT_CHECKOUT_USE_OURS | GIT_CHECKOUT_DONT_UPDATE_INDEX;
    git_merge(ptr_root_repo, (const git_annotated_commit**)&tag_commit, 1,
              &merge_opts, &checkout_opts);
    if (error) {
        std::cerr << "Can't  merge " << tag_name << std::endl;
        PrintLastError();
        git_annotated_commit_free(tag_commit);
        return false;
    }

    // COMMIT
    git_signature* author;
    error = git_signature_now(&author, "ALT submodules util",
                              "proskur@altlinux.org");
    if (error) {
        PrintLastError();
        return false;
    }

    // get index for repo
    git_index* index;
    error = git_repository_index(&index, ptr_root_repo);
    if (error) {
        std::cerr << "Can't find repository index " << std::endl;
        PrintLastError();
        return false;
    }

    // get reference to head
    git_reference* head_ref;
    git_repository_head(&head_ref, ptr_root_repo);
    if (error) {
        std::cerr << "Can't find reference to head " << std::endl;
        PrintLastError();
        return false;
    }

    // get write_tree oid from index
    git_oid tree_oid;
    error = git_index_write_tree(&tree_oid, index);
    if (error) {
        std::cerr << "failed to write merged tree " << std::endl;
        PrintLastError();
        return false;
    }
    git_index_free(index);

    // get tree* from tree_oid
    git_tree* tree;
    git_tree_lookup(&tree, ptr_root_repo, &tree_oid);
    if (error) {
        std::cerr << "failed to lookup tree " << std::endl;
        PrintLastError();
        return false;
    }

    // get head_commit - parent
    git_commit* head_commit = nullptr;
    error = git_reference_peel((git_object**)&head_commit, head_ref,
                               GIT_OBJECT_COMMIT);
    if (error) {
        PrintLastError();
        return false;
    }

    // create git_commit* from git_annotated_commit* (merge_annotated_commit)
    const git_oid* annotated_oid = git_annotated_commit_id(tag_commit);
    git_commit* merge_commit = nullptr;
    error = git_commit_lookup(&merge_commit, ptr_root_repo,
                              annotated_oid);  // TODO free git_commit_free
    git_annotated_commit_free(tag_commit);
    if (error) {
        PrintLastError();
        return false;
    }

    // head_commit and merge_commit are parents for current new commit
    const git_commit* parent[] = {head_commit, merge_commit};
    std::string commit_mesage = "Merge " + tag_name;
    git_oid commit_oid;
    error = git_commit_create(&commit_oid, ptr_root_repo,
                              git_reference_name(head_ref),  // name of HEAD
                              author, author, "UTF-8", commit_mesage.c_str(),
                              tree, 2, parent);
    git_signature_free(author);
    git_reference_free(head_ref);
    git_commit_free(head_commit);
    git_tree_free(tree);
    git_commit_free(merge_commit);
    if (error) {
        std::cerr << "failed to create commit for merge " << std::endl;
        PrintLastError();
        return false;
    }
    git_repository_state_cleanup(ptr_root_repo);
    return true;
}

void Git2::PrintLastError() const {
    std::cerr << git_error_last()->message << std::endl;
}
