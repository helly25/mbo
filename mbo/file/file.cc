// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mbo/file/file.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"

namespace mbo::file {

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
  std::string_view path_str(path.c_str());
  while (path_str.length() > 1 && (absl::ConsumeSuffix(&path_str, "/") || absl::ConsumeSuffix(&path_str, "\\"))) {}
  return std::filesystem::path(path_str).lexically_normal();
}

absl::Status SetContents(const std::filesystem::path& file_name, std::string_view content) {
  std::ofstream ofs;
  ofs.exceptions(static_cast<std::ios_base::iostate>(0));
  ofs.open(file_name, std::ios_base::binary | std::ios_base::trunc);
  if (!ofs) {
    return absl::UnknownError(absl::StrFormat("Unable to open file: '%s'", file_name));
  }

  ofs << content;
  ofs.close();
  if (!ofs) {
    return absl::UnknownError(absl::StrFormat("Unable to write to file: '%s'", file_name));
  }
  return absl::OkStatus();
}

absl::Status Readable(const std::filesystem::path& file_name) {
  if (!std::filesystem::exists(file_name)) {
    return absl::NotFoundError(absl::StrFormat("File does not exist: '%s'", file_name));
  }
  if (std::filesystem::is_directory(file_name)) {
    return absl::FailedPreconditionError(absl::StrFormat("Cannot open directory for reading: '%s'", file_name));
  }
  // Perform same operation as `GetContents` and use the ifstream constructor
  // to check for readability.
  std::ifstream ifs;
  ifs.exceptions(static_cast<std::ios_base::iostate>(0));
  ifs.open(file_name, std::ios_base::in);
  if (!ifs) {
    return absl::NotFoundError(absl::StrFormat("Unable to read file: '%s'", file_name));
  }

  return absl::OkStatus();
}

absl::StatusOr<std::string> GetContents(const std::filesystem::path& file_name) {
  std::ifstream ifs;
  ifs.exceptions(static_cast<std::ios_base::iostate>(0));
  ifs.open(file_name, std::ios_base::in);
  if (!ifs) {
    return absl::NotFoundError(absl::StrFormat("Unable to read file: '%s'", file_name));
  }
  ifs.seekg(0, std::ios::end);  // As opposed to adding mode `ate`
  auto end = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  auto size = end - ifs.tellg();

  if (size == 0) {
    return "";
  }

  std::string result;
  result.resize(static_cast<std::size_t>(size));

  if (!ifs.read(result.data(), size)) {
    return absl::UnknownError(absl::StrFormat("Unable to read file: '%s'", file_name));
  }

  return result;
}

absl::StatusOr<std::string> GetMaxLines(const std::filesystem::path& file_name, std::size_t max_lines) {
  std::ifstream ifs;
  ifs.exceptions(static_cast<std::ios_base::iostate>(0));
  ifs.open(file_name, std::ios_base::in);
  if (!ifs) {
    return absl::NotFoundError(absl::StrFormat("Unable to read file: '%s'", file_name));
  }

  std::string result;
  std::string line;
  std::size_t curr_line = 0;

  if (ifs.eof()) {
    return result;
  }

  while (curr_line++ < max_lines) {
    line.clear();
    std::getline(ifs, line, '\n');
    absl::StrAppend(&result, line);
    if (ifs.eof()) {
      break;
    }
    absl::StrAppend(&result, "\n");
  }
  return result;
}

absl::StatusOr<absl::Time> GetMTime(const std::filesystem::path& file_name) {
  std::error_code error;
  const auto ftime = std::filesystem::last_write_time(file_name, error);
  if (error) {
    return absl::NotFoundError(absl::StrCat("File error: '", file_name.native(), "': ", error.message()));
  }
  // const auto seconds =
  // std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch());
  const int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
  return absl::FromUnixSeconds(seconds);
}

}  // namespace mbo::file
