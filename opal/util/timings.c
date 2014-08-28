#define _GNU_SOURCE
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "opal_config.h"

// TODO : restore ifdefs
//#ifdef HAVE_STRING_H
#include <string.h>
//#endif

#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "opal/constants.h"
#include "opal/runtime/opal_params.h"


#include "opal/class/opal_pointer_array.h"
#include "opal/class/opal_list.h"
#include "opal/util/timings.h"
#include "opal/util/output.h"

#if OPAL_ENABLE_DEBUG


static void debug_hang(int i)
{
  while( i ){
    sleep(1);
  }
}

double opal_timing_get_ts(void);
opal_timing_event_t *opal_timing_event_alloc(opal_timing_t *t);
void opal_timing_init(opal_timing_t *t);
opal_timing_prep_t opal_timing_prep_ev(opal_timing_t *t, const char *fmt, ...);

int opal_timing_report(opal_timing_t *t, const char *prefix, char *fname);
void opal_timing_release(opal_timing_t *t);

static OBJ_CLASS_INSTANCE(opal_timing_event_t, opal_list_item_t, NULL, NULL);


opal_mutex_t tm_lock;
static char *nodename = NULL;
static char *jobid = "";
static double hnp_offs = 0;
// TODO use RTT to estimate precise of measurement
static double hnp_rtt = 0;

int opal_timing_clksync_read(char *fname)
{
    int rc = 0;
    FILE *fp = NULL;
    char *line = NULL;
    size_t n;
    bool found = false;

    fp = fopen(fname,"r");
    if( fp == NULL ){
        return -1;
    }
    char hname[1024];
    if( gethostname(hname, 1024) ){
        rc = -1;
        goto err_exit;
    }
    nodename = strdup(hname);


    while( getline(&line,&n,fp) > 0 ){
        char *ptr = strchr(line,' ');
        if( ptr == NULL ){
            rc = -1;
            goto err_exit;
        }
        *ptr = '\0';
        ptr++;
        if( strcmp(line, hname) == 0 ){
            if( sscanf(ptr,"%lf %lf", &hnp_rtt, &hnp_offs) != 2 ){
                rc = -1;
                goto err_exit;
            }
            break;
        }
    }

    if( !found ){
        opal_output(0,"opal_timing_clksync_read: Can't find my host %s in %s", hname, fname);
        rc = -1;
    }

err_exit:

    if( line != NULL ){
        free(line);
    }

    if( fp != NULL ){
        fclose(fp);
    }
    return rc;
}

int opal_timing_set_jobid(char *jid)
{
    jobid = strdup(jid);
    if( jobid == NULL ){
        return -1;
    }
    return 0;
}

/* Get current timestamp */
double opal_timing_get_ts(void){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    double ret = tv.tv_sec + tv.tv_usec*1E-6;
    return ret;
}

opal_timing_event_t *opal_timing_event_alloc(opal_timing_t *t)
{
    if( t->buffer_offset >= t->buffer_size ){
        // notch timings overhead 
        double alloc_begin = opal_timing_get_ts();

        t->buffer = malloc(sizeof(opal_timing_event_t)*t->buffer_size);
        if( t->buffer == NULL ){
            // TODO: out of memory error process
        }
        memset(t->buffer, 0, sizeof(opal_timing_event_t)*t->buffer_size);

        double alloc_end = opal_timing_get_ts();

        t->buffer_offset = 0;
        t->buffer[0].fib = 1;
        t->buffer[0].ts_ovh = alloc_end - alloc_begin;
    }
    int tmp = t->buffer_offset;
    (t->buffer_offset)++;
    return t->buffer + tmp;
}

void opal_timing_init(opal_timing_t *t)
{
    memset(t,0,sizeof(*t));

    t->cur_id = 0;
    // initialize events list
    t->events = OBJ_NEW(opal_list_t);
    t->buffer_size = OPAL_TIMING_BUFSIZE;
    t->buffer = malloc(sizeof(opal_timing_event_t)*t->buffer_size);
    if( t->buffer == NULL ){
        // TODO: alloc err handler
    }
    OPAL_TIMING_EVENT((t,"Init"));
}

opal_timing_prep_t opal_timing_prep_ev(opal_timing_t *t, const char *fmt, ...)
{
    debug_hang(0);
    
    opal_timing_event_t *ev = opal_timing_event_alloc(t);
    OBJ_CONSTRUCT(ev, opal_timing_event_t);
    ev->ts = opal_timing_get_ts();
    va_list args;
    va_start( args, fmt );
    vsnprintf(ev->descr, OPAL_TIMING_DESCR_MAX - 1, fmt, args);
    ev->descr[OPAL_TIMING_DESCR_MAX-1] = '\0';
    va_end( args );
    opal_timing_prep_t p = { t, ev };
    return p;
}

void opal_timing_add_step(opal_timing_prep_t p,
                          const char *func, const char *file, int line)
{
    p.ev->func = func;
    p.ev->file = file;
    p.ev->line = line;
    p.ev->type = TEVENT;
    opal_list_append(p.t->events, (opal_list_item_t*)p.ev);
}
/*
int opal_timing_begin(opal_timing_t *t, char *file, int line)
{
    opal_timing_event_t *ev = opal_timing_event_alloc(t);
    ev->ts = opal_timing_get_ts();
    ev->file = file;
    ev->line = line;
    ev->type = TBEGIN;
    ev->id = opal_timing_get_id(t);
    opal_list_append(&t->events, (opal_list_item_t*)ev);
    return ev->id;
}

opal_timing_prep_t opal_timing_prep_end(opal_timing_t *t, int id, const char *fmt, ...)
{
    opal_timing_event_t *ev = opal_timing_event_alloc(t);
    ev->ts = opal_timing_get_ts();
    ev->id = id;
    va_list args;
    va_start( args, fmt );
    vsnprintf(ev->descr, OPAL_TIMING_DESCR_MAX - 1, fmt, args);
    ev->descr[OPAL_TIMING_DESCR_MAX-1] = '\0';
    va_end( args );
    opal_timing_prep_t p = { t, ev };
    return p;
}

void opal_timing_end(opal_timing_prep_t p, char *file, int line)
{
    p.ev->file = file;
    p.ev->line = line;
    p.ev->type = TEND;
    opal_list_append(&p.t->events, (opal_list_item_t*)p.ev);
}
*/

int opal_timing_report(opal_timing_t *t, const char *prefix, char *fname)
{
    opal_timing_event_t *ev, *ev_prev, *ev_first;
    int count = 0;
    FILE *fp = NULL;
    char *buf = NULL;
    int buf_size = 0;
    int rc = 0;

    debug_hang(0);

    if( fname != NULL ){
        fp = fopen(fname,"a");
        if( fp == NULL ){
            // TODO: log error
            rc = OPAL_ERROR;
            goto err_exit;
        }
    }
    
    buf = malloc(OPAL_TIMING_OUTBUF_SIZE+1);
    if( buf == NULL ){
        // TODO: log error
        rc = OPAL_ERROR;
        goto err_exit;
    }
    buf[0] = '\0';

    double overhead = 0;
    OPAL_LIST_FOREACH(ev, t->events, opal_timing_event_t){
        count++;
        if( ev->fib ){
            overhead += ev->ts_ovh;
        }
        if( count > 1){
            char *line;
            const char *file_name = ev->file;
            const char *ptr = file_name;
            for( ; *ptr != '\0' ; ptr++ ){
                if( *ptr == '/'){
                    file_name = ptr+1;
                }
            }
            if( prefix != NULL ){
                rc = asprintf(&line,"%s:\t%lfs\t%lfs\t\"%s\"\t|\t%s\t%s\t%s\t%s:%d\n",
                              prefix,ev->ts + hnp_offs, ev->ts + hnp_offs + overhead,
                              ev->descr, nodename, jobid, ev->func, file_name, ev->line);
            } else {
                rc = asprintf(&line,"%lfs\t%lfs\t\"%s\"\t|\t%s\t%s\t%s\t%s:%d\n",
                              ev->ts + hnp_offs, ev->ts + hnp_offs + overhead,
                              ev->descr, nodename, jobid, ev->func, file_name, ev->line);
            }
            if( rc < 0 ){
                // TODO: log mem allocation problems
                goto err_exit;
            }
            rc = 0;

            if( strlen(line) > OPAL_TIMING_OUTBUF_SIZE ){
                // TODO: log buffer overflow
                free(line);
                goto err_exit;
            }
            if( buf_size + strlen(line) > OPAL_TIMING_OUTBUF_SIZE ){
                // flush buffer to the file
                if( fp != NULL ){
                    fprintf(fp,"%s", buf);
                    fprintf(fp,"\n");
                } else {
                    opal_output(0,"\n%s", buf);
                }
                buf[0] = '\0';
                buf_size = 0;
            }
            sprintf(buf,"%s%s", buf, line);
            buf_size += strlen(line);
            free(line);
            ev_prev = ev;
        }else{
            ev_first = ev;
            ev_prev = ev;
        }
    }

    if( buf_size > 0 ){
        // flush buffer to the file
        if( fp != NULL ){
            fprintf(fp,"%s", buf);
            fprintf(fp,"\n");
        } else {
            opal_output(0,"\n%s", buf);
        }
        buf[0] = '\0';
        buf_size = 0;
    }

err_exit:
    if( buf != NULL ){
        free(buf);
    }
    if( fp != NULL ){
        fclose(fp);
    }
    return rc;
}

void opal_timing_release(opal_timing_t *t)
{
    opal_list_t *tmp = OBJ_NEW(opal_list_t);

    int cnt = opal_list_get_size(t->events);
    int i;
    for(i=0; i<cnt; i++){
        opal_timing_event_t *ev = (opal_timing_event_t *)opal_list_remove_first(t->events);
        if( ev->fib ){
            opal_list_append(tmp,(opal_list_item_t*)ev);
        }
    }

    cnt = opal_list_get_size(tmp);
    for(i=0; i<cnt; i++){
        opal_timing_event_t *ev = (opal_timing_event_t *)opal_list_remove_first(tmp);
        free(ev);
    }

    OBJ_RELEASE(tmp);
    OBJ_RELEASE(t->events);
    t->events = NULL;
}

#endif