#include "jansson.h"
#include "sync.h"
#include "sync_tcp.h"
#include "dbm.h"
#include "base64.h"

#include "json_parser.h"


typedef struct stSyncEnv {
	int		fd;
	char	ip[16];
	int		port;
	char	dbaddr[128];
	DBM_Handle	handle;
}stSyncEnv_t;

static int seq = 0;

static int db_sync_get_unsync(stSyncEnv_t *se, stTableSts_t *ts, int count, void **data);
static json_t *db_sync_base64_code(stTableSts_t *ts, void *data, int len);
static int db_sync_send(stSyncEnv_t *se, const char *ip, int port, const char *str);
static int db_sync_wait_resp(stSyncEnv_t *se, const char *ip, int port);
static int db_sync_clr_unsync(stSyncEnv_t *se, stTableSts_t *ts, int count, void *data);

static int db_sync_recv(int fd, char *buf, int len, int s, int m);
static int db_sync_base64_decode(json_t *jdata, stTableSts_t **ts, void **data, int *len, int *seq);
static int db_sync_clr_or_add(stTableSts_t *ts, void *data, int len);
static int db_sync_resp(int fd, int ret, int seq);

static stTableSts_t tss[] = {
	{"House",						sizeof(DBM_House),					1,	1, {
			{"uuid", 0},
		}
	}, 

	{"ExtHouse",				sizeof(DBM_ExtHouse),				2,	1, {
			{"reluuid", 0},
		}
	},

	{"Person",					sizeof(DBM_Person),					3,	1,  {
			{"uuid", 0},	
		}
	},

	{"FlowingPerson",		sizeof(DBM_FlowingPerson),	4,	1, {
			{"reluuid", 0},
		}
	},

	{"Device",					sizeof(DBM_Device),					5,	1, {
			{"uuid",	0},
		}		
	},

	{"Card",						sizeof(DBM_Card),						6,	1,{ 
			{"uuid",	0},
		}
	},

	{"SAMCard",					sizeof(DBM_SAMCard),				7,	2, {
			{"type_",			0},
			{"serial_id", member_offset(DBM_SAMCard, serial_id)},
		}
	},

	{"CardPermission",	sizeof(DBM_CardPermission),	8,	2, {
			{"crk_uuid", 0},
			{"dev_uuid", member_offset(DBM_CardPermission, dev_uuid)},
		}
	},

	{"CardOwning",			sizeof(DBM_CardOwning),			9,	2, {
			{"person_uuid", 0},
			{"crk_uuid", member_offset(DBM_CardOwning, crk_uuid)},
		}
	},

	{"UserHouse",				sizeof(DBM_UserHouse),			10,	2, {
			{"userid", 0},
			{"houseid", member_offset(DBM_UserHouse, houseid)},
		}
	},

	{"AccessRecrod",		sizeof(DBM_AccessRecord),		11,	4, {
			{"cardno", 0},
			{"person_uuid", member_offset(DBM_AccessRecord, person_uuid)},
			{"mac",					member_offset(DBM_AccessRecord, person_uuid)},
			{"dev_date",		member_offset(DBM_AccessRecord, dev_date)},
		}
	},

	{"DeviceAlarm",			sizeof(DBM_DeviceAlarm),		12, 4, {
			{"uuid", 0},
			{"mac",					member_offset(DBM_DeviceAlarm, mac)},
			{"cardno",			member_offset(DBM_DeviceAlarm, cardno)},
			{"cdate",				member_offset(DBM_DeviceAlarm, cdate)}
		}
	},

	{"DeviceStatus",		sizeof(DBM_DeviceStatus),		13,	2, {
			{"dev_uuid", 0},
			{"cdate",		member_offset(DBM_DeviceStatus, cdate)}
		}
	},
};

static int init_flag = 0;
static stSyncEnv_t se;

int db_sync_svr(const char *ip, int port, const char *dbaddr) {
#if 1
	unsigned int	i				= 0;
	int ret = -1;

	if (ip == NULL || port < 6000 || dbaddr == NULL) {
		return -1;
	}

	strcpy(se.ip, ip);
	strcpy(se.dbaddr,dbaddr);
	se.port = port;
	se.handle = NULL;
	se.fd = -1;



	pthread_t tid;
	pthread_create(&tid, NULL, db_sync_listen_thread, se);
	init_flag = 1;

	return 0;
}


int db_sync_listen(char *ip, int port) {
	if (ret <= 0) {
		return -1;
	}
	sfd = ret;

	return 0;
}

void *db_sync_listen_thread(void *arg) {
	while (1) {
		if (se.fd < 0) {
			se.fd = sync_tcp_create(TCP_SERVER, se.ip, se.port);
			if (se.fd <= 0) {
				sleep(2);
				continue;
			}
		}

		int ret = sync_tcp_accept(se.fd, 4, 80);
		if (ret < 0) {
			sync_tcp_destroy(se.fd);
			se.fd = -1;
			continue;
		}

		pthread_create(&tid, NULL, db_sync_recv_thread, ret);
	}

	return (void *)0;
}

void *db_sync_recv_thread(void *arg) {
	char buf[4096];
	int fd = (int)arg;

	while (1) {
		int ret = do_sync_recv(fd, buf, sizeof(buf), 4, 80);
		if (ret < 0) {
			sync_tcp_destroy(fd);
			return 0;
		}
		buf[ret]  = 0;

		json_error_t error;
		json_t *jdata = json_loads(buf, 0, &error);
		if (jdata == NULL) {
			//do_sync_resp(-1, 0);
			continue;
		}

		stTableSts_t *ts = NULL;
		void *data = NULL;
		int len = 0;
		int seq = 0;
		int ret = db_sync_base64_decode(jdata,  &ts, &data, &len, &seq);
		if (ret != 0) {
			//do_sync_resp(-1, 0);
			continue;
		}

		int ret = db_sync_clr_or_add(&ts, data, len);

		
		ret = ret == 0 ? 0 : -1;

		ret = do_sync_resp(fd, ret, seq);
		if (ret != 0) {
			sync_tcp_destroy(fd);
			return 0;
		}
	}
}
	
static int db_sync_recv(int fd, char *buf, int len, int s, int m) {
}
static stTableSts_t *db_sync_ts_search(const char *name) {
	int i = 0;
	for (i = 0; i < sizeof(tss)/sizeof(tss[0]); i++) {
		if (strcmp(tss[i].name, tblname) == 0) {
			return &tss[i];
		}
	}
	return NULL;
}
static int db_sync_base64_decode(json_t *jdata, stTableSts_t **ts, void **data, int *len, int *seq) {
	const char *tblname = json_get_string(jdata, "tblname");
	const char *records = json_get_string(jdata, "records");
	const char *cmd			= json_get_string(jdata, "cmd");
	int iseq = -1;					json_get_string(jdata, "seq", iseq);

	if (tblname == NULL || records == NULL || cmd == NULL || strcmp(cmd, "sync") != 0 || iseq < 0) {
		return -1;
	}

	*req = ireq;

	*ts = db_sync_ts_search(tblname);
	if(*ts == NULL) {
		return -2;
	}

	int blen = Base64decode_len(records);	
	*data = malloc(blen+1);
	if (*data == NULL) {
		return -3;
	}
	*len = blen;
	Base64decode(*data, records);

	return 0;
}

static int db_sync_clr_or_add(stTableSts_t *ts, void *data, int len) {
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
			len += sprintf(where + len, "%s%s = '%s'", ax, cond->name, pi + cond->offset);
		}
		options.pConditions			= where;

		options.pCount					= 0;
		options.offset					= 0;
		options.pEntities				= NULL;

		ret = DBM_updateEntities(se->handle, "sync = 1", &options);
		if (ret != OSA_STATUS_OK) {
			break;
		}
	}

	return i;

}

static int db_sync_resp(int fd, int ret, int seq) {
	json_t *jret = json_object();
	if (jret == NULL) {
		return -1;
	}

	json_object_set_new(jret, "ret", json_integer(ret));
	json_object_set_new(jret, "seq", json_integer(seq));

	char *sret = json_dumps(jret, 0);
	if (sret =NULL) {
		json_decref(sret);
		return -2;
	}

	sync_tcp_send(fd, sret, strlen(sret));

	free(sret);
		
	return 0;
}

