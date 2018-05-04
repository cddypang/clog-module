
#include "Win32InterfaceForCLog.h"
#include "utils/StringUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

CWin32InterfaceForCLog::CWin32InterfaceForCLog() :
  m_hFile(INVALID_HANDLE_VALUE)
{ }

CWin32InterfaceForCLog::~CWin32InterfaceForCLog()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
}

bool CWin32InterfaceForCLog::OpenLogFile(const std::string& logFilename, const std::string& backupOldLogToFilename)
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    return false; // file was already opened

  if (logFilename.empty())
    return false;

  if (!backupOldLogToFilename.empty())
  {
    (void)DeleteFile(backupOldLogToFilename.c_str()); // if it's failed, try to continue
    (void)MoveFile(logFilename.c_str(), backupOldLogToFilename.c_str()); // if it's failed, try to continue
  }

  m_hFile = CreateFile(logFilename.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  static const unsigned char BOM[3] = { 0xEF, 0xBB, 0xBF };
  DWORD written;
  (void)WriteFile(m_hFile, BOM, sizeof(BOM), &written, NULL); // write BOM, ignore possible errors
  (void)FlushFileBuffers(m_hFile);

  return true;
}

void CWin32InterfaceForCLog::CloseLogFile(void)
{
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
  }
}

bool CWin32InterfaceForCLog::WriteStringToLog(const std::string& logString)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  std::string strData(logString);
  StringUtils::Replace(strData, "\n", "\r\n");
  strData += "\r\n";

  DWORD written;
  const bool ret = (WriteFile(m_hFile, strData.c_str(), strData.length(), &written, NULL) != 0) && written == strData.length();

  return ret;
}

void CWin32InterfaceForCLog::GetCurrentLocalTime(int& year, int& month, int& day, 
	int& hour, int& minute, int& second)
{
  SYSTEMTIME time;
  GetLocalTime(&time);
  year = time.wYear;
  month = time.wMonth;
  day = time.wDay;
  hour = time.wHour;
  minute = time.wMinute;
  second = time.wSecond;
}
