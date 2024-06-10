#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include "Domain.h"
#include "LSDynaWriter.h"
#include <iostream>
using namespace std;


// TODO: CHANGE BY GENERIC MODEL
LSDynaWriter:: LSDynaWriter(SPHModel *dom, const std::string& filename){
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }
    file <<"*NODE"<<endl;
    int fieldWidth = 10;
    int decimals = 6;
    for (int i=0;i<dom->Particles.size();i++){
      Vec3_t value = dom->Particles[i]->x ;
      for (int j=0;j<3;j++)
      file << std::setw(fieldWidth) << std::fixed << std::setprecision(decimals) << 
      
      value(j);
      file << std::endl;
      //file << std::setw(fieldWidth) << std::fixed << std::setprecision(decimals) << value << std::endl;
    // for (const double& value : vector) {
        // file << std::setw(fieldWidth) << std::fixed << std::setprecision(decimals) << value << std::endl;
    // }
    }
    file.close();
    
  
}

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



// void exportToLSDyna(const std::vector<double>& vector, const std::string& filename, int fieldWidth = 10, int decimals = 6) {
    // std::ofstream file(filename);

    // if (!file.is_open()) {
        // std::cerr << "Unable to open file: " << filename << std::endl;
        // return;
    // }

    // for (const double& value : vector) {
        // file << std::setw(fieldWidth) << std::fixed << std::setprecision(decimals) << value << std::endl;
    // }

    // file.close();
// }

// int main() {
    // std::vector<double> vector = {1.234567, 123.456789, 0.000001, 987654321.123456};
    // std::string filename = "output.dyna";

    // exportToLSDyna(vector, filename);

    // std::cout << "Data exported to " << filename << std::endl;

    // return 0;
// }