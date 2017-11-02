#pragma once
#ifndef __DBM_H__
#define __DBM_H__

#include "osa/osa.h"
#include "dbm_definitions.h"
#include "dbm_utils.h"
#include "dbm_entities.h"


#define DBM_IS_ENTITY_TYPE_VALID(type)      (((type) >= DBM_ENTITY_TYPE_FIRST) && ((type) <= DBM_ENTITY_TYPE_LAST))


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    typedef enum DBM_EntityType {
        DBM_ENTITY_TYPE_NONE                      = 0,
        DBM_ENTITY_TYPE_HOUSE                     = 1,
        DBM_ENTITY_TYPE_EXT_HOUSE                 = 2,
        DBM_ENTITY_TYPE_PERSON                    = 3,
        DBM_ENTITY_TYPE_FLOWING_PERSON            = 4,
        DBM_ENTITY_TYPE_DEVICE                    = 5,
        DBM_ENTITY_TYPE_CARD                      = 6,
        DBM_ENTITY_TYPE_SAMCARD                   = 7,
        DBM_ENTITY_TYPE_CARD_PERMISSION           = 8, 
        DBM_ENTITY_TYPE_CARD_OWNING               = 9,
        DBM_ENTITY_TYPE_USER_HOUSE                = 10,
        DBM_ENTITY_TYPE_ACCESS_RECORD             = 11,
        DBM_ENTITY_TYPE_DEVICE_ALARM              = 12,
        DBM_ENTITY_TYPE_DEVICE_STATUS             = 13,
        DBM_ENTITY_TYPE_COUNT                     = 14,
        DBM_ENTITY_TYPE_FIRST                     = DBM_ENTITY_TYPE_HOUSE, 
        DBM_ENTITY_TYPE_LAST                      = DBM_ENTITY_TYPE_DEVICE_STATUS, 
    } DBM_EntityType;
    

    typedef enum DBM_DataSync {
        DBM_DATA_SYNC_UNSYNCED = 0,
        DBM_DATA_SYNC_SYNCED,
    } DBM_DataSync;
      

    typedef int (*DBM_EntityFilter)(const DBM_EntityType entityType, const void *pEntity);
    typedef struct DBM_EntityOptions {
        DBM_EntityType                 entityType;           /* [in] which type of entity do you want to get */
        DBM_EntityFilter               filter;               /* [in] returns 0: do not use this entity; returns non-0: use this entity. Can be used to implement complex filters */
        Char                          *pConditions;          /* [in] the conditions after SQL `WHERE` keyword */
        Uint32                        *pCount;               /* [in][out] the caller fills the count of entities to get; the callee fills the actual count of entities got */
        Uint32                         offset;               /* [in] entities count offset (0 based), relative to the first got entity */
        void                          *pEntities;            /* [out] memory to store the returned entities; the memory shall be allocated by the caller */
        
        /*  
           Example:  Get at most 50 persons that are not synced:
           int ret;          
           DBM_EntityOptions options;           
           OSA_clear(&options);
           Uint32 count = 50;
           
           options.entityType = DBM_ENTITY_TYPE_PERSON;
           options.filter = NULL;
           options.pConditions = "sync == 1";
           options.pCount = &count;
           options.offset = 0;
           options.pBuffer = malloc(sizeof(person) * (*options.pCount));
           ret = DBM_getEntity(handle, &options);
         */
    } DBM_EntityOptions;
        

    /* Connection String is sth. like "User ID = papillon; Password = Hello; Server = localhost; Initial Catalog = SAC" */
    int DBM_init(const Char *pConnectionString, DBM_Handle *pHandle);

    int DBM_deinit(DBM_Handle handle);
        
    size_t DBM_getEntitySize(const DBM_EntityType entityType);

    int DBM_getEntitiesCount(DBM_Handle handle, DBM_EntityOptions *pOptions);
        
    int DBM_getEntities(DBM_Handle handle, DBM_EntityOptions *pOptions);

    /* [DEPRECATED] please use `DBM_getEntities()` instead */
    int DBM_getUnsyncedEntity(DBM_Handle handle, const DBM_EntityType entityType, void *pEntity, void *pAssociation);

    int DBM_updateEntities(DBM_Handle handle, const Char *pFieldValues, DBM_EntityOptions *pOptions);

    /*
      @brief: all user-provided fields are assumed to be complete.
      @param pEntity: the entity to be inserted. After insertion, it is modified to reflect the real data in database.
    */
    int DBM_insertEntity(DBM_Handle handle, const DBM_EntityType entityType, void *pEntity, void *pAssociation);

    /* some fields may be incomplete */
    int DBM_insertEntityFromVendor(DBM_Handle handle, const DBM_EntityType entityType, void *pEntity);
    
    int DBM_printEntity(const DBM_EntityType entityType, const void *pEntity);

#ifdef __cplusplus
}
#endif // __cplusplus



#endif  /* __DBM_H__ */
