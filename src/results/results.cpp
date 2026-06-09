#include "results.h"




std::vector<ResultFrameEntry> CollectResultFrameEntriesFromJson(const std::string& jsonFile,
                                                               fs::path* sourceDirectory,
                                                               fs::path* sourceJsonFile,
                                                               std::vector<ResultNodeSet>* nodeSets)
{
    std::vector<ResultFrameEntry> entries;
    fs::path json_path(jsonFile);
    fs::path json_dir = json_path.parent_path();

    if (sourceDirectory != nullptr)
        *sourceDirectory = json_dir;
    if (sourceJsonFile != nullptr)
        *sourceJsonFile = json_path;

    if (!fs::exists(jsonFile)) {
        std::cerr << "Error: JSON file not found: " << jsonFile << std::endl;
        return entries;
    }

    std::ifstream file(jsonFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open JSON file: " << jsonFile << std::endl;
        return entries;
    }

    json data;
    try {
        file >> data;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing results JSON " << jsonFile << ": " << e.what() << std::endl;
        return entries;
    }

    if (nodeSets != nullptr) {
        nodeSets->clear();
        if (data.contains("sets") && data["sets"].is_array()) {
            for (const auto& setEntry : data["sets"]) {
                ResultNodeSet nodeSet;
                nodeSet.setId = setEntry.value("id", -1);
                if (setEntry.contains("nodeIds") && setEntry["nodeIds"].is_array()) {
                    nodeSet.nodeIds = setEntry["nodeIds"].get<std::vector<int>>();
                }
                if (setEntry.contains("directions") && setEntry["directions"].is_array()) {
                    nodeSet.directions = setEntry["directions"].get<std::vector<int>>();
                }
                if (nodeSet.setId >= 0) {
                    nodeSets->push_back(std::move(nodeSet));
                }
            }
        }
    }

    if (!data.contains("vtk_files") || !data["vtk_files"].is_array()) {
        std::cerr << "Error: JSON does not contain valid 'vtk_files' array." << std::endl;
        return entries;
    }

    for (const auto& entry : data["vtk_files"]) {
        if (!entry.contains("file") || !entry.contains("time")) {
            std::cerr << "Warning: Skipping invalid entry in JSON." << std::endl;
            continue;
        }

        fs::path vtk_path = entry["file"].get<std::string>();
        if (vtk_path.is_relative())
            vtk_path = json_dir / vtk_path;

        ResultFrameEntry frameEntry;
        frameEntry.vtkPath = vtk_path;
        frameEntry.time = entry["time"].get<double>();
        entries.push_back(std::move(frameEntry));
    }

    return entries;
}

MultiResult LoadResultsFromJson(const std::string& jsonFile)
{
    MultiResult results;
    std::vector<ResultFrameEntry> entries =
        CollectResultFrameEntriesFromJson(jsonFile,
                                          &results.sourceDirectory,
                                          &results.sourceJsonFile,
                                          &results.nodeSets);

    for (const auto& entry : entries) {
        if (!fs::exists(entry.vtkPath)) {
            std::cerr << "Warning: VTK file not found: " << entry.vtkPath.string() << std::endl;
            continue;
        }

        try {
            auto frame = std::make_unique<ResultFrame>(entry.vtkPath.string());
            frame->time = entry.time;
            results.frames.push_back(std::move(frame));

            // std::cout << "Loaded frame at time " << entry.time << " from " << entry.vtkPath.string() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading " << entry.vtkPath.string() << ": " << e.what() << std::endl;
        }
    }

    // std::cout << "Loaded " << results.frames.size() << " frames from JSON." << std::endl;
    return results;
}
