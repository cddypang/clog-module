#pragma once

#include <stdio.h>
#include <string>

#ifdef WIN32
#include <Windows.h>
#else
#define MAX_PATH 260
#include <pthread.h>
#endif

#define LOG_LEVEL_NONE         -1 // nothing at all is logged
#define LOG_LEVEL_NORMAL        0 // shows notice, error, severe and fatal
#define LOG_LEVEL_DEBUG         1 // shows all
#define LOG_LEVEL_DEBUG_FREEMEM 2 // shows all + shows freemem on screen
#define LOG_LEVEL_MAX           LOG_LEVEL_DEBUG_FREEMEM

// ones we use in the code
#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

// extra masks - from bit 5
#define LOGMASKBIT  5
#define LOGMASK     ((1 << LOGMASKBIT) - 1)

#include "GlobalsHandling.h"
#include "utils/params_check_macros.h"

#if defined(__gnu_linux__) || defined(__ANDROID__)
#include "PosixInterfaceForCLog.h"
typedef class CPosixInterfaceForCLog PlatformInterfaceForCLog;
typedef pthread_t ThreadIdentifier;

static pthread_mutexattr_t recursiveAttr;

static bool setRecursiveAttr() 
{
    static bool alreadyCalled = false; // initialized to 0 in the data segment prior to startup init code running
    if (!alreadyCalled)
    {
        pthread_mutexattr_init(&recursiveAttr);
        pthread_mutexattr_settype(&recursiveAttr,PTHREAD_MUTEX_RECURSIVE);
        alreadyCalled = true;
    }
    return true; // note, we never call destroy.
}

static bool recursiveAttrSet = setRecursiveAttr();

class RecursiveMutex
{
    pthread_mutexattr_t* getRecursiveAttr()
    {
        if (!recursiveAttrSet) // this is only possible in the single threaded startup code
            recursiveAttrSet = setRecursiveAttr();
        return &recursiveAttr;
    }
    pthread_mutex_t mutex;
public:
    inline RecursiveMutex() { pthread_mutex_init(&mutex,getRecursiveAttr()); }

    inline ~RecursiveMutex() { pthread_mutex_destroy(&mutex); }

    inline void lock() { pthread_mutex_lock(&mutex); }

    inline void unlock() { pthread_mutex_unlock(&mutex); }

    inline bool try_lock() { return (pthread_mutex_trylock(&mutex) == 0); }
};


#elif defined(WIN32)
#include "Win32InterfaceForCLog.h"
typedef class CWin32InterfaceForCLog PlatformInterfaceForCLog;
typedef unsigned long ThreadIdentifier;

class RecursiveMutex
{
    CRITICAL_SECTION mutex;
public:
    inline RecursiveMutex()
    {
        InitializeCriticalSection(&mutex);
    }

    inline ~RecursiveMutex()
    {
        DeleteCriticalSection(&mutex);
    }

    inline void lock()
    {
        EnterCriticalSection(&mutex);
    }

    inline void unlock()
    {
        LeaveCriticalSection(&mutex);
    }

    inline bool try_lock()
    {
        return TryEnterCriticalSection(&mutex) ? true : false;
    }
};

#endif


  /**
   * Any class that inherits from NonCopyable will ... not be copyable (Duh!)
   */
  class NonCopyable
  {
    inline NonCopyable(const NonCopyable& ) {}
    inline NonCopyable& operator=(const NonCopyable& ) { return *this; }
  public:
    inline NonCopyable() {}
  };

    /**
   * This template will take any implementation of the "Lockable" concept
   * and allow it to be used as an "Exitable Lockable."
   *
   * Something that implements the "Lockable concept" simply means that 
   * it has the three methods:
   *
   *   lock();
   *   try_lock();
   *   unlock();
   *
   * "Exitable" specifially means that, no matter how deep the recursion
   * on the mutex/critical section, we can exit from it and then restore
   * the state.
   *
   * This requires us to extend the Lockable so that we can keep track of the
   * number of locks that have been recursively acquired so that we can
   * undo it, and then restore that (See class CSingleExit).
   *
   * All xbmc code expects Lockables to be recursive.
   */
  template<class L> class CountingLockable : public NonCopyable
  {
    friend class ConditionVariable;
  protected:
    L mutex;
    unsigned int count;

  public:
    inline CountingLockable() : count(0) {}

    // boost::thread Lockable concept
    inline void lock() { mutex.lock(); count++;}
    inline bool try_lock() { return mutex.try_lock() ? count++, true : false; }
    inline void unlock() { count--; mutex.unlock();}

    /**
     * This implements the "exitable" behavior mentioned above.
     *
     * This can be used to ALMOST exit, but not quite, by passing
     *  the number of locks to leave. This is used in the windows
     *  ConditionVariable which requires that the lock be entered
     *  only once, and so it backs out ALMOST all the way, but
     *  leaves one still there.
     */
    inline unsigned int exit(unsigned int leave = 0) 
    { 
      // it's possibe we don't actually own the lock
      // so we will try it.
      unsigned int ret = 0;
      if (try_lock())
      {
        if (leave < (count - 1))
        {
          ret = count - 1 - leave;  // The -1 is because we don't want 
                                    //  to count the try_lock increment.
          // We must NOT compare "count" in this loop since 
          // as soon as the last unlock is called another thread
          // can modify it.
          for (unsigned int i = 0; i < ret; i++)
            unlock();
        }
        unlock(); // undo the try_lock before returning
      }

      return ret; 
    }

    /**
     * Restore a previous exit to the provided level.
     */
    inline void restore(unsigned int restoreCount)
    {
      for (unsigned int i = 0; i < restoreCount; i++) 
        lock();
    }

    /**
     * Some implementations (see pthreads) require access to the underlying 
     *  CCriticalSection, which is also implementation specific. This 
     *  provides access to it through the same method on the guard classes
     *  UniqueLock, and SharedLock.
     *
     * There really should be no need for the users of the threading library
     *  to call this method.
     */
    inline L& get_underlying() { return mutex; }
  };

  /**
   * This template can be used to define the base implementation for any UniqueLock
   * (such as CSingleLock) that uses a Lockable as its mutex/critical section.
   */
  template<typename L> class UniqueLock : public NonCopyable
  {
  protected:
    L& mutex;
    bool owns;
    inline UniqueLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock(); }
    inline UniqueLock(L& lockable, bool try_to_lock_discrim ) : mutex(lockable) { owns = mutex.try_lock(); }
    inline ~UniqueLock() { if (owns) mutex.unlock(); }

  public:

    inline bool owns_lock() const { return owns; }

    //This also implements lockable
    inline void lock() { mutex.lock(); owns=true; }
    inline bool try_lock() { return (owns = mutex.try_lock()); }
    inline void unlock() { if (owns) { mutex.unlock(); owns=false; } }

    /**
     * See the note on the same method on CountingLockable
     */
    inline L& get_underlying() { return mutex; }
  };

class CLogCriticalSection : public CountingLockable<RecursiveMutex> {};

class CLog
{
public:
  CLog();
  ~CLog(void);
  static void Close();
  static void Log(int loglevel, PRINTF_FORMAT_STRING const char *format, ...) PARAM2_PRINTF_FORMAT;
  static void LogFunction(int loglevel, IN_OPT_STRING const char* functionName, PRINTF_FORMAT_STRING const char* format, ...) PARAM3_PRINTF_FORMAT;
#define LogF(loglevel,format,...) LogFunction((loglevel),__FUNCTION__,(format),##__VA_ARGS__)
  static void MemDump(const char *pData, int length);
  static bool Init(const char* path, const char* name);
  static void SetLogLevel(int level);
  static int  GetLogLevel();
  static void SetExtraLogLevels(int level);
  static bool IsLogLevelLogged(int loglevel);

#ifdef WIN32
  static std::string GBKToUTF8(const char* strGBK);
  static std::string UTF8ToGBK(const char* strUTF8);
#endif  //WIN32

protected:
  class CLogGlobals
  {
  public:
    CLogGlobals(void) : m_repeatCount(0), m_repeatLogLevel(-1), m_logLevel(LOG_LEVEL_DEBUG), m_extraLogLevels(0), m_lastThreadId(0) {}
    ~CLogGlobals() {}
    PlatformInterfaceForCLog m_platform;
    int         m_repeatCount;
    int         m_repeatLogLevel;
    std::string m_repeatLine;
    int         m_logLevel;
    int         m_extraLogLevels;
    unsigned long long m_lastThreadId;
    CLogCriticalSection   critSec;
  };
  class CLogGlobals m_globalInstance; // used as static global variable
  static void LogString(int logLevel, const std::string& logString);
  static bool WriteLogString(int logLevel, const std::string& logString);
  static ThreadIdentifier GetCurrentThreadId();
};     

#ifdef DISABLE_LOGGING
#define log_debug(format, ...) 
#define log_info(format, ...) 
#define log_notice(format, ...) 
#define log_warning(format, ...) 
#define log_error(format, ...) 
#define log_severe(format, ...) 
#define log_fatal(format, ...)
#else
#ifdef NDEBUG
#define log_debug(format, ...)   CLog::Log(LOGDEBUG,   "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_info(format, ...)    CLog::Log(LOGINFO,    "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_notice(format, ...)  CLog::Log(LOGNOTICE,  "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_warning(format, ...) CLog::Log(LOGWARNING, "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_error(format, ...)   CLog::Log(LOGERROR,   "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_severe(format, ...)  CLog::Log(LOGSEVERE,  "[%04d]" format, __LINE__, ##__VA_ARGS__)
#define log_fatal(format, ...)   CLog::Log(LOGFATAL,   "[%04d]" format, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(format, ...) \
    CLog::Log(LOGDEBUG, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_info(format, ...) \
    CLog::Log(LOGINFO, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_notice(format, ...) \
    CLog::Log(LOGNOTICE, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_warning(format, ...) \
    CLog::Log(LOGWARNING, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_error(format, ...) \
    CLog::Log(LOGERROR, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_severe(format, ...) \
    CLog::Log(LOGSEVERE, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#define log_fatal(format, ...) \
    CLog::Log(LOGFATAL, "[%s][%d]" format, __FILE__, __LINE__, ##__VA_ARGS__ )
#endif //NDEBUG
#endif //DISABLE_LOGGING
