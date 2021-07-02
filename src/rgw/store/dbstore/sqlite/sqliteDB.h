// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <sqlite3.h>
#include "rgw/store/dbstore/common/dbstore.h"

using namespace std;

class SQLiteDB : public DBStore, public DBOp{
  private:
    sqlite3_mutex *mutex = NULL;

  protected:
    CephContext *cct;

  public:	
    sqlite3_stmt *stmt = NULL;
    DBOpPrepareParams PrepareParams;

    SQLiteDB(string db_name, CephContext *_cct) : DBStore(db_name, _cct) {
      cct = _cct;
      InitPrepareParams(PrepareParams);
    }
    SQLiteDB(sqlite3 *dbi, CephContext *_cct) : DBStore(_cct) {
      db = (void*)dbi;
      cct = _cct;
      InitPrepareParams(PrepareParams);
    }
    ~SQLiteDB() {}

    int exec(const char *schema,
        int (*callback)(void*,int,char**,char**));
    void *openDB();
    int closeDB();
    int Step(DBOpInfo &op, sqlite3_stmt *stmt,
        int (*cbk)(CephContext *cct, DBOpInfo &op, sqlite3_stmt *stmt));
    int Reset(sqlite3_stmt *stmt);
    int InitializeDBOps();
    int FreeDBOps();
    /* default value matches with sqliteDB style */
    int InitPrepareParams(DBOpPrepareParams &params) { return 0; }

    int createTables();
    int createBucketTable(DBOpParams *params);
    int createUserTable(DBOpParams *params);
    int createObjectTable(DBOpParams *params);
    int createObjectDataTable(DBOpParams *params);
    int createQuotaTable(DBOpParams *params);

    int DeleteBucketTable(DBOpParams *params);
    int DeleteUserTable(DBOpParams *params);
    int DeleteObjectTable(DBOpParams *params);
    int DeleteObjectDataTable(DBOpParams *params);

    int ListAllBuckets(DBOpParams *params);
    int ListAllUsers(DBOpParams *params);
    int ListAllObjects(DBOpParams *params);
};

class SQLObjectOp : public ObjectOp {
  private:
    sqlite3 **sdb = NULL;
    CephContext *cct;

  public:
    SQLObjectOp(sqlite3 **sdbi, CephContext *_cct) : sdb(sdbi), cct(_cct) {};
    ~SQLObjectOp() {}

    int InitializeObjectOps();
    int FreeObjectOps();
};

class SQLInsertUser : public SQLiteDB, public InsertUserOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLInsertUser(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLInsertUser() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLRemoveUser : public SQLiteDB, public RemoveUserOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLRemoveUser(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLRemoveUser() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLGetUser : public SQLiteDB, public GetUserOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement
    sqlite3_stmt *email_stmt = NULL; // Prepared statement to query by useremail
    sqlite3_stmt *ak_stmt = NULL; // Prepared statement to query by access_key_id
    sqlite3_stmt *userid_stmt = NULL; // Prepared statement to query by user_id

  public:
    SQLGetUser(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLGetUser() {
      if (stmt)
        sqlite3_finalize(stmt);
      if (email_stmt)
        sqlite3_finalize(email_stmt);
      if (ak_stmt)
        sqlite3_finalize(ak_stmt);
      if (userid_stmt)
        sqlite3_finalize(userid_stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLInsertBucket : public SQLiteDB, public InsertBucketOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLInsertBucket(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLInsertBucket() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLUpdateBucket : public SQLiteDB, public UpdateBucketOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *info_stmt = NULL; // Prepared statement
    sqlite3_stmt *attrs_stmt = NULL; // Prepared statement
    sqlite3_stmt *owner_stmt = NULL; // Prepared statement

  public:
    SQLUpdateBucket(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLUpdateBucket() {
      if (info_stmt)
        sqlite3_finalize(info_stmt);
      if (attrs_stmt)
        sqlite3_finalize(attrs_stmt);
      if (owner_stmt)
        sqlite3_finalize(owner_stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLRemoveBucket : public SQLiteDB, public RemoveBucketOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLRemoveBucket(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLRemoveBucket() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLGetBucket : public SQLiteDB, public GetBucketOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLGetBucket(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLGetBucket() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLListUserBuckets : public SQLiteDB, public ListUserBucketsOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLListUserBuckets(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    ~SQLListUserBuckets() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLInsertObject : public SQLiteDB, public InsertObjectOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLInsertObject(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLInsertObject(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLInsertObject() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLRemoveObject : public SQLiteDB, public RemoveObjectOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLRemoveObject(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLRemoveObject(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLRemoveObject() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLListObject : public SQLiteDB, public ListObjectOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLListObject(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLListObject(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLListObject() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLPutObjectData : public SQLiteDB, public PutObjectDataOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLPutObjectData(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLPutObjectData(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLPutObjectData() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLGetObjectData : public SQLiteDB, public GetObjectDataOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLGetObjectData(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLGetObjectData(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLGetObjectData() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};

class SQLDeleteObjectData : public SQLiteDB, public DeleteObjectDataOp {
  private:
    sqlite3 **sdb = NULL;
    sqlite3_stmt *stmt = NULL; // Prepared statement

  public:
    SQLDeleteObjectData(void **db, CephContext *cct) : SQLiteDB((sqlite3 *)(*db), cct), sdb((sqlite3 **)db) {}
    SQLDeleteObjectData(sqlite3 **sdbi, CephContext *cct) : SQLiteDB(*sdbi, cct), sdb(sdbi) {}

    ~SQLDeleteObjectData() {
      if (stmt)
        sqlite3_finalize(stmt);
    }
    int Prepare(DBOpParams *params);
    int Execute(DBOpParams *params);
    int Bind(DBOpParams *params);
};
#endif
