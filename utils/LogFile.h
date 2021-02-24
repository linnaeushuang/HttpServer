#ifndef __LOG_FILE_HEAD
#define __LOG_FILE_HEAD

#include <memory>
#include <string>
#include "FileUtils.h"
#include "MutexLock.h"
#include "uncopyable.h"


class LogFile : uncopyable {
//encapsulate FileUtils::AppendFile
//use mutex to thread safe
public:
	LogFile(const std::string& basename, int flushEveryN = 1024);
	~LogFile();

	void append(const char* logline, int len);
	void flush();

	void rollFile();
private:
    void append_unlocked(const char* logline, int len);
    
    const std::string basename_;
	// flush every flushEveryN_ times call append_unlocked()
    const int flushEveryN_;
    
    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;

	// for roll
	int curSize_;
	std::string getFilename();
	static const long KMaxFileSize = 512*1024*1024; // 0.5G
};
#endif
