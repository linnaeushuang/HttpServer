#include "HttpData.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "Event.h"
#include "EventLoop.h"
#include "utils/SocketUtils.h"
#include "utils/Logging.h"


const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

const long DEFAULT_EXPIRED_TIME = 2000000;             // microsecond
const long DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000000; // microsecond, five minutes


HttpData::HttpData(EventLoop *loop, int connfd)
	: loop_(loop),
	// Event::event_ is connfd,
	// also HttpData::fd_ is connfd.
	event_(new Event(loop, connfd)),
	fd_(connfd),
	error_(false),
	connectionState_(H_CONNECTED),
	method_(METHOD_GET),
	HTTPVersion_(HTTP_11),
	nowReadPos_(0),
	state_(STATE_PARSE_URI),
	hState_(H_START),
	keepAlive_(false) {
		event_->setReadHandler(bind(&HttpData::handleRead, this));
		event_->setWriteHandler(bind(&HttpData::handleWrite, this));
		event_->setConnHandler(bind(&HttpData::handleConn, this));
	} // end HttpData::HttpData

void HttpData::reset() {
	fileName_.clear();
	path_.clear();
	nowReadPos_ = 0;
	state_ = STATE_PARSE_URI;
	hState_ = H_START;
	headers_.clear();
	if (timer_.lock()) {
		std::shared_ptr<TimerNode> my_timer(timer_.lock());
		my_timer->clearReq();
		timer_.reset();
	}
}

void HttpData::seperateTimer() {
	// lock(): convert weak_ptr to shared_ptr
	// because shared_ptr overload operator bool()
	if (timer_.lock()) {
		std::shared_ptr<TimerNode> my_timer(timer_.lock());
		my_timer->clearReq();
		timer_.reset();
	}
}

void HttpData::handleRead() {
	// get connection fd
	//
	// if connection fd has read event,
	// something was send from TCP/IP,
	// call this function
	__uint32_t &events_ = event_->getEvents();
	do {
		bool zero = false;
		int read_num = readn(fd_, inBuffer_, zero);
		LOG << "Request: " << inBuffer_;
		if (connectionState_ == H_DISCONNECTING) {
			inBuffer_.clear();
			break;
		}
		if (read_num < 0) {
			perror("1");
			error_ = true;
			handleError(fd_, 400, "Bad Request");
			break;
		}
		else if (zero) {
			/*
			 * if there is a request, but can't read data,
			 * may be request aborted,
			 * or data in trans.
			 * most likely, opposite end close the connection.
			 * i see all case as oppositen end close the connection.
			 */
			connectionState_ = H_DISCONNECTING;
			if (read_num == 0) {
				break;
			}
		}

		// read data from socket
		// depend on current state_ to do one of follow:
		// 1.parse uri
		// 2.parse request header
		// 3.analysis or recv body


		if (state_ == STATE_PARSE_URI) {
			URIState flag = this->parseURI();
			if (flag == PARSE_URI_AGAIN) break;
			else if (flag == PARSE_URI_ERROR) {
				perror("2");
				LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
				inBuffer_.clear();
				error_ = true;
				handleError(fd_, 400, "Bad Request");
				break;
			} else
				// trans state_ to next state.
				state_ = STATE_PARSE_HEADERS;
		}
		// next state
		if (state_ == STATE_PARSE_HEADERS) {
			HeaderState flag = this->parseHeaders();
			if (flag == PARSE_HEADER_AGAIN)
				break;
			else if (flag == PARSE_HEADER_ERROR) {
				perror("3");
				error_ = true;
				handleError(fd_, 400, "Bad Request");
				break;
			}
			if (method_ == METHOD_POST) {
				// trans state_ to next state
				state_ = STATE_RECV_BODY;
			} else {
				// trans state_ to next state.
				state_ = STATE_ANALYSIS;
			}
		}
    
		// next state
		if (state_ == STATE_RECV_BODY) {
			int content_length = -1;
			if (headers_.find("Content-length") != headers_.end()) {
				content_length = stoi(headers_["Content-length"]);
			} else {
				error_ = true;
				handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
				break;
			}
			if (static_cast<int>(inBuffer_.size()) < content_length) break;
			// trans state_ to next state.
			state_ = STATE_ANALYSIS;
		}
		// next state
		if (state_ == STATE_ANALYSIS) {
			AnalysisState flag = this->analysisRequest();
			if (flag == ANALYSIS_SUCCESS) {
				// finish
				state_ = STATE_FINISH;
				break;
			} else {
				error_ = true;
				break;
			}
		}
	} while (false);
	if (!error_) {
		if (outBuffer_.size() > 0) {
			handleWrite();
		}
		if (!error_ && state_ == STATE_FINISH) {
			this->reset();
			if (inBuffer_.size() > 0) {
				if (connectionState_ != H_DISCONNECTING) handleRead();
			}

		} else if (!error_ && connectionState_ != H_DISCONNECTED)
			events_ |= EPOLLIN;
	}
}

void HttpData::handleWrite() {
	// call by handleRead.
	// after parse request, respond this request
	if (!error_ && connectionState_ != H_DISCONNECTED) {
		__uint32_t &events_ = event_->getEvents();
		if (writen(fd_, outBuffer_) < 0) {
			perror("writen");
			events_ = 0;
			error_ = true;
		}
		if (outBuffer_.size() > 0){
			events_ |= EPOLLOUT;
		}
	}
}

void HttpData::handleConn() {
	// after hanlde read (see Event::handleEvents)
	// do some follow-up work
	seperateTimer();
	__uint32_t &events_ = event_->getEvents();
	if (!error_ && connectionState_ == H_CONNECTED) {
		if (events_ != 0) {
			// some error! reset
			int timeout = DEFAULT_EXPIRED_TIME;
			if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;
			if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
				events_ = __uint32_t(0);
				events_ |= EPOLLOUT;
			}
			events_ |= EPOLLET;
			loop_->updateEpoller(event_, timeout);
		} else if (keepAlive_) {
			events_ |= (EPOLLIN | EPOLLET);
			long timeout = DEFAULT_KEEP_ALIVE_TIME;
			loop_->updateEpoller(event_, timeout);
    
		} else {
			// non keep-alive
			// shutdown: half close
			loop_->shutdown(event_);
			loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
		}
	} else if (!error_ && connectionState_ == H_DISCONNECTING &&
			(events_ & EPOLLOUT)) {
		events_ = (EPOLLOUT | EPOLLET);
	} else {
		loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
  }
}

URIState HttpData::parseURI() {
	// parse URI from inBuffer_
	// HTTP header first line
	// :method URL version
	std::string &str = inBuffer_;
	std::string cop = str;
	// from nowReadPos_ index (default 0), find '\r' 
	size_t pos = str.find('\r', nowReadPos_);
	if (pos < 0) {
		return PARSE_URI_AGAIN;
	}
	// get request first line, URI
	std::string request_line = str.substr(0, pos);
	if (str.size() > pos + 1)
		str = str.substr(pos + 1);
	else
		str.clear();
  
	// parse method
	int posGet = request_line.find("GET");
	int posPost = request_line.find("POST");
	int posHead = request_line.find("HEAD");
  
	if (posGet >= 0) {
		pos = posGet;
		method_ = METHOD_GET;
	} else if (posPost >= 0) {
		pos = posPost;
		method_ = METHOD_POST;
	} else if (posHead >= 0) {
		pos = posHead;
		method_ = METHOD_HEAD;
	} else {
		return PARSE_URI_ERROR;
	}

	// from method, find "/"
	// parse URL
	// and get request filename
	pos = request_line.find("/", pos);
	if (pos < 0) {
		fileName_ = "index.html";
		HTTPVersion_ = HTTP_11;
		return PARSE_URI_SUCCESS;
	} else {
		size_t _pos = request_line.find(' ', pos);
		if (_pos < 0)
			return PARSE_URI_ERROR;
		else {
			if (_pos - pos > 1) {
				fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
				size_t __pos = fileName_.find('?');
				if (__pos >= 0) {
					fileName_ = fileName_.substr(0, __pos);
				}
			}

			else
				fileName_ = "index.html";
		}
		pos = _pos;
	}

	// from URL find HTTP version(e.g.,HTTP/1.1).
	pos = request_line.find("/", pos);
	if (pos < 0) return PARSE_URI_ERROR;
	else {
		if (request_line.size() - pos <= 3)
			return PARSE_URI_ERROR;
		else {
			std::string ver = request_line.substr(pos + 1, 3);
			if (ver == "1.0")
				HTTPVersion_ = HTTP_10;
			else if (ver == "1.1")
				HTTPVersion_ = HTTP_11;
			else
				return PARSE_URI_ERROR;
		}
	}
	return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
	/*
	 * parse HTTP Header
	 *
	 * for the sake of coding,
	 * we do nothing(set some value or something)
	 * after parse field successfully.
	 * but still simulate this process.
	 *
	 */
	
	
	std::string &str = inBuffer_;
	int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
	int now_read_line_begin = 0;
	bool notFinish = true;
	size_t i = 0;

	// parse diff field
	for (; i < str.size() && notFinish; ++i) {
		switch (hState_) {
			case H_START: {
							  if (str[i] == '\n' || str[i] == '\r') break;
							  // parse next field
							  hState_ = H_KEY;
							  key_start = i;
							  now_read_line_begin = i;
							  break;
						  }
			case H_KEY: {
							if (str[i] == ':') {
								key_end = i;
								if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
								hState_ = H_COLON;
							} else if (str[i] == '\n' || str[i] == '\r')
								return PARSE_HEADER_ERROR;
							break;
      
						}
			case H_COLON: {
							  if (str[i] == ' ') {
								  hState_ = H_SPACES_AFTER_COLON;
							  } else
								  return PARSE_HEADER_ERROR;
							  break;
						  }
			case H_SPACES_AFTER_COLON: {
										   hState_ = H_VALUE;
										   value_start = i;
										   break;
      
									   }
			case H_VALUE: {
							  if (str[i] == '\r') {
								  hState_ = H_CR;
								  value_end = i;
								  if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
							  } else if (i - value_start > 512)
								  return PARSE_HEADER_ERROR;
							  break;
						  }
			case H_CR: {
						   if (str[i] == '\n') {
							   hState_ = H_LF;
							   std::string key(str.begin() + key_start, str.begin() + key_end);
							   std::string value(str.begin() + value_start, str.begin() + value_end);
							   headers_[key] = value;
							   now_read_line_begin = i;
						   } else
							   return PARSE_HEADER_ERROR;
						   break;
					   }
			case H_LF: {
						   if (str[i] == '\r') {
         
							   hState_ = H_END_CR;
						   } else {
							   key_start = i;
          
							   hState_ = H_KEY;
						   }
						   break;
					   }
			case H_END_CR: {
							   if (str[i] == '\n') {
								   hState_ = H_END_LF;
							   } else
								   return PARSE_HEADER_ERROR;
							   break;
						   }
      
			case H_END_LF: {
							   notFinish = false;
							   key_start = i;
							   now_read_line_begin = i;
							   break;
						   }
		}
	}
	if (hState_ == H_END_LF) {
		str = str.substr(i);
		return PARSE_HEADER_SUCCESS;
	}
	str = str.substr(now_read_line_begin);
	return PARSE_HEADER_AGAIN;
}

AnalysisState HttpData::analysisRequest() {
	if (method_ == METHOD_POST) {
		// this server does not allow client to POST data,
		// do nothing on this request
	} else if (method_ == METHOD_GET || method_ == METHOD_HEAD){
		std::string response_header;
		response_header += "HTTP/1.1 200 OK\r\n";
		if (headers_.find("Connection") != headers_.end() &&
				(headers_["Connection"] == "Keep-Alive" ||
				 headers_["Connection"] == "keep-alive")) {
			keepAlive_ = true;
			response_header += std::string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
		}
		// parse URL
		// request file type
		int dot_pos = fileName_.find('.');
		std::string filetype;
		if (dot_pos < 0)
			filetype = MimeType::getInstance().getMime("default");
		else
			// get request file type from '.' (dot_pos)
			filetype = MimeType::getInstance().getMime(fileName_.substr(dot_pos));

    	// echo test
		if (fileName_ == "hello") {
			outBuffer_ =
				"HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
			return ANALYSIS_SUCCESS;
		}
		struct stat sbuf;
		//get fileName's state in filesys
		//return 0 if successfully
		//
		//
		
		if (stat((MimeType::getInstance().getPath() + fileName_).c_str(), &sbuf) < 0) {
			// if error(many reason), return 404
			response_header.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
		response_header += "Content-Type: " + filetype + "\r\n";
		response_header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
    
		response_header += "Server: LinHuang's Web Server\r\n";

    
		// header end
    
		response_header += "\r\n";
		outBuffer_ += response_header;
		if (method_ == METHOD_HEAD) return ANALYSIS_SUCCESS;

		// if method_ == METHOD_GET, continue
		//
		// write the request data in outBuffer
		int src_fd = open((MimeType::getInstance().getPath() + fileName_).c_str(), O_RDONLY, 0);
    
		if (src_fd < 0) {
			// if file not exist, return 404
			outBuffer_.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
		void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		close(src_fd);
		if (mmapRet == (void *)-1) {
			munmap(mmapRet, sbuf.st_size);
			outBuffer_.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
		char *src_addr = static_cast<char *>(mmapRet);
		outBuffer_ += std::string(src_addr, src_addr + sbuf.st_size);
		munmap(mmapRet, sbuf.st_size);
		return ANALYSIS_SUCCESS;
	}
	return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, std::string short_msg) {
	short_msg = " " + short_msg;
	char send_buff[4096];
	std::string body_buff, header_buff;
	body_buff += "<html><title>emmm ~ something wrong</title>";
	body_buff += "<body bgcolor=\"ffffff\">";
	body_buff += std::to_string(err_num) + short_msg;
	body_buff += "<hr><em> LinHuang's Web Server</em>\n</body></html>";
	
	header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
	header_buff += "Content-Type: text/html\r\n";
	header_buff += "Connection: Close\r\n";
	header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
	header_buff += "Server: LinHuang's Web Server\r\n";
	header_buff += "\r\n";
	// it is not possible for "error" to write overflow.
	sprintf(send_buff, "%s", header_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
	sprintf(send_buff, "%s", body_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
}

void HttpData::handleClose() {
	connectionState_ = H_DISCONNECTED;
	std::shared_ptr<HttpData> guard(shared_from_this());
	loop_->removeFromEpoller(event_);
}

void HttpData::newEvent() {
	event_->setEvents(DEFAULT_EVENT);
	loop_->addToEpoller(event_, DEFAULT_EXPIRED_TIME);
}
