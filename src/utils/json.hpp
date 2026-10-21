#pragma once

#include <nlohmann/json.hpp>

namespace cunqa {
    using JSON = nlohmann::json;

    // Returns null if the file does not exist and an empty object if it is not valid JSON.
    JSON read_file(const std::string &filename);

    // Atomically replaces the content of the file (temporary file + rename), creating the parent
    // directories if needed. Readers see either the old or the new content, never a partial one.
    void write_file(const JSON& data, const std::string &filename);

    // Returns an object {<file stem>: <content>} with every *.json file of the directory, skipping
    // hidden files (as the temporary ones of write_file). A missing directory gives an empty object.
    JSON read_dir(const std::string &dirname);
}
