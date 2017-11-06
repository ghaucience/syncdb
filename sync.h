#ifndef __SYNC_H_
#define __SYNC_H_

#include "dbm.h"
#include "syslog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stCond {
	const char	*name;
	int		offset;
	int		type;
}stCond_t;


typedef struct stTableSts {
	const char		 *name;					/* table name */
	int				size;					/* record size */
	int				type;					/* EntityType */
	int				syncoff;
	int				condcnt;
	stCond_t	conds[5];
}stTableSts_t;


#define member_offset(type, field) ((size_t)&(((type *)0)->field))

int db_sync_cli(const char *ip, int port, const char *dbpath);
int db_sync_svr(const char *ip, int port, const char *dbpath);




#if 0
#define log_info(fmt,...)		syslog(LOG_INFO | LOG_CONS, fmt, __VA_ARGS__)
#define log_warn(fmt,...)		syslog(LOG_WARNING | LOG_CONS, fmt,__VA_ARGS__ )
#define log_err(fmt, ...)		syslog(LOG_ERR | LOG_CONS, fmt,__VA_ARGS__ )
#define log_debug(fmt,...)	syslog(LOG_DEBUG | LOG_CONS, fmt,__VA_ARGS__)
#else
#define log_info(fmt,...)		printf("[INFO] [%s] [%d]:" fmt, __func__, __LINE__, __VA_ARGS__)

#define log_warn(fmt,...)		printf("[WARN] [%s] [%d]:" fmt, __func__, __LINE__, __VA_ARGS__)

#define log_err(fmt, ...)		printf("[ERRR] [%s] [%d]:" fmt, __func__, __LINE__, __VA_ARGS__)

#define log_debug(fmt,...)	printf("[DBUG] [%s] [%d]:" fmt, __func__, __LINE__, __VA_ARGS__)
//#define log_debug(fmt,...)

#endif


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



#ifdef __cplusplus
}
#endif



#endif
