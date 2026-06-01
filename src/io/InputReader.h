#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <string>

class Model;

class InputReader {
public:
  explicit InputReader(Model* model);
  bool readFromFile(const std::string& fname);

private:
  Model* m_model = nullptr;
};

#endif
