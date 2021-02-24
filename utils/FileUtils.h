
#ifndef __FILE_UTILS_HEAD
#define __FILE_UTILS_HEAD

#include <string>
#include "uncopyable.h"

class AppendFile:uncopyable{
// this class read and write directly with file.
// thread safe is guaranted by LogFile
public:
	explicit AppendFile(std::string filename);
	~AppendFile();
	void append(const char *logline,const size_t len);
	void flush();
private:
	size_t write(const char*logline,size_t len);
	FILE *fp_;
	char buffer_[64*1024];

};
#endif
