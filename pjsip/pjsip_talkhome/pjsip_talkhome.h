//
//  pjsip_talkhome.h
//  TalkHome
//
//  Created by Nicolas Dowell on 2016-02-25.
//  Copyright © 2016 Nowtel Management Ltd. All rights reserved.
//

#include <pj/config_site.h>
#include <pjsua-lib/pjsua.h>

pj_status_t pjsip_talkhome_init(void);

pjsip_transport *pjsip_talkhome_get_transport();

void pjsip_talkhome_shutdown_transport();
