#pragma once

#include <string>

typedef void* HANDLE; // forward declaration, to avoid inclusion of whole Windows.h

class CWin32InterfaceForCLog 
{
public:
  CWin32InterfaceForCLog();
  ~CWin32InterfaceForCLog();
  bool OpenLogFile(const std::string& logFilename, const std::string& backupOldLogToFilename);
  void CloseLogFile(void);
  bool WriteStringToLog(const std::string& logString);
  static void GetCurrentLocalTime(int& year, int& month, int& day,
	  int& hour, int& minute, int& second);
private:
  HANDLE m_hFile;
};
