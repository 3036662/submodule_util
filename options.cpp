#include "options.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>


Options::Options(int argc,char**& argv)
    :description("Allowed options") {
    description.add_options()
        ("help", "produce this help message")
        ("path", po::value<std::string>(), "path to local repo")
        ("version",po::value<std::string>(), "package vesion from spec")
        ("exclude",po::value<std::string>(),"path to file containig simple list of submodules to exclude")
        ("url", po::value<std::string>(), "git url");
    try{
        po::store(parse_command_line(argc, argv, description), var_map);
    }
    catch(boost::wrapexcept<boost::program_options::invalid_command_line_syntax>& ex){
       std::cerr << "Wrong parameters, see --help"<<std::endl;
       wrong_params=true;
    }
}


bool Options::Help() const{
    if (var_map.size()==0 || var_map.count("help")) {
        std::cout << description << "\n";
        return true;
    }
    return false;
}

std::string Options::GetPath() const{
      std::string local_path;
      if (var_map.count("path")){
          local_path=var_map["path"].as<std::string>();
      }
      local_path =ResolvePath(local_path);
      return local_path;
}

std::string Options::GetUrl() const{
    std::string upstream_url;
    if (var_map.count("url")){
        upstream_url=var_map["url"].as<std::string>();
    }
    return upstream_url;
}

std::optional<std::vector<std::string>> Options::GetExludes() const{
    std::vector<std::string> excludes;
    if (!var_map.count("exclude"))
        return excludes;
    std::string path_to_excludes_file=ResolvePath(var_map["exclude"].as<std::string>());
    if (!std::filesystem::exists(path_to_excludes_file)){
        std::cerr << "File " << path_to_excludes_file << " not found"<<std::endl;
        return {};
    }
    std::ifstream fs(path_to_excludes_file,std::ios::in);
    if (!fs.is_open()){
        std::cerr << "Can't open excludes file" << path_to_excludes_file << std::endl;
        return {};
    }
    std::string exclude_module;
    while (std::getline(fs,exclude_module)){
        if(!exclude_module.empty()){
            excludes.push_back(std::move(exclude_module));
        }
    }
    fs.close();
    return excludes;
}


// -------- private ----------

std::string Options::ResolvePath(const std::string& path) const{
    std::string local_path=path;
    std::string current_path= std::filesystem::current_path();
    current_path+="/";
    std::string home_path = std::filesystem::path(getenv("HOME"));
    home_path+="/";
    if (local_path.empty() || local_path=="."){
        local_path=std::filesystem::current_path();
    }
    if (boost::starts_with(local_path,"./")){
        boost::replace_first(local_path,"./",current_path);
    }
    if (boost::starts_with(local_path,"~/")){
        boost::replace_first(local_path,"~/",home_path);
    }
    std::filesystem::path fs_path= local_path;
    std::error_code ec;
    fs_path = std::filesystem::absolute(fs_path,ec);
    if (ec) std::cerr <<ec.message();
    local_path =fs_path;
    return local_path;
}
