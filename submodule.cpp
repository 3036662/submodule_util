#include "submodule.hpp"

#include <iomanip>

std::ostream& operator<<(std::ostream& os, const Submodule& sm) {
    os << std::setw(50) << std::setfill('_') << "\n";
    os << "Submodule name: " << sm.name << std::endl;
    os << "Head hash : " << sm.head_oid << std::endl;
    os << "Path: " << sm.absolute_path << std::endl;
    os << "URL : " << sm.absolute_url << std::endl;
    return os;
}

bool Submodule::Init(bool override) {
    std::cout << "Init submodule " << name << std::endl;
    // update ptrSubmodule
    UpdatePtrSM();
    // init
    int error = git_submodule_init(ptr_submodule, override ? 1 : 0);
    if (error) {
        std::cerr << "Error. Failed init submodule " << name << std::endl;
        ;
        PrintLastError();
    }
    FreePtrSM();
    return error ? false : true;
}

bool Submodule::Update(bool init) {
    // update ptrSubmodule
    UpdatePtrSM();
    if (!ptr_submodule) return false;
    std::cout << "Updating submodule " << name << " ..." << std::endl;
    int error = git_submodule_update(ptr_submodule, init ? 1 : 0, NULL);
    FreePtrSM();
    if (error) {
        std::cerr << "Error updating submodule " << name << std::endl;
        PrintLastError();
        std::cerr << "Check if " << head_oid << " still exists at "
                  << absolute_url << std::endl;
        return false;
    }
    std::cout << "Update complete" << std::endl;
    return true;
}

// get repo object for submodule
git_repository* Submodule::GetRepo() {
    git_repository* repo = nullptr;
    UpdatePtrSM();
    int error = git_submodule_open(&repo, ptr_submodule);
    if (error) {
        std::cerr << "Error opening submodule " << name;
        PrintLastError();
        return nullptr;
    }
    FreePtrSM();
    ptr_repo = repo;
    return repo;
}

// free repo object
void Submodule::FreeRepo() {
    if (!ptr_repo) return;
    git_repository_free(ptr_repo);
    ptr_repo = nullptr;
}

// --------------------------------------------------------
// private
void Submodule::UpdatePtrSM() {
    if (!owner) return;
    git_submodule* sm;
    int error = git_submodule_lookup(&sm, owner, name.c_str());
    if (error) {
        std::cerr << "Can't find submodule info for submodule " << name;
        PrintLastError();
        ptr_submodule = nullptr;
    }
    ptr_submodule = sm;
}

void Submodule::FreePtrSM() {
    if (ptr_submodule) {
        git_submodule_free(ptr_submodule);
        ptr_submodule = nullptr;
    }
}

void Submodule::PrintLastError() const {
    std::cerr << git_error_last()->message << std::endl;
}
