//
//  ProjectConfig.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef ProjectConfig_hpp
#define ProjectConfig_hpp

#include <stdio.h>

#include <yaml-cpp/yaml.h>

namespace ccache {
struct ProjectConfig {
    struct FileStorage {
        std::string scheme;
        std::string host;
    };

    std::vector<std::string> ignore_path_prefix;
    std::vector<std::string> remove_path_prefix;
    std::string log_dir;
    bool console_log{false};
    bool key_by_content{false};
    bool key_by_time{true};
    FileStorage file_storage;

    void replace_environment_variables();
};

};  // namespace ccache

namespace YAML {

using namespace ccache;

template <>
struct convert<ProjectConfig> {
    static Node encode(const ProjectConfig &rhs) {
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
        if (!rhs.log_dir.empty()) {
            node["log_dir"] = rhs.log_dir;
        }
        node["console_log"] = rhs.console_log;
        node["key_by_content"] = rhs.key_by_content;
        node["key_by_time"] = rhs.key_by_time;
        return node;
    }

    static bool decode(const Node &node, ProjectConfig &config) {
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
        config.file_storage = file_storage.as<ProjectConfig::FileStorage>();
        Node log_dir_node = node["log_dir"];
        if (log_dir_node.IsDefined()) {
            config.log_dir = log_dir_node.as<std::string>();
        }
        Node console_log_node = node["console_log"];
        if (console_log_node.IsDefined()) {
            config.console_log = console_log_node.as<bool>();
        }

        Node key_by_content_node = node["key_by_content"];
        if (key_by_content_node.IsDefined()) {
            config.key_by_content = key_by_content_node.as<bool>();
        }

        Node key_by_time_node = node["key_by_time"];
        if (key_by_time_node.IsDefined()) {
            config.key_by_time = key_by_time_node.as<bool>();
        }
        return true;
    }
};

template <>
struct convert<ProjectConfig::FileStorage> {
    static Node encode(const ProjectConfig::FileStorage &rhs) {
        Node node;
        node["scheme"] = rhs.scheme;
        node["host"] = rhs.host;
        return node;
    }

    static bool decode(const Node &node, ProjectConfig::FileStorage &storage) {
        if (!node.IsMap()) {
            return false;
        }
        storage.scheme = node["scheme"].as<std::string>();
        storage.host = node["host"].as<std::string>();
        return true;
    }
};
};     // namespace YAML
#endif /* config_hpp */

