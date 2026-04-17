#include "results.h"




MultiResult LoadResultsFromJson(const std::string& jsonFile)
{
    MultiResult results;
    fs::path json_path(jsonFile);
    fs::path json_dir = json_path.parent_path();

    if (!fs::exists(jsonFile)) {
        std::cerr << "Error: JSON file not found: " << jsonFile << std::endl;
        return results;
    }

    std::ifstream file(jsonFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open JSON file: " << jsonFile << std::endl;
        return results;
    }

    json data;
    file >> data;

    if (!data.contains("vtk_files") || !data["vtk_files"].is_array()) {
        std::cerr << "Error: JSON does not contain valid 'vtk_files' array." << std::endl;
        return results;
    }

    for (const auto& entry : data["vtk_files"]) {
        if (!entry.contains("file") || !entry.contains("time")) {
            std::cerr << "Warning: Skipping invalid entry in JSON." << std::endl;
            continue;
        }

        std::string vtkFile = entry["file"];
        double time = entry["time"];
        fs::path vtk_path(vtkFile);
        if (vtk_path.is_relative())
            vtk_path = json_dir / vtk_path;

        if (!fs::exists(vtk_path)) {
            std::cerr << "Warning: VTK file not found: " << vtk_path.string() << std::endl;
            continue;
        }

        try {
            auto frame = std::make_unique<ResultFrame>(vtk_path.string());
            results.frames.push_back(std::move(frame));

            std::cout << "Loaded frame at time " << time << " from " << vtk_path.string() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading " << vtk_path.string() << ": " << e.what() << std::endl;
        }
    }

    std::cout << "Loaded " << results.frames.size() << " frames from JSON." << std::endl;
    return results;
}
