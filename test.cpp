
#ifdef WIN32
#include <Windows.h>
#endif
#include "log.h"


#ifdef WIN32

static std::string GBKToUTF8(const char* strGBK)
{
	int len = MultiByteToWideChar(CP_ACP, 0, strGBK, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, strGBK, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	std::string strTemp = str;
	if (wstr) delete[] wstr;
	if (str) delete[] str;
	return strTemp;
}

static std::string UTF8ToGBK(const char* strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
#endif  //WINDOWS

int DoWork()
{
	log_info("%s", GBKToUTF8("写中文测试").c_str());
	log_info("%s", GBKToUTF8("123abcc写中文测试").c_str());
    log_debug("this is first line log");
    log_debug("this is first line log");
    log_notice("this is notice line log");
    log_info("this is info line log");
    log_warning("this is warning log msg");
    log_warning("this is warning log msg");
    log_error("this is error log msg");
    log_severe("this is severe log msg");
    log_fatal("fatal msg, app crash");

    char refdata[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    CLog::MemDump(refdata, sizeof(refdata));

    return 0;
}


int SingleThread()
{
    return DoWork();
}

void* CalcThread(void *pParam)
{
    DoWork();
    return 0;
}

int MultiThread( int threadCount)
{
#ifdef WIN32
    HANDLE* hThreads = new HANDLE[threadCount];
    for(int i = 0; i < threadCount; ++i)
    {
        printf("create VAD Test thread[%d]. \n", i);


        DWORD dwThreadId;
        hThreads[i] = CreateThread
            (
            0,                        //Choose default security
            0,                        //Default stack size
            (LPTHREAD_START_ROUTINE)&CalcThread,      //Routine to execute
            (LPVOID) NULL,                  //Thread parameter
            0,                        //Immediately run the thread
            &dwThreadId                   //Thread Id
            );

    }

    WaitForMultipleObjects(threadCount, hThreads, TRUE, INFINITE);
    delete [] hThreads;

#else

    pthread_t* thread_id = new pthread_t[threadCount];

    for(int i = 0; i < threadCount; ++i)
    {
        pthread_create(thread_id + i, NULL, &CalcThread, NULL);
    }

    for(int i = 0; i < threadCount; ++i)
    {
        pthread_join(thread_id[i], NULL);
    }
    delete [] thread_id;

#endif

    return 0;
}


//int main(int argc, char *argv[])
int VadCheck(int threadCount)
{

    printf("thread count = %d.\n", threadCount);
    if(threadCount > 1) 
    {
        MultiThread(threadCount);
    }
    else
        SingleThread();

    return 0;
}


int main(int argc, char* argv[])
{
    std::string path(argv[1]);
#ifndef DISABLE_LOGGING
    CLog::Init(path.c_str(), "TEST");
#ifdef NDEBUG
    CLog::SetLogLevel(LOG_LEVEL_NORMAL);
#else
    CLog::SetLogLevel(LOG_LEVEL_DEBUG);
#endif
#endif

    int cnt = atoi(argv[2]);
    VadCheck(cnt);

    CLog::Close();

    log_error("this is error");
    return 0;
}
