
#include "PosixInterfaceForCLog.h"
#include <stdio.h>
#include <time.h>

struct FILEWRAP : public FILE
{};


CPosixInterfaceForCLog::CPosixInterfaceForCLog() :
  m_file(NULL)
{ }

CPosixInterfaceForCLog::~CPosixInterfaceForCLog()
{
  if (m_file)
    fclose(m_file);
}

bool CPosixInterfaceForCLog::OpenLogFile(const std::string &logFilename, const std::string &backupOldLogToFilename)
{
  if (m_file)
    return false; // file was already opened

  (void)remove(backupOldLogToFilename.c_str()); // if it's failed, try to continue
  (void)rename(logFilename.c_str(), backupOldLogToFilename.c_str()); // if it's failed, try to continue

  m_file = (FILEWRAP*)fopen(logFilename.c_str(), "wb");
  if (!m_file)
    return false; // error, can't open log file

  static const unsigned char BOM[3] = { 0xEF, 0xBB, 0xBF };
  (void)fwrite(BOM, sizeof(BOM), 1, m_file); // write BOM, ignore possible errors

  return true;
}

void CPosixInterfaceForCLog::CloseLogFile()
{
  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }
}

bool CPosixInterfaceForCLog::WriteStringToLog(const std::string &logString)
{
  if (!m_file)
    return false;

  const bool ret = (fwrite(logString.data(), logString.size(), 1, m_file) == 1) &&
                   (fwrite("\n", 1, 1, m_file) == 1);
  (void)fflush(m_file);

  return ret;
}

void CPosixInterfaceForCLog::GetCurrentLocalTime(int& year, int& month, int& day, 
	int &hour, int &minute, int &second)
{
  time_t curTime;
  struct tm localTime;
  if (time(&curTime) != -1 && localtime_r(&curTime, &localTime) != NULL)
  {
    year   = localTime.tm_year;
	month  = localTime.tm_mon;
	day    = localTime.tm_mday;
    hour   = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
  }
  else
    year = month = day = hour = minute = second = 0;
}
