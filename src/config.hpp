//
//  config.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef config_hpp
#define config_hpp

#include <stdio.h>

#include <yaml-cpp/yaml.h>

namespace ccache {
struct Config {
    struct FileStorage {
        std::string scheme;
        std::string path;
    };

    std::vector<std::string> ignore_path_prefix;
    std::vector<std::string> remove_path_prefix;
    FileStorage file_storage;

    void replace_environment_variables();
};

};  // namespace ccache

namespace YAML {

using namespace ccache;

template <>
struct convert<Config> {
    static Node encode(const Config &rhs) {
        Node node;
        Node ignore_path_prefix = node["ignore_path_prefix"];
        for (const std::string &prefix : rhs.ignore_path_prefix) {
            ignore_path_prefix.push_back(prefix);
        }

        Node remove_path_prefix = node["remove_path_prefix"];
        for (const std::string &prefix : rhs.remove_path_prefix) {
            remove_path_prefix.push_back(prefix);
        }

        if (!rhs.file_storage.scheme.empty()) {
            node["file_storage"] = rhs.file_storage;
        }

        return node;
    }

    static bool decode(const Node &node, Config &config) {
        Node ignore_path_prefix = node["ignore_path_prefix"];
        if (!ignore_path_prefix.IsSequence()) {
            return false;
        }
        config.ignore_path_prefix = ignore_path_prefix.as<std::vector<std::string>>();

        Node remove_path_prefix = node["remove_path_prefix"];
        if (!remove_path_prefix.IsSequence()) {
            return false;
        }
        config.remove_path_prefix = remove_path_prefix.as<std::vector<std::string>>();
        Node file_storage = node["file_storage"];
        config.file_storage = file_storage.as<Config::FileStorage>();
        return true;
    }
};

template <>
struct convert<Config::FileStorage> {
    static Node encode(const Config::FileStorage &rhs) {
        Node node;
        node["scheme"] = rhs.scheme;
        node["path"] = rhs.path;
        return node;
    }

    static bool decode(const Node &node, Config::FileStorage &storage) {
        if (!node.IsMap()) {
            return false;
        }
        storage.scheme = node["scheme"].as<std::string>();
        storage.path = node["path"].as<std::string>();
        return true;
    }
};
};     // namespace YAML
#endif /* config_hpp */

