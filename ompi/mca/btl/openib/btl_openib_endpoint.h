
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#ifndef MCA_BTL_IB_ENDPOINT_H
#define MCA_BTL_IB_ENDPOINT_H

#include "opal/class/opal_list.h"
#include "opal/event/event.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"
#include "btl_openib_frag.h"
#include "btl_openib.h"
#include <errno.h> 
#include <string.h> 
#include "mca/btl/base/btl_base_error.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
OBJ_CLASS_DECLARATION(mca_btl_openib_endpoint_t);


struct mca_btl_openib_frag_t;

struct mca_btl_openib_port_info_t {
    uint16_t subnet; 
};
typedef struct mca_btl_openib_port_info_t mca_btl_openib_port_info_t;


/**
 * State of IB endpoint connection.
 */

typedef enum {
    /* Defines the state in which this BTL instance
     * has started the process of connection */
    MCA_BTL_IB_CONNECTING,

    /* Waiting for ack from endpoint */
    MCA_BTL_IB_CONNECT_ACK,
    
    /*Waiting for final connection ACK from endpoint */ 
    MCA_BTL_IB_WAITING_ACK, 

    /* Connected ... both sender & receiver have
     * buffers associated with this connection */
    MCA_BTL_IB_CONNECTED,

    /* Connection is closed, there are no resources
     * associated with this */
    MCA_BTL_IB_CLOSED,

    /* Maximum number of retries have been used.
     * Report failure on send to upper layer */
    MCA_BTL_IB_FAILED
} mca_btl_openib_endpoint_state_t;

struct mca_btl_openib_rem_info_t { 
    
    uint32_t                    rem_qp_num_high;
    uint32_t                    rem_qp_num_low; 
    /* Remote QP number  (Low and High priority) */ 

    uint16_t                    rem_lid;
    /* Local identifier of the remote process */
    
    
    uint32_t                    rem_psn_high; 
    uint32_t                    rem_psn_low; 
    /* Remote processes port sequence number (Low and High) */ 
   
    uint16_t                    rem_subnet; 
    /* subnet of remote process */     

    
}; 
typedef struct mca_btl_openib_rem_info_t mca_btl_openib_rem_info_t; 



/**
 * An abstraction that represents a connection to a endpoint process.
 * An instance of mca_btl_base_endpoint_t is associated w/ each process
 * and BTL pair at startup. However, connections to the endpoint
 * are established dynamically on an as-needed basis:
 */

struct mca_btl_base_endpoint_t {
    opal_list_item_t            super;

    struct mca_btl_openib_module_t* endpoint_btl;
    /**< BTL instance that created this connection */

    struct mca_btl_openib_proc_t*   endpoint_proc;
    /**< proc structure corresponding to endpoint */

    mca_btl_openib_endpoint_state_t     endpoint_state;
    /**< current state of the connection */

    size_t                      endpoint_retries;
    /**< number of connection retries attempted */

    double                      endpoint_tstamp;
    /**< timestamp of when the first connection was attempted */

    opal_mutex_t                endpoint_lock;
    /**< lock for concurrent access to endpoint state */
    
    opal_list_t                 pending_send_frags;
    /**< list of pending send frags for this endpotint */
    
    opal_list_t                 pending_frags_hp; 
    /**< list of pending high priority frags */ 

    opal_list_t                 pending_frags_lp; 
    /**< list of pending low priority frags */ 

    
    int32_t                     wr_sq_tokens_hp; 
    /**< number of high priority frags that  can be outstanding (down counter) */ 

    int32_t                     wr_sq_tokens_lp; 
    /**< number of low priority frags that  can be outstanding (down counter) */ 
    

    mca_btl_openib_rem_info_t   rem_info;
    
    uint32_t                    lcl_psn_high; 
    uint32_t                    lcl_psn_low; 
    /* Local processes port sequence number (Low and High) */
 
    struct ibv_qp*              lcl_qp_high;
    struct ibv_qp*              lcl_qp_low;
    /* Local QP (Low and High) */

    struct ibv_qp_attr*         lcl_qp_attr_high; 
    struct ibv_qp_attr*         lcl_qp_attr_low; 
    /* Local QP attributes (Low and High) */

    uint32_t rr_posted_high;  /**< number of high priority rr posted to the nic*/ 
    uint32_t rr_posted_low;  /**< number of low priority rr posted to the nic*/ 
    
    uint16_t subnet; /**< subnet of this endpoint*/
};

typedef struct mca_btl_base_endpoint_t mca_btl_base_endpoint_t;
typedef mca_btl_base_endpoint_t  mca_btl_openib_endpoint_t;

int  mca_btl_openib_endpoint_send(mca_btl_base_endpoint_t* endpoint, struct mca_btl_openib_frag_t* frag);
int  mca_btl_openib_endpoint_connect(mca_btl_base_endpoint_t*);
void mca_btl_openib_post_recv(void);


    
#define MCA_BTL_OPENIB_ENDPOINT_POST_RR_HIGH(endpoint, \
                                             additional) \
{ \
   do { \
    mca_btl_openib_module_t * openib_btl = endpoint->endpoint_btl; \
    OPAL_THREAD_LOCK(&openib_btl->ib_lock); \
    if(endpoint->rr_posted_high <= mca_btl_openib_component.ib_rr_buf_min+additional && \
       endpoint->rr_posted_high < mca_btl_openib_component.ib_rr_buf_max){ \
        MCA_BTL_OPENIB_ENDPOINT_POST_RR_SUB(mca_btl_openib_component.ib_rr_buf_max -  \
                                            endpoint->rr_posted_high, \
                                            endpoint, \
                                            &openib_btl->recv_free_eager, \
                                            &endpoint->rr_posted_high, \
                                            endpoint->lcl_qp_high); \
    } \
    OPAL_THREAD_UNLOCK(&openib_btl->ib_lock); \
   } while(0); \
}

#define MCA_BTL_OPENIB_ENDPOINT_POST_RR_LOW(endpoint, \
                                            additional) { \
    do { \
    mca_btl_openib_module_t * openib_btl = endpoint->endpoint_btl; \
    OPAL_THREAD_LOCK(&openib_btl->ib_lock); \
    if(endpoint->rr_posted_low <= mca_btl_openib_component.ib_rr_buf_min+additional && \
       endpoint->rr_posted_low < mca_btl_openib_component.ib_rr_buf_max){ \
       MCA_BTL_OPENIB_ENDPOINT_POST_RR_SUB(mca_btl_openib_component.ib_rr_buf_max - \
                                            endpoint->rr_posted_low,  \
                                            endpoint, \
                                            &openib_btl->recv_free_max, \
                                            &endpoint->rr_posted_low, \
                                            endpoint->lcl_qp_low \
                                            ); } \
    OPAL_THREAD_UNLOCK(&openib_btl->ib_lock); \
    } while(0); \
}

#define MCA_BTL_OPENIB_ENDPOINT_POST_RR_SUB(cnt, \
                                            my_endpoint, \
                                            frag_list, \
                                            rr_posted, \
                                            qp ) \
{\
    do { \
    uint32_t i; \
    int rc; \
    opal_list_item_t* item; \
    mca_btl_openib_frag_t* frag; \
    struct ibv_recv_wr* bad_wr; \
    for(i = 0; i < cnt; i++) { \
        OMPI_FREE_LIST_WAIT(frag_list, item, rc); \
        frag = (mca_btl_openib_frag_t*) item; \
        frag->endpoint = my_endpoint; \
        frag->sg_entry.length = frag->size + \
            ((unsigned char*) frag->segment.seg_addr.pval-  \
             (unsigned char*) frag->hdr);  \
        if(ibv_post_recv(qp, \
            &frag->wr_desc.rr_desc, \
            &bad_wr)) { \
            BTL_ERROR(("error posting receive errno says %s\n", strerror(errno))); \
            return OMPI_ERROR; \
        }\
    }\
    OPAL_THREAD_ADD32((int32_t*) rr_posted, cnt); \
    } while(0); \
}

#define BTL_OPENIB_INSERT_PENDING(frag, frag_list, tokens, lock) \
{ \
 do{ \
     OPAL_THREAD_LOCK(&lock); \
     opal_list_append(&frag_list, (opal_list_item_t *)frag); \
     OPAL_THREAD_UNLOCK(&lock); \
     OPAL_THREAD_ADD32(&tokens, 1); \
 } while(0); \
}


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif
