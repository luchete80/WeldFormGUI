#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <locale>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

class ExportGenerationDirectory {
public:
  ExportGenerationDirectory() = default;
  ExportGenerationDirectory(const ExportGenerationDirectory&) = delete;
  ExportGenerationDirectory& operator=(const ExportGenerationDirectory&) = delete;
  // Resources live beside the project/input file. This directory belongs to
  // the user: never clear it or remove it when an export fails.
  bool create(const std::filesystem::path& parent,
              const std::string& /*stem*/,
              std::string* diagnostic = nullptr) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (diagnostic) *diagnostic = "Cannot create export directory: " + ec.message();
      return false;
    }
    m_path = parent;
    if (diagnostic) diagnostic->clear();
    return true;
  }

  const std::filesystem::path& path() const { return m_path; }
  std::string name() const { return "export"; }
  void retain() {}

private:
  std::filesystem::path m_path;
};

inline bool ExportGenerationRelativePath(const std::filesystem::path& resource,
                                         const std::filesystem::path& referenceFile,
                                         std::string& result,
                                         std::string* diagnostic = nullptr) {
  std::error_code ec;
  const auto absoluteResource = std::filesystem::absolute(resource, ec).lexically_normal();
  if (ec) {
    if (diagnostic != nullptr) *diagnostic = "Cannot resolve generated resource path: " + ec.message();
    return false;
  }
  const auto absoluteReference = std::filesystem::absolute(referenceFile, ec).lexically_normal();
  if (ec) {
    if (diagnostic != nullptr) *diagnostic = "Cannot resolve reference file path: " + ec.message();
    return false;
  }
  const auto relative = absoluteResource.lexically_relative(absoluteReference.parent_path());
  if (relative.empty() || relative.is_absolute()) {
    if (diagnostic != nullptr) *diagnostic = "Generated resource cannot be referenced with a relative path.";
    return false;
  }
  result = relative.generic_string();
  if (diagnostic != nullptr) diagnostic->clear();
  return true;
}

inline bool PackageExternalCsv(const std::string& sourcePath,
                               int materialIndex,
                               const std::filesystem::path& generationDirectory,
                               const std::filesystem::path& referenceFile,
                               std::string& relativePath,
                               std::string* diagnostic = nullptr) {
  namespace fs = std::filesystem;
  auto fail = [diagnostic](const std::string& message) {
    if (diagnostic != nullptr) *diagnostic = message;
    return false;
  };
  std::error_code ec;
  const fs::path source = fs::absolute(fs::path(sourcePath), ec).lexically_normal();
  if (ec) return fail("Cannot resolve external material CSV: " + ec.message());
  if (!fs::is_regular_file(source, ec) || ec)
    return fail("External material CSV is missing or unreadable: " + sourcePath);
  const fs::path destination = generationDirectory /
      ("material_" + std::to_string(materialIndex) + "_flow_stress.csv");
  const bool sameFile = fs::equivalent(source, destination, ec);
  ec.clear();
  if (!sameFile)
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
  if (ec) return fail("Cannot package external material CSV: " + ec.message());
  if (!ExportGenerationRelativePath(destination, referenceFile, relativePath, diagnostic))
    return false;
  return true;
}

inline bool ReplaceFileAtomically(const std::filesystem::path& temporaryFile,
                                  const std::filesystem::path& finalFile,
                                  std::string* diagnostic = nullptr) {
#ifdef _WIN32
  if (MoveFileExW(temporaryFile.c_str(), finalFile.c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    if (diagnostic != nullptr) diagnostic->clear();
    return true;
  }
  if (diagnostic != nullptr)
    *diagnostic = "Cannot atomically replace published file (Windows error " +
        std::to_string(GetLastError()) + ").";
  return false;
#else
  std::error_code ec;
  std::filesystem::rename(temporaryFile, finalFile, ec);
  if (ec) {
    if (diagnostic != nullptr) *diagnostic = "Cannot atomically replace published file: " + ec.message();
    return false;
  }
  if (diagnostic != nullptr) diagnostic->clear();
  return true;
#endif
}

inline bool WriteJsonAndPublish(const nlohmann::json& value,
                                const std::filesystem::path& temporaryFile,
                                const std::filesystem::path& finalFile,
                                std::string* diagnostic = nullptr) {
  auto fail = [diagnostic, &temporaryFile](const std::string& message) {
    std::error_code ignored;
    std::filesystem::remove(temporaryFile, ignored);
    if (diagnostic != nullptr) *diagnostic = message;
    return false;
  };
  try {
    {
      std::ofstream output(temporaryFile, std::ios::out | std::ios::trunc);
      if (!output.is_open()) return fail("Cannot open temporary JSON: " + temporaryFile.string());
      output.imbue(std::locale::classic());
      output << value.dump(4) << '\n';
      output.flush();
      const bool writeFailed = !output.good();
      output.close();
      if (writeFailed || output.fail())
        return fail("Failed while writing or closing temporary JSON: " + temporaryFile.string());
    }
  } catch (const std::exception& error) {
    return fail(std::string("Failed while serializing temporary JSON: ") + error.what());
  } catch (...) {
    return fail("Failed while serializing temporary JSON with an unknown exception.");
  }
  std::string replaceError;
  if (!ReplaceFileAtomically(temporaryFile, finalFile, &replaceError))
    return fail(replaceError);
  if (diagnostic != nullptr) diagnostic->clear();
  return true;
}
