#ifndef _JOB_H_
#define _JOB_H_

#include <string>
#include <filesystem>

#include "Dialog.h"

class Job:
public Dialog{
public: 
  Job(){}
  Job(std::string );
  virtual void Draw();
  int Run();
  void UpdateOutput(int max_lines = 100);
  std::string & getPathFile(){return m_path_file;}
  std::string & getLog(){return m_log;  }
  void setPathFile(const std::string &path){m_path_file = path;}
  bool isImplicit() const;
protected:
  std::filesystem::path getJobDirectory() const;
  std::filesystem::path getLogFilePath() const;
  std::filesystem::path getTempLogPath() const;
  std::string m_path_file;
  int m_pid;
  std::string m_filename;
  std::string m_log;
  
};






#endif
