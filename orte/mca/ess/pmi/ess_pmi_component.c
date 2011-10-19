/*
 * Copyright (c) 2011      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <pmi.h>

#include "orte/util/proc_info.h"

#include "orte/mca/ess/ess.h"
#include "orte/mca/ess/pmi/ess_pmi.h"

extern orte_ess_base_module_t orte_ess_pmi_module;

static int pmi_component_open(void);
static int pmi_component_close(void);
static int pmi_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */
orte_ess_base_component_t mca_ess_pmi_component = {
    {
        ORTE_ESS_BASE_VERSION_2_0_0,

        /* Component name and version */
        "pmi",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */
        pmi_component_open,
        pmi_component_close,
        pmi_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    }
};


static int pmi_component_open(void)
{
    return ORTE_SUCCESS;
}


static int pmi_component_query(mca_base_module_t **module, int *priority)
{
    int spawned;
    PMI_BOOL initialized;

    /* for now, only use PMI when direct launched */
    if (!ORTE_PROC_IS_HNP &&
        NULL == orte_process_info.my_hnp_uri &&
        PMI_SUCCESS == PMI_Initialized(&initialized)) {
        if (PMI_TRUE != initialized) {
            /* if we can't startup the PMI, we can't be used */
            if (PMI_SUCCESS != PMI_Init(&spawned)) {
                *priority = -1;
                *module = NULL;
                return ORTE_ERROR;
            }
            /* if PMI is available, use it */
            *priority = 100;
            *module = (mca_base_module_t *)&orte_ess_pmi_module;
            return ORTE_SUCCESS;
        }
    }

    /* we can't run */
    *priority = -1;
    *module = NULL;
    return ORTE_ERROR;
}


static int pmi_component_close(void)
{
    PMI_BOOL initialized;

    /* if we weren't selected, cleanup */
    if (PMI_SUCCESS == PMI_Initialized(&initialized) &&
        PMI_TRUE == initialized) {
        PMI_Finalize();
    }

    return ORTE_SUCCESS;
}
