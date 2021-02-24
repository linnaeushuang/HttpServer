
#include "Server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>
#include "utils/SocketUtils.h"
#include "utils/Logging.h"


Server::Server(const int &threadNum, const int &port)
	: threadNum_(threadNum),
	port_(port),
	//main loop, main thread
	loop_(new EventLoop()),
	eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
	started_(false),
	// create a listen event, and hold main loop
	acceptEvent_(new Event(loop_)),
	listenFd_(socket_bind_listen(port_)){
		acceptEvent_->setFd(listenFd_);
		// ignord SIGPIPE
		handle_for_sigpipe();
		if(setSocketNonBlocking(listenFd_) < 0){
			perror("set socket non block failed");
			//exit
			abort();
		}
	}

void Server::start(){
	eventLoopThreadPool_->start();
	// use ET
	acceptEvent_->setEvents(EPOLLIN | EPOLLET);
	acceptEvent_->setReadHandler(std::bind(&Server::handNewConn, this));
	acceptEvent_->setConnHandler(std::bind(&Server::handThisConn,this));
	// register listen event to main loop
	loop_->addToEpoller(acceptEvent_, 0);
	// run loop
	loop_->loop();
	started_=true;
}

void Server::handNewConn(){
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	memset(&client_addr, 0, client_addr_len);
	int accept_fd = 0;
	while((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,&client_addr_len))>0){
		if(accept_fd > MAXFDS){
			close(accept_fd);
			LOG<<"New connection and current concurrency exceeds maximum concurrency.";
			continue;
		}
		// get a loop(or thread) from LoopThreadPool
		// one loop per thread
		EventLoop *loop = eventLoopThreadPool_->getNextWorkLoop();
		LOG<<"New connection from"<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port);
		if(setSocketNonBlocking(accept_fd) < 0){
			LOG<<"Set non block failed!";
			return;
		}

		// Nagle algorithm is ban.
		setSocketNodelay(accept_fd);
		// this server only for HTTP
		// create http event
		std::shared_ptr<HttpData> http_req(new HttpData(loop, accept_fd));
		http_req->getEvent()->setHttpHolder(http_req);
		// assign this HTTP to worker thread
		// and wakeup this worker thread using eventfd
		loop->queueInLoop(std::bind(&HttpData::newEvent, http_req));
	}
}

void Server::handThisConn(){ 
	// because ET,
	// nothing to do.
}
