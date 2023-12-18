#ifndef OPTIONS_HPP
#define OPTIONS_HPP
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <optional>
namespace po =boost::program_options;

class Options {
 private:
    po::options_description description;
    std::string ResolvePath(const std::string& path) const;

 public:
    bool wrong_params=false;
    po::variables_map var_map;

   Options(int argc,char**& argv);
   bool Help() const;
   std::string GetPath() const;
   std::string GetUrl() const;
   std::optional<std::vector<std::string>> GetExludes() const;
};

#endif  // OPTIONS_HPP