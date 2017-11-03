#include "jansson.h"
#include "sync.h"
#include "sync_tcp.h"
#include "dbm.h"
#include "base64.h"

#include "json_parser.h"

#include <pthread.h>


typedef struct stSyncEnv {
	int		fd;
	char	ip[16];
	int		port;
	char	dbaddr[128];
	DBM_Handle	handle;
}stSyncEnv_t;

static int db_sync_recv(int fd, char *buf, int len, int s, int m);
static int db_sync_base64_decode(json_t *jdata, stTableSts_t **ts, void **data, int *len, int *seq);
static int db_sync_clr_or_add(void *handle, stTableSts_t *ts, void *data, int len);
static int db_sync_resp(int fd, int ret, int seq);

void *db_sync_listen_thread(void *arg);
void *db_sync_recv_thread(void *arg);

static stTableSts_t tss[] = {
	{"House",						sizeof(DBM_House),					1,	member_offset(DBM_House, sync), 1, {
			{"uuid", 0, 's'},
		}
	}, 

	{"ExtHouse",				sizeof(DBM_ExtHouse),				2,	member_offset(DBM_ExtHouse, sync),	1, {
			{"reluuid", 0, 's'},
		}
	},

	{"Person",					sizeof(DBM_Person),					3,	member_offset(DBM_Person, sync),	1,  {
			{"uuid", 0, 's'},	
		}
	},

	{"FlowingPerson",		sizeof(DBM_FlowingPerson),	4,	member_offset(DBM_FlowingPerson, sync),	1, {
			{"reluuid", 0, 's'},
		}
	},

	{"Device",					sizeof(DBM_Device),					5,	member_offset(DBM_Device, sync),	1, {
			{"uuid",	0, 's'},
		}		
	},

	{"Card",						sizeof(DBM_Card),						6,	member_offset(DBM_Card, sync),	1,{ 
			{"uuid",	0, 's'},
		}
	},

	{"SAMCard",					sizeof(DBM_SAMCard),				7,	member_offset(DBM_SAMCard, sync),	2, {
			{"type_",			0, 'c'},
			{"serial_id", member_offset(DBM_SAMCard, serial_id), 's'},
		}
	},

	{"CardPermission",	sizeof(DBM_CardPermission),	8,	member_offset(DBM_CardPermission, sync),	2, {
			{"crk_uuid", 0, 's'},
			{"dev_uuid", member_offset(DBM_CardPermission, dev_uuid), 's'},
		}
	},

	{"CardOwning",			sizeof(DBM_CardOwning),			9,	member_offset(DBM_CardOwning, sync),	2, {
			{"person_uuid", 0, 's'},
			{"crk_uuid", member_offset(DBM_CardOwning, crk_uuid), 's'},
		}
	},

	{"UserHouse",				sizeof(DBM_UserHouse),			10,	member_offset(DBM_UserHouse, sync),	2, {
			{"userid", 0, 's'},
			{"houseid", member_offset(DBM_UserHouse, houseid), 's'},
		}
	},

	{"AccessRecrod",		sizeof(DBM_AccessRecord),		11,	member_offset(DBM_AccessRecord, sync),	4, {
			{"cardno", 0, 's'},
			{"person_uuid", member_offset(DBM_AccessRecord, person_uuid), 's'},
			{"mac",					member_offset(DBM_AccessRecord, mac), 's'},
			{"slide_date",	member_offset(DBM_AccessRecord, slide_date), 's'},
		}
	},

	{"DeviceAlarm",			sizeof(DBM_DeviceAlarm),		12,	member_offset(DBM_DeviceAlarm, sync), 4, {
			{"uuid", 0, 's'},
			{"mac",					member_offset(DBM_DeviceAlarm, mac), 's'},
			{"cardno",			member_offset(DBM_DeviceAlarm, cardno), 's'},
			{"cdate",				member_offset(DBM_DeviceAlarm, cdate), 's'}
		}
	},

	{"DeviceStatus",		sizeof(DBM_DeviceStatus),		13,	member_offset(DBM_DeviceStatus, sync),	2, {
			{"dev_uuid", 0, 's'},
			{"cdate",		member_offset(DBM_DeviceStatus, cdate), 's'}
		}
	},
};

static int init_flag = 0;
static stSyncEnv_t se;

int db_sync_svr(const char *ip, int port, const char *dbaddr) {
	if (ip == NULL || port < 6000 || dbaddr == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -1;
	}

	if (init_flag == 1) {	
		return 0;
	}

	strcpy(se.ip, ip);
	strcpy(se.dbaddr,dbaddr);
	se.port = port;
	se.handle = NULL;
	se.fd = -1;

	pthread_t tid;
	pthread_create(&tid, NULL, db_sync_listen_thread, &se);
	init_flag = 1;

	return 0;
}

void *db_sync_listen_thread(void *arg) {
	while (1) {
		if (se.fd < 0) {
			se.fd = sync_tcp_create(TCP_SERVER, se.ip, se.port);
			if (se.fd <= 0) {
				sleep(2);
				log_err("[%s] [%d]\n", __func__, __LINE__);
				continue;
			}
		}

		int ret = sync_tcp_accept(se.fd, 4, 80);
		if (ret < 0) {
			sync_tcp_destroy(se.fd);
			se.fd = -1;
			log_err("[%s] [%d]\n", __func__, __LINE__);
			continue;
		}
		if (ret == 0) {
			continue;
		}

		pthread_t tid;
		log_info("Client : %d\n", ret);
		pthread_create(&tid, NULL, db_sync_recv_thread, (void *)ret);
	}

	return (void *)0;
}

void *db_sync_recv_thread(void *arg) {
	int buflen = 1024*20;
	char *buf = (char*)malloc(buflen);
	int fd = *(int*)&arg;
	void *handle = NULL;

	log_info("Client : %d\n", fd);
	while (1) {
		int ret = db_sync_recv(fd, buf, buflen, 4, 80);
		if (ret < 0) {
			sync_tcp_destroy(fd);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			return 0;
		}
		buf[ret]  = 0;

		log_info("Req:\n%s\n", buf);
		json_error_t error;
		json_t *jdata = json_loads(buf, 0, &error);
		if (jdata == NULL) {
			//db_sync_resp(-1, 0);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			continue;
		}

		stTableSts_t *ts = NULL;
		void *data = NULL;
		int len = 0;
		int seq = 0;
		ret = db_sync_base64_decode(jdata,  &ts, &data, &len, &seq);
		if (ret != 0) {
			//db_sync_resp(-1, 0);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			continue;
		}

		if (handle == NULL) {
			ret = DBM_init(se.dbaddr, &handle);
			if (ret != OSA_STATUS_OK) {
				handle = NULL;
				log_err("[%s] [%d]\n", __func__, __LINE__);
				return 0;
			}
		}
	
		ret = db_sync_clr_or_add(handle, ts, data, len);

			
		log_info("Sync Table[%s] %d/%d\n", ts->name, len/ts->size, ret);
		
		ret = (ret == (len/ts->size)) ? 0 : -1;

		ret = db_sync_resp(fd, ret, seq);
		if (ret != 0) {
			sync_tcp_destroy(fd);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			if (handle != NULL) {
				DBM_deinit(handle);
			}
			return 0;
		}
	}
}
	
static stTableSts_t *db_sync_ts_search(const char *name) {
	unsigned int i = 0;
	for (i = 0; i < sizeof(tss)/sizeof(tss[0]); i++) {
		if (strcmp(tss[i].name, name) == 0) {
			return &tss[i];
		}
	}
	return NULL;
}

static int db_sync_base64_decode(json_t *jdata, stTableSts_t **ts, void **data, int *len, int *seq) {
	const char *tblname = json_get_string(jdata, "tblname");
	const char *records = json_get_string(jdata, "records");
	const char *cmd			= json_get_string(jdata, "cmd");
	int iseq = -1;				json_get_int(jdata, "seq", &iseq);

	if (tblname == NULL || records == NULL || cmd == NULL || strcmp(cmd, "sync") != 0 || iseq < 0) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -1;
	}

	//log_debug("records is %s\n", records);

	*seq = iseq;

	*ts = db_sync_ts_search(tblname);
	if(*ts == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -2;
	}

	int blen = Base64decode_len(records);	
	*data = malloc(blen+1);
	if (*data == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -3;
	}
	*len = blen;
	Base64decode((char *)*data, records);

	return 0;
}

static int db_sync_clr_or_add(void *handle, stTableSts_t *ts, void *data, int len) {
	int ret = -1;

	uint32_t cnt = len / ts->size;
	uint32_t i = 0;
	for (i = 0; i < cnt; i++) {
		char *pi = ((char *)data) + ts->size * i;

		DBM_EntityOptions options;
		options.entityType			= (DBM_EntityType)ts->type;
		options.filter					= 0;

		/* condtions */
		char where[128];
		int j = 0;
		int len = 0;
		for (j = 0; j < ts->condcnt; j++) {
			stCond_t *cond = &ts->conds[j];
			char *ax = (char *)"";

			if (j != 0) {
				ax = (char *)" and ";
			}
			if (cond->type == 's') {
				len += sprintf(where + len, "%s%s = '%s'", ax, cond->name, pi + cond->offset);
			} else if (cond->type == 'c') {
				len += sprintf(where + len, "%s%s = '%d'", ax, cond->name, *(pi + cond->offset));
			}
		}
		options.pConditions			= where;

		uint32_t exsit = 1;
		options.pCount					= &exsit;
		options.offset					= 0;
		options.pEntities				= NULL;

		log_info("Query %s where [%s]\n", ts->name, where);
		ret = DBM_getEntitiesCount(handle, &options);
		if (ret != OSA_STATUS_OK) {
			log_err("[%s] [%d]\n", __func__, __LINE__);
			break;
		}

		DBM_printEntity((DBM_EntityType)ts->type, pi);

		*(pi + ts->syncoff)			= 1;


		if (exsit == 0) {
			log_info("insert %s %s...\n", ts->name, where);
			ret = DBM_insertEntity(handle, (DBM_EntityType)ts->type, pi, NULL);
			//ret = DBM_insertEntityFromVendor(handle, (DBM_EntityType)ts->type, pi, NULL);
			if (ret != OSA_STATUS_OK) {
				log_err("[%s] [%d]\n", __func__, __LINE__);
				break;
			}
		} else {
			options.entityType			= (DBM_EntityType)ts->type;
			options.filter					= 0;
			options.pConditions			= where;
			uint32_t cnt = 1;
			options.pCount					= &cnt;
			options.offset					= 0;
			options.pEntities				= pi;
			log_info("overwrite %s where %s\n", ts->name, where);
			ret = DBM_overwriteEntities(handle, &options);
			if (ret != OSA_STATUS_OK) {
				log_err("[%s] [%d]\n", __func__, __LINE__);
				break;
			}
		}
	}



	return i;
}

static int db_sync_resp(int fd, int ret, int seq) {
	json_t *jret = json_object();
	if (jret == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -1;
	}

	json_object_set_new(jret, "ret", json_integer(ret));
	json_object_set_new(jret, "seq", json_integer(seq));

	char *sret = json_dumps(jret, 0);
	if (sret == NULL) {
		json_decref(jret);
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -2;
	}

	sync_tcp_send(fd, sret, strlen(sret), 0, 80);

	free(sret);
		
	return 0;
}

static int db_sync_recv(int fd, char *buf, int len, int s, int m) {
	//char buf[4096];
	int i = 0;
	int state = 0;
	int ret = 0;
	char ch = 0;

	while (1) {
		switch (state) {
			case 0:
				ret = sync_tcp_recv(fd, &ch, 1, s, m);
				if (ret <= 0) {
					log_err("[%s] [%d] fd:%d, ret:[%d]\n", __func__, __LINE__, fd, ret);
					return -1;
				}
				if (ret == 1 && ch == 0x02) {
					state = 1;
				}
				break;
			case 1:
				ret = sync_tcp_recv(fd, buf+i, len - i, s, m);
				if (ret <= 0) {
					log_err("[%s] [%d] fd:%d, ret:[%d]\n", __func__, __LINE__, fd, ret);
					return -2;
				}
				/*
				{
					int j = 0;
					for (j = 0; j < ret; j++) {
						printf("%c", *(buf+i + j));
					}
					printf("\n");
				}
				*/
				i += ret;
				if (buf[i-1] == 0x03) {
					log_info("%s i: %d, %02X(%02X)\n", "\n", i, buf[i-1], buf[i-1]);
					ret = i - 1;
					buf[ret] = 0;
					return ret;
				}
				break;
		}
	}
	buf[ret]  = 0;

	return ret;
}


