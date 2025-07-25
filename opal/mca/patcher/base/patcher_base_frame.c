/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2016      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "opal_config.h"

#include "opal/mca/patcher/base/base.h"
#include "opal/mca/patcher/base/static-components.h"
#include "opal/mca/patcher/patcher.h"

/*
 * Local variables
 */
static mca_patcher_base_module_t empty_module;

/*
 * Globals
 */
mca_patcher_base_module_t *opal_patcher = &empty_module;

int opal_patcher_base_select(void)
{
    mca_patcher_base_module_t *best_module;
    mca_patcher_base_component_t *best_component;
    int rc, priority;

    rc = mca_base_select("patcher", opal_patcher_base_framework.framework_output,
                         &opal_patcher_base_framework.framework_components,
                         (mca_base_module_t **) &best_module,
                         (mca_base_component_t **) &best_component, &priority);
    if (OPAL_SUCCESS != rc) {
        return rc;
    }

    OBJ_CONSTRUCT(&best_module->patch_list, opal_list_t);
    OBJ_CONSTRUCT(&best_module->patch_list_mutex, opal_mutex_t);

    if (best_module->patch_init) {
        rc = best_module->patch_init();
        if (OPAL_SUCCESS != rc) {
            return rc;
        }
    }

    opal_patcher = best_module;

    return OPAL_SUCCESS;
}

int opal_patcher_base_restore_all(void)
{
    mca_patcher_base_patch_t *patch, *patch_next;

    if (opal_patcher == &empty_module) {
        return OPAL_SUCCESS;
    }

    opal_mutex_lock(&opal_patcher->patch_list_mutex);

    OPAL_LIST_FOREACH_SAFE_REV(patch, patch_next, &opal_patcher->patch_list, mca_patcher_base_patch_t) {
        patch->patch_restore (patch);
        opal_list_remove_item(&opal_patcher->patch_list, &patch->super);
        OBJ_RELEASE(patch);
    }

    opal_mutex_unlock(&opal_patcher->patch_list_mutex);

    return OPAL_SUCCESS;
}

static int opal_patcher_base_close(void)
{
    if (opal_patcher == &empty_module) {
        return OPAL_SUCCESS;
    }

    opal_patcher_base_restore_all();

    OPAL_LIST_DESTRUCT(&opal_patcher->patch_list);
    OBJ_DESTRUCT(&opal_patcher->patch_list_mutex);

    if (opal_patcher->patch_fini) {
        return opal_patcher->patch_fini();
    }

    return mca_base_framework_components_close(&opal_patcher_base_framework, NULL);
}

/* Use default register/open functions */
MCA_BASE_FRAMEWORK_DECLARE(opal, patcher, "runtime code patching", NULL, NULL,
                           opal_patcher_base_close, mca_patcher_base_static_components, 0);
