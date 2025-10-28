class Result {
public:
    std::string name;
    std::vector<std::string> vtk_files;
    std::map<std::string,std::string> field_types; // "pl_strain"->"scalar", "DISP"->"vector"
    Job* parent_job; // Referencia al job
};

class Job {
public:
    std::string name;
    Model* model;
    Result results;
};
