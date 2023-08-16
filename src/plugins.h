#ifndef _PLUGINS_H_
#define _PLUGINS_H_

// FROM LUMIX ENGINE STUDIO APP
struct GUIPlugin {
  virtual ~GUIPlugin() {}
  virtual void onGUI() = 0;
  virtual bool hasFocus() const { return false; }
  virtual bool onAction(const Action& action) { return false; }
  virtual void update(float) {}
  virtual void pluginAdded(GUIPlugin& plugin) {}
  virtual const char* getName() const = 0;
  virtual bool onDropFile(const char* file) { return false; }
  virtual bool exportData(const char* dest_dir) { return true; }
  virtual void guiEndFrame() {}
  virtual void onSettingsLoaded() {}
  virtual void onBeforeSettingsSaved() {}
};



#endif