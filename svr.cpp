#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "sync.h"


static const char *ip		= "127.0.0.1";
static int port					= 8000;
//static const char *path = "./test.db";
//static const char *path = "User ID = papillon; Password = Hello; Server = localhost; Initial Catalog = SAC";
static const char *path = "User ID = harper; Password = Hello; Server = localhost; Initial Catalog = SAC";

int main(int argc, char *argv[]) {

	while (1) {
		db_sync_svr(ip, port, path);
		sleep(5);
	}

	return 0;
}
