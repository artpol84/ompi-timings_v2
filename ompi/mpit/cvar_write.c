/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2013 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi/mpit/mpit-internal.h"

static const char FUNC_NAME[] = "MPI_T_cvar_write";

int MPI_T_cvar_write (MPI_T_cvar_handle handle, const void *buf)
{
    int rc = MPI_SUCCESS;

    if (!mpit_is_initialized ()) {
        return MPI_T_ERR_NOT_INITIALIZED;
    }

    if (MPI_PARAM_CHECK && NULL == buf) {
        return MPI_ERR_ARG;
    }

    mpit_lock ();

    do {
        if (MCA_BASE_VAR_SCOPE_CONSTANT == handle->var->mbv_scope ||
            MCA_BASE_VAR_SCOPE_READONLY == handle->var->mbv_scope) {
            rc = MPI_T_ERR_CVAR_SET_NEVER;
            break;
        }

        if (!(MCA_BASE_VAR_FLAG_SETTABLE & handle->var->mbv_flags)) {
            rc = MPI_T_ERR_CVAR_SET_NOT_NOW;
            break;
        }

        rc = mca_base_var_set_value(handle->var->mbv_index, buf, sizeof(unsigned long long), MCA_BASE_VAR_SOURCE_SET, NULL);
        if (OPAL_SUCCESS != rc) {
            rc = MPI_T_ERR_CVAR_SET_NOT_NOW;
        }
    } while (0);

    mpit_unlock ();

    return rc;
}
