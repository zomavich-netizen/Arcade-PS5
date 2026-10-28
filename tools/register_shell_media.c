/* Repair only Arcade's registered home-screen media. GPL-3.0-or-later.
 * Run as an ELF with filesystem access after installing pic0.dds/snd0.at9.
 * Live SQLite transactions only: never replace the system database files. */
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static sqlite3 *db;
static FILE *logfile;
static int sql(const char *q){char *e=0;int rc=sqlite3_exec(db,q,0,0,&e);if(rc){fprintf(logfile,"SQL error: %s\n",e?e:sqlite3_errmsg(db));sqlite3_free(e);}return rc;}
static int rollback_journal(const char*q){sqlite3_stmt*s=0;int ok=0;if(sqlite3_prepare_v2(db,q,-1,&s,0)==SQLITE_OK&&sqlite3_step(s)==SQLITE_ROW){const char*v=(const char*)sqlite3_column_text(s,0);ok=v&&(!strcmp(v,"delete")||!strcmp(v,"truncate")||!strcmp(v,"persist"));}sqlite3_finalize(s);return ok;}
static int backup(const char *schema,const char *path){sqlite3 *out=0;int rc=sqlite3_open(path,&out);if(!rc){sqlite3_backup*b=sqlite3_backup_init(out,"main",db,schema);if(!b)rc=SQLITE_ERROR;else {rc=sqlite3_backup_step(b,-1);sqlite3_backup_finish(b);if(rc==SQLITE_DONE)rc=SQLITE_OK;}}sqlite3_close(out);return rc;}
static int scalar(const char*q){sqlite3_stmt*s=0;int value=0;if(sqlite3_prepare_v2(db,q,-1,&s,0)==SQLITE_OK&&sqlite3_step(s)==SQLITE_ROW)value=sqlite3_column_int(s,0);sqlite3_finalize(s);return value;}
static int media(const char*path,const char*sig,long size){FILE*f=fopen(path,"rb");if(!f)return 0;char b[4];int ok=fread(b,1,4,f)==4&&!memcmp(b,sig,4);fseek(f,0,SEEK_END);long n=ftell(f);fclose(f);return ok&&(size?n==size:n>1000);}
int main(int argc,char**argv){
 const char*root="";
#ifdef ARCADE_MEDIA_HOST
 if(argc!=2)return 2;root=argv[1];
#else
 (void)argc;(void)argv;
#endif
 char path[1200],out[1200],q[8192];
 snprintf(path,sizeof path,"%s/data/arcade-shell-media.log",root);logfile=fopen(path,"w");if(!logfile)return 2;
 int result=1;time_t stamp=time(0);fprintf(logfile,"Arcade shell media repair %lld\n",(long long)stamp);fflush(logfile);
 snprintf(path,sizeof path,"%s/user/app/PPSA99984/sce_sys/pic0.dds",root);if(!media(path,"DDS ",8294548))goto done;
 snprintf(path,sizeof path,"%s/user/app/PPSA99984/sce_sys/snd0.at9",root);if(!media(path,"RIFF",0))goto done;
 snprintf(path,sizeof path,"%s/system_data/priv/mms/app.db",root);if(sqlite3_open_v2(path,&db,SQLITE_OPEN_READWRITE,0))goto done;
 sqlite3_busy_timeout(db,5000);
 snprintf(path,sizeof path,"%s/system_data/priv/mms/appinfo.db",root);char*attach=sqlite3_mprintf("ATTACH DATABASE %Q AS registry",path);int rc=sql(attach);sqlite3_free(attach);if(rc)goto done;
 if(!scalar("SELECT count(*)=1 FROM tbl_contentinfo WHERE titleId='PPSA99984' AND metaDataPath='/user/app/PPSA99984/sce_sys' AND json_valid(AppInfoJson)"))goto done;
 if(!scalar("SELECT count(*)=1 FROM registry.tbl_appinfo WHERE titleId='PPSA99984' AND metaDataId='prior:internal:0' AND key='_local_concept_id' AND val='cid:scp:0000000000018690'"))goto done;
 /* Refuse WAL: the two attached files must commit through a super-journal. */
 if(!rollback_journal("PRAGMA main.journal_mode")||!rollback_journal("PRAGMA registry.journal_mode"))goto done;
 snprintf(out,sizeof out,"%s/data/arcade-media-backup-%lld",root,(long long)stamp);if(mkdir(out,0700))goto done;
 snprintf(path,sizeof path,"%s/app.db",out);if(backup("main",path))goto done;
 snprintf(path,sizeof path,"%s/appinfo.db",out);if(backup("registry",path))goto done;
 fprintf(logfile,"Backups: %s\n",out);fflush(logfile);
 if(sql("PRAGMA main.synchronous=FULL; PRAGMA registry.synchronous=FULL; BEGIN IMMEDIATE"))goto done;
 snprintf(q,sizeof q,
 "INSERT INTO registry.tbl_appinfo(titleId,metaDataId,key,val) VALUES"
 "('PPSA99984','prior:internal:0','_path_pic0_info',-6917529027641081856),"
 "('PPSA99984','prior:internal:0','_path_pic0_info_time_stamp',%lld),"
 "('PPSA99984','prior:internal:0','_path_snd0_info_time_stamp',%lld) "
 "ON CONFLICT(titleId,metaDataId,key) DO UPDATE SET val=excluded.val;"
 "INSERT INTO registry.tbl_conceptinfo(local_concept_id,metadata_id,key,val) VALUES"
 "('cid:scp:0000000000018690',NULL,'path_pic0_info',-6917529027641081856),"
 "('cid:scp:0000000000018690',NULL,'path_pic0_info_time_stamp',%lld),"
 "('cid:scp:0000000000018690',NULL,'path_snd0_info_time_stamp',%lld) "
 "ON CONFLICT(local_concept_id,key) DO UPDATE SET val=excluded.val;"
 "UPDATE tbl_contentinfo SET pic0Info='/user/app/PPSA99984/sce_sys/pic0.dds?ts=%lld',"
 "snd0Info='/user/app/PPSA99984/sce_sys/snd0.at9?ts=%lld',"
 "AppInfoJson=json_set(AppInfoJson,'$._path_pic0_info',-6917529027641081856,'$._path_pic0_info_time_stamp',%lld,'$._path_snd0_info_time_stamp',%lld) WHERE titleId='PPSA99984';",
 (long long)stamp,(long long)stamp,(long long)stamp,(long long)stamp,(long long)stamp,(long long)stamp,(long long)stamp,(long long)stamp);
 if(sql(q))goto rollback;
 /* Update existing shell view tables using the exact registered title/concept. */
 sqlite3_stmt*t=0;if(sqlite3_prepare_v2(db,"SELECT name FROM sqlite_master WHERE type='table' AND name GLOB 'tbl_concepticoninfo_*'",-1,&t,0))goto rollback;
 while(sqlite3_step(t)==SQLITE_ROW){const char*name=(const char*)sqlite3_column_text(t,0);char *update=sqlite3_mprintf("UPDATE \"%w\" SET pic0Info='/user/app/PPSA99984/sce_sys/pic0.dds?ts=%lld',snd0Info='/user/app/PPSA99984/sce_sys/snd0.at9?ts=%lld' WHERE primaryTitleId='PPSA99984' AND localConceptId='cid:scp:0000000000018690'",name,(long long)stamp,(long long)stamp);rc=sql(update);sqlite3_free(update);if(rc)break;}
 sqlite3_finalize(t);if(rc)goto rollback;
 /* Mirror concept metadata while SQLite retains exact signed 64-bit data. */
 snprintf(q,sizeof q,"UPDATE tbl_conceptmetadata SET pic0Info='/user/app/PPSA99984/sce_sys/pic0.dds?ts=%lld',ConceptInfoJson=json_insert(json_set(ConceptInfoJson,'$.field_list',(SELECT json_group_array(json(value)) FROM json_each(ConceptInfoJson,'$.field_list') WHERE json_extract(value,'$.key') NOT IN ('path_pic0_info','path_pic0_info_time_stamp','path_snd0_info_time_stamp'))),'$.field_list[#]',json_object('data',-6917529027641081856,'key','path_pic0_info','size',8,'type',0),'$.field_list[#]',json_object('data',%lld,'key','path_pic0_info_time_stamp','size',8,'type',0),'$.field_list[#]',json_object('data',%lld,'key','path_snd0_info_time_stamp','size',8,'type',0)) WHERE localConceptId='cid:scp:0000000000018690' AND json_valid(ConceptInfoJson)",(long long)stamp,(long long)stamp,(long long)stamp);
 if(sql(q))goto rollback;
 if(!scalar("SELECT count(*)=1 FROM tbl_contentinfo WHERE titleId='PPSA99984' AND json_extract(AppInfoJson,'$._path_pic0_info')=-6917529027641081856 AND pic0Info IS NOT NULL AND snd0Info IS NOT NULL"))goto rollback;
 if(sql("COMMIT"))goto rollback;
 fprintf(logfile,"SUCCESS: Arcade background and music registered; home screen may need restart.\n");result=0;goto done;
rollback: sql("ROLLBACK");fprintf(logfile,"Rolled back.\n");
done: if(result)fprintf(logfile,"FAILED: repair incomplete; %s\n",db?sqlite3_errmsg(db):"check installed media");fflush(logfile);if(db)sqlite3_close(db);fclose(logfile);return result;
}
