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
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */


#include "orte_config.h"
#include "orte/orte_constants.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/util/output.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/threads/condition.h"

#include "orte/mca/oob/oob.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "orte/mca/oob/base/static-components.h"


/*
 * Global variables
 */
mca_oob_t mca_oob;
int mca_oob_base_output = -1;
opal_list_t mca_oob_base_components;
opal_list_t mca_oob_base_modules;
opal_list_t mca_oob_base_exception_handlers;
bool orte_oob_base_timing;
bool orte_oob_xcast_timing;
int orte_oob_xcast_mode;
opal_mutex_t orte_oob_xcast_mutex;
opal_condition_t orte_oob_xcast_cond;

bool orte_oob_base_already_opened = false;

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_oob_base_open(void)
{
    int param, value;
    char *mode;

    /* Sanity check.  This may be able to be removed when the rml/oob
       interface is re-worked (the current infrastructure may invoke
       this function twice: once as a standalone, and once via the rml
       oob component). */
    if (orte_oob_base_already_opened) {
        return ORTE_SUCCESS;
    }
    
    /* initialize the condition variables for xcast */
    OBJ_CONSTRUCT(&orte_oob_xcast_mutex, opal_mutex_t);
    OBJ_CONSTRUCT(&orte_oob_xcast_cond, opal_condition_t);
    
    /* Open up all available components */

  OBJ_CONSTRUCT(&mca_oob_base_components, opal_list_t);
  OBJ_CONSTRUCT(&mca_oob_base_modules, opal_list_t);
  OBJ_CONSTRUCT(&mca_oob_base_exception_handlers, opal_list_t);

  if (ORTE_SUCCESS != 
      mca_base_components_open("oob", mca_oob_base_output,
                               mca_oob_base_static_components, 
                               &mca_oob_base_components, true)) {
    return ORTE_ERROR;
  }

  /* register parameters */
  param = mca_base_param_reg_int_name("orte", "timing",
                                      "Request that critical timing loops be measured",
                                      false, false, 0, &value);
  if (value != 0) {
      orte_oob_base_timing = true;
  } else {
      orte_oob_base_timing = false;
  }
  
  param = mca_base_param_reg_int_name("oob", "xcast_timing",
                                      "Request that xcast timing loops be measured",
                                      false, false, 0, &value);
  if (value != 0) {
      orte_oob_xcast_timing = true;
  } else {
      orte_oob_xcast_timing = false;
  }
  
  param = mca_base_param_reg_string_name("oob", "xcast_mode",
                                           "Select xcast mode (\"linear\" | \"binomial\" | \"direct [default] \")",
                                           false, false, "direct", &mode);
    if (0 == strcmp(mode, "binomial")) {
        orte_oob_xcast_mode = 0;
    } else if (0 == strcmp(mode, "linear")) {
        orte_oob_xcast_mode = 1;
    } else if (0 == strcmp(mode, "direct")) {
        orte_oob_xcast_mode = 2;
    } else {
        opal_output(0, "oob_xcast_mode: unknown option %s", mode);
        return ORTE_ERROR;
    }
    
    /* All done */
    orte_oob_base_already_opened = true;
    
    return ORTE_SUCCESS;
}

