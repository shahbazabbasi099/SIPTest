/**
 * pj_extras.c
 * TalkHome
 *
 * Copyright © 2015 Nowtel Management Ltd. All rights reserved.
 */

#include "pj_extras.h"

#include "pj_g729.h"
#include "pjsip_talkhome.h"


PJ_DEF(void) pj_extras_init()
{
    pj_status_t status;
    
    pjmedia_endpt *media_endpt = pjsua_get_pjmedia_endpt();
    
    if ((status = pjmedia_codec_g729_init(media_endpt)) != PJ_SUCCESS) {
        pjsua_perror(__FUNCTION__, "pjmedia_codec_g729_init", status);
    }
    
    if ((status = pjsip_talkhome_init()) != PJ_SUCCESS) {
        pjsua_perror(__FUNCTION__, "pjsip_nat64_init", status);
    }
}
