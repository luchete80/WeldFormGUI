#ifndef _LSDYNAWRITER_
#define _LSDYNAWRITER_

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

class SPH::Domain; //SPH DOMAIN

class LSDynaWriter {
public:
  LSDynaWriter(SPH::Domain *dom, const std::string& filename);




};


// int main() {
    // std::vector<double> vector = {1.234567, 123.456789, 0.000001, 987654321.123456};
    // std::string filename = "output.dyna";

    // exportToLSDyna(vector, filename);

    // std::cout << "Data exported to " << filename << std::endl;

    // return 0;
// }

#endif