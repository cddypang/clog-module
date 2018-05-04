
#include "log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

/**
 * This implements a "guard" pattern for a CCriticalSection that
 *  borrows most of it's functionality from boost's unique_lock.
 */
class CLogSingleLock : public UniqueLock<CLogCriticalSection>
{
public:
  inline CLogSingleLock(CLogCriticalSection& cs) : UniqueLock<CLogCriticalSection>(cs) {}
  inline CLogSingleLock(const CLogCriticalSection& cs) : UniqueLock<CLogCriticalSection> ((CLogCriticalSection&)cs) {}

  inline void Leave() { unlock(); }
  inline void Enter() { lock(); }
protected:
  inline CLogSingleLock(CLogCriticalSection& cs, bool dicrim) : UniqueLock<CLogCriticalSection>(cs,true) {}
};

/******************************************* Class CLog *************************************************/

static const char* const levelNames[] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

// add 1 to level number to get index of name
static const char* const logLevelNames[] =
{ "LOG_LEVEL_NONE" /*-1*/, "LOG_LEVEL_NORMAL" /*0*/, "LOG_LEVEL_DEBUG" /*1*/, "LOG_LEVEL_DEBUG_FREEMEM" /*2*/ };

// s_globals is used as static global with CLog global variables
XBMC_GLOBAL_REF(CLog, g_logInst);
#define s_globals XBMC_GLOBAL_USE(CLog).m_globalInstance 
//xbmcutil::GlobalsSingleton<CLog>::Deleter<std::shared_ptr<CLog> > xbmcutil::GlobalsSingleton<CLog>::instance;


CLog::CLog()
{
}

CLog::~CLog()
{
}

void CLog::Close()
{
  CLogSingleLock waitLock(s_globals.critSec);
  s_globals.m_platform.CloseLogFile();
  s_globals.m_repeatLine.clear();
}

void CLog::Log(int loglevel, const char *format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    va_list va;
    va_start(va, format);
    LogString(loglevel, StringUtils::FormatV(format, va));
    va_end(va);
  }
}

void CLog::LogFunction(int loglevel, const char* functionName, const char* format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    std::string fNameStr;
    if (functionName && functionName[0])
      fNameStr.assign(functionName).append(": ");
    va_list va;
    va_start(va, format);
    LogString(loglevel, fNameStr + StringUtils::FormatV(format, va));
    va_end(va);
  }
}

void CLog::LogString(int logLevel, const std::string& logString)
{
  CLogSingleLock waitLock(s_globals.critSec);
  std::string strData(logString);
  StringUtils::TrimRight(strData);
  if (!strData.empty())
  {
    if (s_globals.m_repeatLogLevel == logLevel && s_globals.m_repeatLine == strData
        && s_globals.m_lastThreadId == (uint64_t)GetCurrentThreadId())
    {
      s_globals.m_repeatCount++;
      return;
    }
    else if (s_globals.m_repeatCount)
    {
      std::string strData2 = StringUtils::Format("Previous line repeats %d times.",
                                                s_globals.m_repeatCount);
      WriteLogString(s_globals.m_repeatLogLevel, strData2);
      s_globals.m_repeatCount = 0;
    }
    
    s_globals.m_lastThreadId = (unsigned long long)GetCurrentThreadId();
    s_globals.m_repeatLine = strData;
    s_globals.m_repeatLogLevel = logLevel;

    WriteLogString(logLevel, strData);
  }
  
}

bool CLog::Init(const char* path, const char* name)
{
  CLogSingleLock waitLock(s_globals.critSec);

  // the log folder location is initialized in the CAdvancedSettings
  // constructor and changed in CApplication::Create()

  std::string appName(name);
  std::string logPath(path);
  URIUtils::AddSlashAtEnd(logPath);
  return s_globals.m_platform.OpenLogFile(logPath + appName + ".log", logPath + appName + ".old.log");
}

void CLog::MemDump(const char *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    std::string strLine = StringUtils::Format("MEM_DUMP: %04x ", i);
    const char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        std::string strFormat = StringUtils::Format(" %02x", (unsigned char)*pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      if (*alpha > 31)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, "%s", strLine.c_str());
  }
}

void CLog::SetLogLevel(int level)
{
  CLogSingleLock waitLock(s_globals.critSec);
  if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_MAX)
  {
    s_globals.m_logLevel = level;
    CLog::Log(LOGNOTICE, "Log level changed to \"%s\"", logLevelNames[s_globals.m_logLevel + 1]);
  }
  else
    CLog::Log(LOGERROR, "%s: Invalid log level requested: %d", __FUNCTION__, level);
  
}

int CLog::GetLogLevel()
{
  return s_globals.m_logLevel;
}

void CLog::SetExtraLogLevels(int level)
{
  CLogSingleLock waitLock(s_globals.critSec);
  s_globals.m_extraLogLevels = level;
  
}

bool CLog::IsLogLevelLogged(int loglevel)
{
  const int extras = (loglevel & ~LOGMASK);
  if (extras != 0 && (s_globals.m_extraLogLevels & extras) == 0)
    return false;

#if defined(_DEBUG) || defined(PROFILE)
  return true;
#else
  if (s_globals.m_logLevel >= LOG_LEVEL_DEBUG)
    return true;
  if (s_globals.m_logLevel <= LOG_LEVEL_NONE)
    return false;

  // "m_logLevel" is "LOG_LEVEL_NORMAL"
  return (loglevel & LOGMASK) >= LOGNOTICE;
#endif
}

bool CLog::WriteLogString(int logLevel, const std::string& logString)
{
  //static const char* prefixFormat = "%02.2d:%02.2d:%02.2d T:%" PRIu64" %7s: ";
  static const char* prefixFormat = "%04d-%02d-%02d %02.2d:%02.2d:%02.2d T:%llu %7s: ";

  std::string strData(logString);
  /* fixup newline alignment, number of spaces should equal prefix length */
  StringUtils::Replace(strData, "\n", "\n                                            ");

  int year, mon, day, hour, minute, second;
  s_globals.m_platform.GetCurrentLocalTime(year, mon, day, hour, minute, second);
  
  strData = StringUtils::Format(prefixFormat, year, mon, day, hour, minute, second,
                                  (unsigned long long)GetCurrentThreadId(),
                                  levelNames[logLevel]) + strData;

  return s_globals.m_platform.WriteStringToLog(strData);
}

ThreadIdentifier CLog::GetCurrentThreadId()
{
#if defined(__gnu_linux__)
    return pthread_self();
#elif defined(_WIN32)
    return ::GetCurrentThreadId();
#endif
}
