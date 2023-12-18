#ifndef TAG_HPP
#define TAG_HPP
#include <string>

struct Tag
{
    std::string name;
    std::string path;
    Tag(const std::string& name_,const std::string& path_):name{name_},path{path_}{}
};

#endif // TAG_HPP
