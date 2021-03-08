
#include <string>
#include <filesystem>

#include "restc-cpp/SerializeJson.h"

namespace vuberdns {

std::string slurp (const std::filesystem::path& path);
std::string fileToJson(const std::filesystem::path &pathToFile, bool assumeYaml = false);

/*! Reads the contents from a .json or .yaml file and serialize it to obj */
template <typename T>
void fileToObject(T& obj, const std::filesystem::path& pathToFile) {
    const auto json = fileToJson(pathToFile);
    std::istringstream ifs{json};
    restc_cpp::serialize_properties_t properties;
    properties.ignore_unknown_properties = false;
    restc_cpp::SerializeFromJson(obj, ifs, properties);
}

template <typename T>
std::string toJson(const T& obj) {
    std::ostringstream out;
    restc_cpp::serialize_properties_t properties;
    restc_cpp::SerializeToJson(obj, out, properties);
    return out.str();
}

/*! Split a data string / view into an array of string_views.
 */
template <typename viewT = std::string_view, typename retV = std::vector<viewT>, typename strT>
auto Split(const strT &data, const char delimiter = '/')
{
    retV rval;
    viewT w{data.data(), data.size()};
    do {
        viewT k;
        if (const auto pos = w.find(delimiter) ; pos != viewT::npos) {
            if (pos > 0) {
                k = w.substr(0, pos);
            }
            w = w.substr(pos +1);
        } else {
            k = w;
            w = {};
        }

        if (!k.empty()) {
            rval.emplace_back(k);
        }
    } while (!w.empty());

    return rval;
}
}
