#ifndef _LOG_H_
#define _LOG_H_

struct ExampleAppLog{
  
  void    AddLog(const char* fmt, ...);
  
};

void ShowExampleAppLog(bool* p_open, ExampleAppLog *);

#endif