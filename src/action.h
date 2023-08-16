#ifndef _ACTION_H_
#define _ACTION_H_

#include <string>

//TODO: CHEF IF ISNT BETTER AS STRUCT LIKE IN GODOT: godot/editor/editor_undo_redo_manager.h

class Action {
  
public:
  Action(){m_description = "UNDEFINED";}
  virtual std::string getDescription();
  virtual bool isEndedWithSelection();
  virtual bool isEntityArray();
  virtual ~Action();
  
protected:
  string m_description;
};

class CreateSet:
public Action {
  
  std::string getDescription() {return "CreatingSet";};
};

#endif