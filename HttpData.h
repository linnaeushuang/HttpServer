
#ifndef __HTTP_DATA_HEAD
#define __HTTP_DATA_HEAD

#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class TimerNode;
class Event;

enum ProcessState {
	STATE_PARSE_URI = 1,
	STATE_PARSE_HEADERS,
	STATE_RECV_BODY,
	STATE_ANALYSIS,
	STATE_FINISH
};

enum URIState {
	PARSE_URI_AGAIN = 1,
	PARSE_URI_ERROR,
	PARSE_URI_SUCCESS,
};

enum HeaderState {
	PARSE_HEADER_SUCCESS = 1,
	PARSE_HEADER_AGAIN,
	PARSE_HEADER_ERROR
};

enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };


// parse one attribution, like Accept, Referer, User-Agent
enum ParseState {
	H_START = 0,
	H_KEY,
	H_COLON,
	H_SPACES_AFTER_COLON,
	H_VALUE,
	H_CR,
	H_LF,
	H_END_CR,
	H_END_LF
};

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

enum HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum HttpVersion { HTTP_10 = 1, HTTP_11 };


// HTTP request file type
// for http header
class MimeType {
private:
	std::unordered_map<std::string, std::string> mime;
	std::string sourcePath="/myServerSource/";
	char currentPath[64];
	void init(){
		getcwd(currentPath,64);
		sourcePath = std::string(currentPath) + sourcePath;
		mime[".html"] = "text/html";
		mime[".avi"] = "video/x-msvideo";
		mime[".bmp"] = "image/bmp";
		mime[".c"] = "text/plain";
		mime[".doc"] = "application/msword";
		mime[".gif"] = "image/gif";
		mime[".gz"] = "application/x-gzip";
		mime[".htm"] = "text/html";
		mime[".ico"] = "image/x-icon";
		mime[".jpg"] = "image/jpeg";
		mime[".png"] = "image/png";
		mime[".txt"] = "text/plain";
		mime[".mp3"] = "audio/mp3";
		mime["default"] = "text/html";
	}
	MimeType(){ init(); }
	MimeType(const MimeType& other) = delete;
	MimeType& operator=(const MimeType&) = delete;
public:
	static MimeType& getInstance(){
		static MimeType value;
		return value;
	}
	std::string getMime(const std::string &suffix){
		if (mime.find(suffix) == mime.end()) return mime["default"];
		else return mime[suffix];
	}
	std::string getPath(){
		return sourcePath;
	}
	std::string getCurrentPath(){
		return currentPath;
	}
};

class HttpData : public std::enable_shared_from_this<HttpData> {
public:
	HttpData(EventLoop *loop, int connfd);
	~HttpData() { close(fd_); }
	void reset();
	void seperateTimer();
	void linkTimer(std::shared_ptr<TimerNode> mtimer) {
	  timer_ = mtimer;
	}
	std::shared_ptr<Event> getEvent() { return event_; }
	EventLoop *getLoop() { return loop_; }
	void handleClose();
	void newEvent();

private:
	EventLoop *loop_;
	// one http connection <--> one Event
	std::shared_ptr<Event> event_;
	// socket connection fd
	int fd_;
	std::string inBuffer_;
	std::string outBuffer_;
	bool error_;
	ConnectionState connectionState_;

	HttpMethod method_;
	HttpVersion HTTPVersion_;
	std::string fileName_;
	std::string path_;
	int nowReadPos_;
	ProcessState state_;
	ParseState hState_;
	bool keepAlive_;
	std::map<std::string, std::string> headers_;
	std::weak_ptr<TimerNode> timer_;

	void handleRead();
	void handleWrite();
	void handleConn();
	void handleError(int fd, int err_num, std::string short_msg);
	URIState parseURI(); //parse http uniform resourse identifier
	HeaderState parseHeaders(); // parse http header
	AnalysisState analysisRequest(); // parse http request
};
#endif
