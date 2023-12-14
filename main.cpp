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

    std::cout << "Hello World!" << std::endl;
    return 0;
}
