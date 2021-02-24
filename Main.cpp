
#include <getopt.h>
#include <string>
#include "Server.h"

int main(int args, char *argv[]){
	int threadNum=4;
	int port=924;

	int opt;
	const char *str = "t:p";
	while((opt = getopt(args,argv,str)) != -1){
		switch(opt){
			case 't':
				threadNum = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			default:
				break;
		}
	}
	Server myHTTPServer(threadNum, port);
	myHTTPServer.start();
	return 0;
}
