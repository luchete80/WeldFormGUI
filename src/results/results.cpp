#include "results.h"




MultiResult LoadResultsFromJson(const std::string& jsonFile)
{
    MultiResult results;

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

        if (!fs::exists(vtkFile)) {
            std::cerr << "Warning: VTK file not found: " << vtkFile << std::endl;
            continue;
        }

        try {
            auto frame = std::make_unique<ResultFrame>(vtkFile);
            results.frames.push_back(std::move(frame));

            std::cout << "Loaded frame at time " << time << " from " << vtkFile << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading " << vtkFile << ": " << e.what() << std::endl;
        }
    }

    std::cout << "Loaded " << results.frames.size() << " frames from JSON." << std::endl;
    return results;
}
