#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

void exportToLSDyna(const std::vector<double>& vector, const std::string& filename, int fieldWidth = 10, int decimals = 6) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    for (const double& value : vector) {
        file << std::setw(fieldWidth) << std::fixed << std::setprecision(decimals) << value << std::endl;
    }

    file.close();
}

// int main() {
    // std::vector<double> vector = {1.234567, 123.456789, 0.000001, 987654321.123456};
    // std::string filename = "output.dyna";

    // exportToLSDyna(vector, filename);

    // std::cout << "Data exported to " << filename << std::endl;

    // return 0;
// }