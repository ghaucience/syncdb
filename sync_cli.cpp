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
static int db_sync_req(stSyncEnv_t *se, const char *ip, int port, const char *str);
static int db_sync_wait_resp(stSyncEnv_t *se, const char *ip, int port);
static int db_sync_clr_unsync(stSyncEnv_t *se, stTableSts_t *ts, int count, void *data);


extern stTableSts_t tss[];
#if 0
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
#endif

int db_sync_cli(const char *ip, int port, const char *dbaddr) {
#if 1
	unsigned int	i				= 0;
	int ret = -1;

	if (ip == NULL || port < 6000 || dbaddr == NULL) {
		return -1;
	}

	stSyncEnv_t se;
	strcpy(se.ip, ip);
	strcpy(se.dbaddr,dbaddr);
	se.port = port;
	se.handle = NULL;
	se.fd = -1;

	/*
	for (i = 0; i < sizeof(tss)/sizeof(tss[0]); i++) {
		log_info("%d\n", tss[i].size);
	}
	*/
	
	for (i = 0; i < sizeof(tss)/sizeof(tss[0]);) {
		stTableSts_t *ts = &tss[i];
		void *data = NULL;

		log_info("sync table [%s] ...\n", ts->name);

		//ret = db_sync_get_unsync(&se, ts, 5, &data);
		ret = db_sync_get_unsync(&se, ts, 1, &data);
		if (ret < 0) {
			log_err("[%s] [%d]\n", __func__, __LINE__);
			ret = -2;
			break;
		} else if (ret == 0) {
			i++;
			continue;
		}
		int count = ret;

		log_info("sync %d (%d)records...\n", count, ts->size);
		DBM_printEntity((DBM_EntityType)ts->type, data);
		json_t * jsync = db_sync_base64_code(ts, data, ts->size*count);
		if (jsync == NULL) {
			free(data);
			ret = -3;
			break;
		}
		log_info("%s 1", "\n");

		char *ssync = json_dumps(jsync, 0);
		if (ssync == NULL) {
			json_decref(jsync);
			free(data);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			ret = -4;
			break;
		}
		log_info("%s 2", "\n");

		log_debug("req : \n%s\n", ssync);

		ret = db_sync_req(&se, se.ip, se.port, ssync);
		free(ssync);
		json_decref(jsync);
		if (ret != 0) {
			free(data);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			ret = -5;
			break;
		}

		ret = db_sync_wait_resp(&se, se.ip, se.port);
		if (ret != 0) {
			free(data);
			log_err("[%s] [%d]\n", __func__, __LINE__);
			ret = -6;
			break;
		}

		ret = db_sync_clr_unsync( &se, ts, count, data);
		free(data);
		if (ret != count) {	
			log_err("[%s] [%d]\n", __func__, __LINE__);
			ret = -7;
			break;
		}
	}

	if (se.fd > 0) {
		sync_tcp_destroy(se.fd);
	}
	if (se.handle != NULL) {
		DBM_deinit(se.handle);
	}
	
	return ret;
#else
	return 0;
#endif
}

int db_sync_req(stSyncEnv_t *se, const char *ip, int port, const char *str) {
#if 1

	if (se->fd < 0) {
		se->fd = sync_tcp_create(TCP_CLIENT, ip, port);	
		if (se->fd < 0) {
			log_err("[%s] [%d]\n", __func__, __LINE__);
			return -1;
		}
	}


	int ret = sync_tcp_send(se->fd, (char*)"\x02", 1, 0, 80);
	if (ret <= 0) {
		sync_tcp_destroy(se->fd);
		se->fd = -1;
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -2;
	}
	

	ret = sync_tcp_send(se->fd, (char *)str, strlen(str), 0, 80);
	if (ret <= 0) {
		sync_tcp_destroy(se->fd);
		se->fd = -1;
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -3;
	}

	ret = sync_tcp_send(se->fd, (char*)"\x03", 1, 0, 80);
	if (ret <= 0) {
		sync_tcp_destroy(se->fd);
		se->fd = -1;
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -4;
	}




	return 0;
#else
	return 0;
#endif
}

static int db_sync_get_unsync(stSyncEnv_t *se, stTableSts_t *ts, int count, void **data) {
#if 1
	int ret = -1;
	if (se->handle == NULL) {
		ret = DBM_init(se->dbaddr, &se->handle);
		if (ret != OSA_STATUS_OK) {
			se->handle = NULL;
			log_err("[%s] [%d]\n", __func__, __LINE__);
			return -1;
		}
	}

	
	uint32_t cnt = count;
	int size = ts->size*cnt;
	DBM_EntityOptions options;
	options.entityType			= (DBM_EntityType)ts->type;
	options.filter					= 0;
	options.pConditions			= (char*)"sync = 0";
	options.pCount					= &cnt;
	options.offset					= 0;
	options.pEntities				= malloc(size);
	if (options.pEntities == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -2;
	}

	//ret = DBM_getEntitiesCount(se->handle, &options);
	ret = DBM_getEntities(se->handle, &options);
	if (ret != OSA_STATUS_OK) {
		free(options.pEntities);
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -3;
	}

	if (cnt == 0) {
		return 0;
	}

	*data = options.pEntities;

	return cnt;
#else
	return 0;
#endif
}


static json_t *db_sync_base64_code(stTableSts_t *ts, void *data, int len) {
#if 1

	int blen = Base64encode_len(len);
	char *buf = (char *)malloc(blen+1);
	memset(buf, 1, blen+1);
	if (buf == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return NULL;
	}
	json_t *jret = json_object();
	if (jret == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return NULL;
	}
	

	Base64encode(buf, (const char *)data, len);
	buf[blen] = 0;

	json_object_set_new(jret, "tblname", json_string(ts->name));
	json_object_set_new(jret, "records", json_string(buf));
	json_object_set_new(jret, "cmd",		 json_string("sync"));
	json_object_set_new(jret, "seq",		 json_integer(++seq));

	return jret;
#else
	return NULL;
#endif

}

static int db_sync_clr_unsync(stSyncEnv_t *se, stTableSts_t *ts, int count, void *data) {
#if 1
	return count;
	int ret = -1;

	uint32_t cnt = count;
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

		options.pCount					= 0;
		options.offset					= 0;
		options.pEntities				= NULL;

		log_info("update %s where %s\n", ts->name, where);
		ret = DBM_updateEntities(se->handle, "sync = 1", &options);
		if (ret != OSA_STATUS_OK) {
			break;
		}
	}

	return i;
#else 
	return 0;
#endif
}

static int db_sync_wait_resp(stSyncEnv_t *se, const char *ip, int port) {
#if 1
	char buf[128];
	int ret = sync_tcp_recv(se->fd, buf, sizeof(buf), 4, 80);
	if (ret <= 0) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -1;
	}
	buf[ret]  = 0;
	log_debug("rsp:\n%s\n", buf);


  json_error_t error;
  json_t *jret = json_loads(buf, 0, &error);
  if (jret == NULL) {
		log_err("[%s] [%d]\n", __func__, __LINE__);
		return -2;
  }


	int rseq = -1;  json_get_int(jret, "seq", &rseq);
	int rret = -1;	json_get_int(jret, "ret", &rret);
	/*
	{
		const char *sret = json_dumps(jret, 0);
		if (sret != NULL) {
			printf("sret is %s\n", sret);
			free(sret);
		}
	}
	*/
	if (rseq != seq || rret != 0) {
		json_decref(jret);
		log_err("[%s] [%d], seq:%d(r:%d), ret:%d \n", __func__, __LINE__, seq, rseq, rret);
		return -3;
	}
	
	json_decref(jret);

	return 0;
#else 
	return 0;
#endif
}


