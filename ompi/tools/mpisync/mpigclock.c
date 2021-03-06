/*
 * Copyright (c) 2010-2011, Siberian State University of Telecommunications 
 *                         and Information Sciences. All rights reserved.
 * Copyright (c) 2010-2011, A.V. Rzhanov Institute of Semiconductor Physics SB RAS.
 *                        All rights reserved.
 *
 * mpigclock.c: MPI clock synchronization.
 *
 * Copyright (C) 2011 Mikhail Kurnosov <mkurnosov@gmail.com>
 *
 * This source code is part of MPIPerf project: http://mpiperf.cpct.sibsutis.ru/index.php/Main/Documentation
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "mpigclock.h"
#include "hpctimer.h"

#define INVALIDTIME -1.0
#define MPIGCLOCK_RTTMIN_NOTCHANGED_MAX 100
#define MPIGCLOCK_MSGTAG 128

static double mpigclock_measure_offset_adaptive(MPI_Comm comm, int root, int peer, double *min_rtt);


/*
 * mpigclock_sync_linear: Clock synchronization algorithm with O(n) steps.
 */
double mpigclock_sync_linear(MPI_Comm comm, int root, double *rtt)
{
    int i, rank, commsize;
    double ret = 0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &commsize);

    if (commsize < 2) {
        *rtt = 0.0;
        return 0.0;
    }

    for (i = 1; i < commsize; i++) {
        MPI_Barrier(comm);
        if (rank == root || rank == i) {
            ret = mpigclock_measure_offset_adaptive(comm, root, i, rtt);
        }
    }
    return ret;
}

/* mpigclock_measure_offset_adaptive: Measures clock's offset of peer. */
static double mpigclock_measure_offset_adaptive(MPI_Comm comm, int root, int peer, double *min_rtt)
{
    int rank, commsize, rttmin_notchanged = 0;
    double starttime, stoptime, peertime, rtt, rttmin = 1E12,
           invalidtime = INVALIDTIME, offset;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &commsize);

    offset = 0.0;
    for (;;) {
        if (rank != root) {
            /* Peer process */
            starttime = hpctimer_wtime();
            MPI_Send(&starttime, 1, MPI_DOUBLE, root, MPIGCLOCK_MSGTAG, comm);
            MPI_Recv(&peertime, 1, MPI_DOUBLE, root, MPIGCLOCK_MSGTAG, comm,
                     MPI_STATUS_IGNORE);
            stoptime = hpctimer_wtime();
            rtt = stoptime - starttime;

            if (rtt < rttmin) {
                rttmin = rtt;
                rttmin_notchanged = 0;
                offset =  peertime - rtt / 2.0 - starttime;
            } else {
                if (++rttmin_notchanged == MPIGCLOCK_RTTMIN_NOTCHANGED_MAX) {
                    MPI_Send(&invalidtime, 1, MPI_DOUBLE, root, MPIGCLOCK_MSGTAG,
                             comm);
                    break;
                }
            }
        } else {
            /* Root process */
            MPI_Recv(&starttime, 1, MPI_DOUBLE, peer, MPIGCLOCK_MSGTAG, comm,
                     MPI_STATUS_IGNORE);
            peertime = hpctimer_wtime();
            if (starttime < 0.0) {
                break;
            }
            MPI_Send(&peertime, 1, MPI_DOUBLE, peer, MPIGCLOCK_MSGTAG, comm);
        }
    } /* for */

    if( rank != root ){
        *min_rtt = rttmin;
    } else {
        rtt = 0.0;
    }
    return offset;
}
