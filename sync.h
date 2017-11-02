#ifndef __SYNC_H_
#define __SYNC_H_

#include "dbm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stCond {
	char	*name;
	int		offset;
}stCond_t;


typedef struct stTableSts {
	char		 *name;					/* table name */
	int				size;					/* record size */
	int				type;					/* EntityType */
	int				condcnt;
	stCond_t	conds[5];
}stTableSts_t;


#define member_offset(type, field) ((size_t)&(((type *)0)->field))

int db_sync(const char *ip, int port, const char *dbpath);



#ifdef __cplusplus
}
#endif



#endif
