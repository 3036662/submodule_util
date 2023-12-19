#ifndef OPTIONS_HPP
#define OPTIONS_HPP
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <optional>
#include <string>
#include <vector>
namespace po = boost::program_options;

class Options {
   private:
    po::options_description description;
    std::string ResolvePath(const std::string& path) const;

   public:
    bool wrong_params = false;
    po::variables_map var_map;

    Options(int argc, char**& argv);
    bool Help() const;
    std::string GetPath() const;
    std::string GetUrl() const;
    std::string GetVersion() const;
    std::optional<std::pair<std::string, std::string>> GetCommitter() const;
    std::optional<std::vector<std::string>> GetExludes() const;
};

#endif  // OPTIONS_HPP
