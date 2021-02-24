#include "FileUtils.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename) : fp_(fopen(filename.c_str(), "ae")) {
	//set file stream's buffer
	setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char* logline, const size_t len) {
	//write logline into fp_
	size_t n = this->write(logline, len);
	size_t remain = len - n;
	while (remain > 0) {
		size_t x = this->write(logline + n, remain);
		if (x == 0) {
			int err = ferror(fp_);
			if (err) fprintf(stderr, "AppendFile::append() failed !\n");
			break;
		}
		n += x;
		remain = len - n;
	}
}

// from buffer to file
void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char* logline, size_t len) {
	// non thread safe
	return fwrite_unlocked(logline, 1, len, fp_);
}
