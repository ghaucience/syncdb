#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "sync.h"


static const char *ip		= "127.0.0.1";
static int port					= 8000;
//static const char *path = "./test.db";
static const char *path = "User ID = harper; Password = Hello; Server = localhost; Initial Catalog = SAC";


int main(int argc, char *argv[]) {

	int ret = 0;

	while (1) {
		log_info("=================sync [%s]==============\n", path);
		ret = db_sync_cli(ip, port, path);
		log_info("=================sync ret :%d==============\n", ret);
		sleep(5);
	}

	return 0;
}
