/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004 The Ohio State University.
 *                    All rights reserved.
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

#include "ompi_config.h"
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"

#define MYBUFSIZE (1<<21)
#define CHECK 0
#define PONG 1

char        s_buf[MYBUFSIZE];
char        r_buf[MYBUFSIZE];
int         skip = 40;

int
main (int argc, char *argv[])
{
    char hostname[32];

    int         myid, numprocs, i, j;
    double      startwtime = 0.0, endwtime;
    int         namelen;
    int         size;
    int         loop;
    MPI_Status  stat;
    int         sleep = 1;

    struct timeval t_start, t_end;

    /*loop = 2;*/

    if (argc < 2) {
        fprintf (stderr, "Usage: %s msg_size\n", argv[0]);
        return 0;
    } else {
	size = atoi (argv[1]);
	if (argc > 2) 
	    loop = atoi (argv[2]);
    }

    setenv("OMPI_MCA_ptl_base_exclude", "tcp", 1);

    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank (MPI_COMM_WORLD, &myid);
    printf("the size is %d\n",numprocs);

    /* touch the data */
    for (i = 0; i < size; i++) {
        s_buf[i] = 'A' + i%26;
    }

    gethostname(hostname, 32);

    fprintf(stdout, "[%s:%s:%d] done with init and barrier\n",
	    hostname, __FUNCTION__, __LINE__);
    fflush(stdout);

    for (i = 0; i < loop + skip; i++) {
	if (i == skip)
	    gettimeofday (&t_start, 0);
	if (myid == 0) {
            MPI_Send (s_buf, size, MPI_CHAR, 1, i, MPI_COMM_WORLD);
	    /*if (PONG)*/
            MPI_Recv (r_buf, size, MPI_CHAR, 1, i, MPI_COMM_WORLD, &stat);
	} else {
            MPI_Recv (r_buf, size, MPI_CHAR, 0, i, MPI_COMM_WORLD, &stat);
	    /*if (PONG)*/
            MPI_Send (s_buf, size, MPI_CHAR, 0, i, MPI_COMM_WORLD);
        }

	if (CHECK && myid != 0) {
	    for (j=0; j < size; j ++) {
		if (r_buf[j] != 'A' + j%26) {
		    fprintf(stderr, "[%s:%s] error from byte %d \n",
			    hostname, __FUNCTION__, j);
		    break;
		} else {
		    r_buf[j] = '0';
		}
	    } 
	}
    }
    gettimeofday (&t_end, 0);

    fprintf(stdout, "[%s:%s:%d] done with pingpong\n",
	    hostname, __FUNCTION__, __LINE__);
    fflush(stdout);
 
    if (myid == 0) {
        double      latency;
        latency = ((1.0e6 * t_end.tv_sec + t_end.tv_usec) 
		- (1.0e6 * t_start.tv_sec + t_start.tv_usec)) / (2.0 * loop);
	fprintf(stdout, "length %d latency %8f\n", 
		size, latency);
	fflush(stdout);
    }
    MPI_Finalize ();
    return 0;
}
