#include "submodule.hpp"
#include <iomanip>

std::ostream& operator << (std::ostream& os, const Submodule& sm){
    os << std::setw(50) << std::setfill('_') <<"\n";
    os << "Submodule name: " << sm.name <<std::endl;
    os << "Head hash : " << sm.headOid << std::endl;
    os << "Path: "  << sm.path << std::endl;
    os << "URL : " << sm.url << std::endl;
    return os;
}


bool Submodule::init(bool override){
    std::cout << "Init submodule " << name << std::endl;
    // update ptrSubmodule
    updatePtrSM();
    // init
    int error=git_submodule_init(ptrSubmodule,override ? 1 : 0);
    if (error){
        std::cerr << "Error. Failed init submodule " << name << std::endl;;
        printLastError();
    }
    freePtrSM();
    return error ? false :true;
}

bool  Submodule::update(bool init){
    // update ptrSubmodule
    updatePtrSM();
    if (!ptrSubmodule)
        return false;
    std::cout << "Updating submodule"<< name << " ..." << std::endl;
    int error= git_submodule_update(ptrSubmodule,init ? 1:0,NULL);
    if (error){
        std::cout << "Error updating submodule " << name << std::endl;
        printLastError();
        return false;
    }
    freePtrSM();
    std::cout << "Update complete" << std::endl;
    return true;
}


// --------------------------------------------------------
// private

void  Submodule::updatePtrSM(){
    if (!owner) return;
    git_submodule* sm;
    int error=  git_submodule_lookup(&sm,owner,name.c_str());
    if (error){
        std::cerr << "Can't find submodule info for submodule "<< name;
        printLastError();
        ptrSubmodule=nullptr;
    }
    ptrSubmodule=sm;
}

void Submodule::freePtrSM(){
    if (ptrSubmodule){
         git_submodule_free(ptrSubmodule);
        ptrSubmodule = nullptr;
    }
}

void Submodule::printLastError() const{
       std::cerr << git_error_last()->message << std::endl;
}
