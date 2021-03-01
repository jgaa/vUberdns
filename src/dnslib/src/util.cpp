
#include <filesystem>

#include "boost/process.hpp"

#include "vudnslib/util.h"
#include "warlib/WarLog.h"
#include "restc-cpp/SerializeJson.h"

namespace vuberdns {

using namespace std;

// https://stackoverflow.com/questions/18816126/c-read-the-whole-file-in-buffer
string slurp (const filesystem::path& path) {
  ostringstream buf;
  ifstream input (path);
  buf << input.rdbuf();
  return buf.str();
}

string fileToJson(const filesystem::path &path, bool assumeYaml)
{
    if (!filesystem::is_regular_file(path)) {
        LOG_ERROR << "Not a file: " << path;
        throw runtime_error("Not a file: "s + path.string());
    }

    string json;
    const auto ext = path.extension();
    if (assumeYaml || ext == ".yaml") {
        // https://www.commandlinefu.com/commands/view/12218/convert-yaml-to-json
        const auto expr = R"(import sys, yaml, json; json.dump(yaml.load(open(")"s
                + path.string()
                + R"(","r").read()), sys.stdout, indent=4))"s;
        auto args = boost::process::args({"-c"s, expr});
        boost::process::ipstream pipe_stream;
        boost::process::child process(boost::process::search_path("python"),
                                      args,
                                      boost::process::std_out > pipe_stream);
        string line;
        while (pipe_stream && std::getline(pipe_stream, line)) {
            json += line;
        }

        error_code ec;
        process.wait(ec);
        if (ec) {
            LOG_ERROR << "Failed to convert yaml from " << path << ": " << ec.message();
            throw runtime_error("Failed to convert yaml: "s + path.string());
        }

    } else if (ext == ".json") {
        json = slurp(path);
    } else {
        LOG_ERROR << "File extension must be yaml or json: " << path;
        throw runtime_error("Unknown extension "s + path.string());
    }

    return json;
}

}
