// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <string>
#include <vector>
#include <fstream>
#ifdef _MSC_VER
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

namespace fastdeploy {

inline std::string PathJoin(const std::vector<std::string>& paths,
                            const std::string& sep = PATH_SEP) {
  if (paths.size() == 1) {
    return paths[0];
  }
  std::string filepath = "";
  for (const auto& path : paths) {
    if (filepath == "") {
      filepath += path;
      continue;
    }
    if (path[0] == sep[0] || filepath.back() == sep[0]) {
      filepath += path;
    } else {
      filepath += sep + path;
    }
  }
  return filepath;
}

inline std::string PathJoin(const std::string& folder,
                            const std::string& filename,
                            const std::string& sep = PATH_SEP) {
  return PathJoin(std::vector<std::string>{folder, filename}, sep);
}

inline std::string GetDirFromPath(const std::string& path) {
  auto pos = path.find_last_of(PATH_SEP);
  if (pos == std::string::npos) {
    return "";
  }
  // The root path in UNIX systems
  if (pos == 0) {
    return "/";
  }
  return path.substr(0, pos);
}

inline bool CheckFileExists(const std::string& path) {
  std::fstream fin(path, std::ios::in);
  if (!fin) {
    return false;
  }
  return true;
}

}  // namespace fastdeploy
