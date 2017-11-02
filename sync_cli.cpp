#include "jansson.h"
#include "json_parser.h"
#include "sync.h"
#include "sync_tcp.h"
#include "dbm.h"
#include "base64.h"


static int fd = -1;
static int seq = 0;
static int db_sync_send(const char *ip, int port, const char *str);
static int db_sync_get_unsync(stTableSts_t *ts, int count, void **data);
static json_t *db_sync_base64_code(stTableSts_t *ts, void *data, int len);
static int db_sync_clr_unsync(stTableSts_t *ts, int count, void *data);
static int db_sync_wait_resp(const char *ip, int port);

static DBM_Handle handle = NULL;

static stTableSts_t tss[] = {
	/*
	{"House",						sizeof(DBM_House),					1,	1,
		{"uuid", 0},
	}, 

	{"ExtHouse",				sizeof(DBM_ExtHouse),				2,	1,
		{"reluuid", 0},
	},

	{"Person",					sizeof(DBM_Person),					3,	1, 
		{"uuid", 0},	
	},

	{"FlowingPerson",		sizeof(DBM_FlowingPerson),	4,	1,
		{"reluuid", 0},
	},

	{"Device",					sizeof(DBM_Device),					5,	1,
		{"uuid",	0},
	},

	{"Card",						sizeof(DBM_Card),						6,	1,
		{"uuid",	0},
	},

	{"SAMCard",					sizeof(DBM_SAMCard),				7,	1,
		{"type_",			0},
		{"serial_id", member_offset(DBM_SAMCard, serial_id)},
	},

	{"CardPermission",	sizeof(DBM_CardPermission),	8,	1,
		{"crk_uuid", 0},
		{"dev_uuid", member_offset(DBM_CardPermission, dev_uuid)}
	},

	{"CardOwning",			sizeof(DBM_CardOwning),			9,	1,
		{"person_uuid", 0},
		{"crk_uuid", member_offset(DBM_CardOwning, crk_uuid)}
	},

	{"UserHouse",				sizeof(DBM_UserHouse),			10,	1,
		{"userid", 0},
		{"houseid", member_offset(DBM_UserHouse, houseid)}
	},

	{"AccessRecrod",		sizeof(DBM_AccessRecord),		11,	1,
		{"cardno", 0},
		{"person_uuid", member_offset(DBM_AccessRecord, person_uuid)},
		{"mac",					member_offset(DBM_AccessRecord, person_uuid)},
		{"dev_date",		member_offset(DBM_AccessRecord, dev_date)},
	},

	{"DeviceAlarm",			sizeof(DBM_DeviceAlarm),		12, 1,
		{"uuid", 0},
		{"mac",					member_offset(DBM_DeviceAlarm, mac)},
		{"cardno",			member_offset(DBM_DeviceAlarm, cardno)},
		{"cdate",				member_offset(DBM_DeviceAlarm, cdate)}
	},

	{"DeviceStatus",		sizeof(DBM_DeviceStatus),		13,	1,
		{"dev_uuid", 0},
		{"cdate",		member_offset(DBM_DeviceStatus, cdate)}
	},
	*/
};

int db_sync(const char *ip, int port, const char *dbpath) {
#if 0
	unsigned int	i				= 0;
	int ret = -1;

	
	for (i = 0; i <= sizeof(tss)/sizeof(tss[0]);) {
		stTableSts_t *ts = &tss[i];
		int count = 10;
		void *data = NULL;


		ret = db_sync_get_unsync(ts, 10, &data);
		if (ret < 0) {
			return -1;
		} else if (ret == 0) {
			i++;
			continue;
		}
		count = ret;

		
		json_t * jsync = db_sync_base64_code(ts, data, ts->size*count);
		if (jsync == NULL) {
			free(data);
			return -3;
		}

		char *ssync = json_dumps(jsync, 0);
		if (ssync == NULL) {
			json_decref(jsync);
			free(data);
			return -3;
		}

		ret = db_sync_send(ip, port, ssync);
		free(ssync);
		json_decref(jsync);
		if (ret != 0) {
			free(data);
			return -4;
		}

		ret = db_sync_wait_resp(ip, port);
		if (ret != 0) {
			free(data);
			return -5;
		}

		ret = db_sync_clr_unsync(ts, count, data);
		free(data);
		if (ret != count) {	
			return -6;
		}
	}
#else
	return 0;
#endif
	return 0;
}

int db_sync_send(const char *ip, int port, const char *str) {
#if 0

	if (fd < 0) {
		fd = sync_tcp_create(TCP_CLIENT, ip, port);	
		if (fd < 0) {
			return -1;
		}
	}

	int ret = sync_tcp_send(fd, (char *)str, strlen(str), 0, 80);
	if (ret <= 0) {
		sync_tcp_destroy(fd);
		fd = -1;
		return -2;
	}

	return 0;
#else
	return 0;
#endif
}
static int db_sync_get_unsync(stTableSts_t *ts, int count, void **data) {
#if 0
	int ret = -1;
	if (handle == NULL) {
		ret = DBM_init(NULL, &handle);
		if (ret != OSA_STATUS_OK) {
			handle = NULL;
			return -1;
		}
	}

	
	uint32_t cnt = count;
	int size = sizeof(ts->size*cnt);
	DBM_EntityOptions options;
	options.entityType			= (DBM_EntityType)ts->type;
	options.filter					= 0;
	options.pConditions			= (char*)"sync = 0";
	options.pCount					= &cnt;
	options.offset					= 0;
	options.pEntities				= malloc(size);
	if (options.pEntities == NULL) {
		return -2;
	}

	ret = DBM_getEntitiesCount(handle, &options);
	if (ret != OSA_STATUS_OK) {
		free(options.pEntities);
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
#if 0

	int blen = Base64encode_len(len);
	char *buf = (char *)malloc(blen+1);
	if (buf == NULL) {
		return NULL;
	}
	json_t *jret = json_object();
	if (jret == NULL) {
		return NULL;
	}
	
	Base64encode(buf, (const char *)data, len);

	json_object_set_new(jret, "tblname", json_string(ts->name));
	json_object_set_new(jret, "records", json_string(buf));
	json_object_set_new(jret, "cmd",		 json_string("sync"));
	json_object_set_new(jret, "seq",		 json_integer(seq++));

	return jret;
#else
	return NULL;
#endif

}

static int db_sync_clr_unsync(stTableSts_t *ts, int count, void *data) {
#if 0
	int ret = -1;

	if (handle == NULL) {
		ret = DBM_init(NULL, &handle);
		if (ret != OSA_STATUS_OK) {
			handle = NULL;
			return -1;
		}
	}


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
			len += sprintf(where + len, "%s%s = '%s'", ax, cond->name, pi + cond->offset);
		}
		options.pConditions			= where;

		options.pCount					= 0;
		options.offset					= 0;
		options.pEntities				= NULL;

		ret = DBM_updateEntities(handle, "sync = 1", &options);
		if (ret != OSA_STATUS_OK) {
			break;
		}
	}

	return i;
#else 
	return 0;
#endif
}

#if 0
static int db_sync_wait_resp(const char *ip, int port) {
#if 0
	/*
	char buf[128];
	int ret = sync_tcp_recv(fd, buf, sizeof(buf), 4, 80);
	if (ret <= 0) {
		return -1;
	}
	buf[ret]  = 0;

  json_error_t error;
  json_t *jret = json_loads(buf, 0, &error);
  if (jret == NULL) {
		return -2;
  }

	int rseq = -1;  json_get_int(jret, "seq", &rseq);
	int rret = -1;	json_get_int(jret, "ret", &rret);
	if (rseq != seq || rret != 0) {
		json_decref(jret);
		return -3;
	}
	
	json_decref(jret);
	*/

	return 0;
#else 
	return 0;
#endif
}
#endif


