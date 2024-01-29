#include "options.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

Options::Options(int argc, char**& argv) : description("Allowed options") {
    description.add_options()("help", "produce this help message")(
        "path", po::value<std::string>(), "path to local repo")(
        "version", po::value<std::string>(), "package version from spec")(
        "exclude", po::value<std::string>(),
        "path to file containig simple list of submodule names to exclude (one "
        "line - one name)")("url", po::value<std::string>(),
                            "git url to clone if path don't exist")(
        "committer", po::value<std::string>(),
        "Name to override global git config committer. Example \"Jhon Doe\"")(
        "email", po::value<std::string>(),
        "Email to override global git config committer email")
        ("only-update","Only init and update submodules, no tags,no merges");
    try {
        po::store(parse_command_line(argc, argv, description), var_map);
    } catch (
        boost::wrapexcept<boost::program_options::invalid_command_line_syntax>&
            ex) {
        std::cerr << "Wrong parameters, see --help" << std::endl;
        wrong_params = true;
    } catch (boost::wrapexcept<boost::program_options::unknown_option>& ex) {
        std::cerr << "Unknown option passed." << std::endl
                  << ex.what() << std::endl;
        wrong_params = true;
    }
}

bool Options::Help() const {
    std::cout << "This tool recursively updates git submodules, create tags "
                 "and merges them into the current branch"
              << std::endl;
    if (var_map.size() == 0 || var_map.count("help")) {
        std::cout << description << "\n";
        return true;
    }
    return false;
}

std::string Options::GetPath() const {
    std::string local_path;
    if (var_map.count("path")) {
        local_path = var_map["path"].as<std::string>();
        local_path = ResolvePath(local_path);
    } else {
        std::cerr << "You need to specify path to repository" << std::endl;
    }
    return local_path;
}

std::string Options::GetUrl() const {
    std::string upstream_url;
    if (var_map.count("url")) {
        upstream_url = var_map["url"].as<std::string>();
    }
    return upstream_url;
}

std::optional<std::vector<std::string>> Options::GetExludes() const {
    std::vector<std::string> excludes;
    if (!var_map.count("exclude")) return excludes;
    std::string path_to_excludes_file =
        ResolvePath(var_map["exclude"].as<std::string>());
    if (!std::filesystem::exists(path_to_excludes_file)) {
        std::cerr << "File " << path_to_excludes_file << " not found"
                  << std::endl;
        return {};
    }
    std::ifstream fs(path_to_excludes_file, std::ios::in);
    if (!fs.is_open()) {
        std::cerr << "Can't open excludes file" << path_to_excludes_file
                  << std::endl;
        return {};
    }
    std::string exclude_module;
    while (std::getline(fs, exclude_module)) {
        if (!exclude_module.empty()) {
            excludes.push_back(std::move(exclude_module));
        }
    }
    fs.close();
    return excludes;
}

std::string Options::GetVersion() const {
    std::string version;
    if (var_map.count("version")) {
        version = var_map["version"].as<std::string>();
    } else {
        std::cerr
            << "Package version is required (must be same as in the .spec file"
            << std::endl;
    }
    return version;
}

std::optional<std::pair<std::string, std::string>> Options::GetCommitter()
    const {
    std::string name;
    std::string email;
    switch (var_map.count("committer") + var_map.count("email")) {
        case 0:
            return {};
            break;
        case 1:
            std::cerr << "You need to provide both --committer  and --email"
                      << std::endl;
            return {};
            break;
        default:;
    }

    name = var_map["committer"].as<std::string>();
    email = var_map["email"].as<std::string>();
    if (name.empty() || email.empty()) return {};
    return std::make_pair<std::string, std::string>(std::move(name),
                                                    std::move(email));
}

// -------- private ----------

std::string Options::ResolvePath(const std::string& path) const {
    std::string local_path = path;
    std::string current_path = std::filesystem::current_path();
    current_path += "/";
    std::string home_path = std::filesystem::path(getenv("HOME"));
    home_path += "/";
    if (local_path.empty() || local_path == ".") {
        local_path = std::filesystem::current_path();
    }
    if (boost::starts_with(local_path, "./")) {
        boost::replace_first(local_path, "./", current_path);
    }
    if (boost::starts_with(local_path, "~/")) {
        boost::replace_first(local_path, "~/", home_path);
    }
    std::filesystem::path fs_path = local_path;
    std::error_code ec;
    fs_path = std::filesystem::absolute(fs_path, ec);
    if (ec) std::cerr << ec.message();
    local_path = fs_path;
    return local_path;
}
