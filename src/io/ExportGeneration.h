#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
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
  ~ExportGenerationDirectory() {
    if (m_owned && !m_retained) {
      std::error_code ignored;
      std::filesystem::remove_all(m_path, ignored);
    }
  }

  bool create(const std::filesystem::path& parent,
              const std::string& stem,
              std::string* diagnostic = nullptr) {
    namespace fs = std::filesystem;
    auto fail = [diagnostic](const std::string& message) {
      if (diagnostic != nullptr) *diagnostic = message;
      return false;
    };
    std::error_code ec;
    fs::create_directories(parent, ec);
    if (ec) return fail("Cannot create export directory parent: " + ec.message());
    // Reuse the newest generation directory for this export stem. Keeping its
    // name stable prevents every save from leaving another resource package.
    fs::path candidate;
    std::uint64_t newestSequence = 0;
    for (std::uint64_t sequence = 1; sequence <= 100000; ++sequence) {
      const fs::path candidate = parent / (".wf_" + stem + "_generation_" + std::to_string(sequence));
      ec.clear();
      const bool candidateExists = fs::exists(candidate, ec);
      if (ec) return fail("Cannot inspect export generation directory: " + ec.message());
      if (!candidateExists) continue;
      if (fs::is_directory(candidate, ec)) newestSequence = sequence;
      else if (ec) return fail("Cannot inspect export generation directory: " + ec.message());
    }
    if (newestSequence == 0) newestSequence = 1;
    candidate = parent / (".wf_" + stem + "_generation_" + std::to_string(newestSequence));
    bool created = false;
    ec.clear();
    if (!fs::exists(candidate, ec)) {
      ec.clear();
      if (!fs::create_directory(candidate, ec))
        return fail("Cannot create export generation directory: " + ec.message());
      created = true;
    } else {
      if (ec) return fail("Cannot inspect export generation directory: " + ec.message());
      if (!fs::is_directory(candidate, ec) || ec)
        return fail("Export generation path exists but is not a directory.");
      for (fs::directory_iterator it(candidate, ec), end; !ec && it != end; it.increment(ec))
        fs::remove_all(it->path(), ec);
      if (ec) return fail("Cannot clear export generation directory: " + ec.message());
    }
    m_path = candidate;
    m_owned = created;
    if (diagnostic != nullptr) diagnostic->clear();
    return true;
  }

  const std::filesystem::path& path() const { return m_path; }
  std::string name() const { return m_path.filename().string(); }
  void retain() { m_retained = true; }

private:
  std::filesystem::path m_path;
  bool m_owned = false;
  bool m_retained = false;
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
  fs::copy_file(source, destination, fs::copy_options::none, ec);
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
