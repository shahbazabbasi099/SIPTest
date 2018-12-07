//
//  pjsip_talkhome.c
//  TalkHome
//
//  Created by Nicolas Dowell on 2016-02-25.
//  Copyright © 2016 Nowtel Management Ltd. All rights reserved.
//

#include "pjsip_talkhome.h"

#include <pjsua-lib/pjsua.h>

#define THIS_FILE "pjsip_talkhome.c"


static pj_bool_t   on_rx(pjsip_rx_data *);
static pj_status_t on_tx(pjsip_tx_data *);

static pjsip_module module = {
    .name = { "talkhome", 8 },
    .id = -1,
    .priority = 0,
    .on_rx_request = &on_rx,
    .on_rx_response = &on_rx,
    .on_tx_request = on_tx,
    .on_tx_response = on_tx,
};

static char s_external_addr_buffer[80];
static pj_str_t s_external_addr = { s_external_addr_buffer, 0 };
static pj_str_t s_external_addr_type = { NULL, 0 };
static pj_uint16_t s_external_af;
static pjsip_transport *s_transport;


pj_status_t pjsip_talkhome_init(void)
{
    return pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &module);
}

pjsip_transport *pjsip_talkhome_get_transport()
{
    return s_transport;
}

void pjsip_talkhome_shutdown_transport()
{
    if (!s_transport)
        return;
    
    //
    // DO NOT shutdown a UDP transport because pjsip will NOT recreate one
    //
    if (s_transport->key.type == PJSIP_TRANSPORT_TCP ||
        s_transport->key.type == PJSIP_TRANSPORT_TLS ||
        s_transport->key.type == PJSIP_TRANSPORT_TCP6 ||
        s_transport->key.type == PJSIP_TRANSPORT_TLS6) {
        
        PJ_LOG(3, (THIS_FILE,
                   "%s transport %.*s:%d is being asked to shut down...",
                   (s_transport->key.type == PJSIP_TRANSPORT_TLS ||
                    s_transport->key.type == PJSIP_TRANSPORT_TLS6) ? "TLS" : "TCP",
                   (int)s_transport->local_name.host.slen,
                   s_transport->local_name.host.ptr,
                   s_transport->local_name.port));

        pj_status_t status = pjsip_transport_shutdown(s_transport);
        if (status != PJ_SUCCESS) {
            PJ_LOG(2, (THIS_FILE, "pjsip_transport_shutdown failed"));
        }
        if (pj_atomic_get(s_transport->ref_cnt) > 0) {
            pjsip_transport_dec_ref(s_transport);
        }
        if (pj_atomic_get(s_transport->ref_cnt) == 0) {
            pjsip_transport_destroy(s_transport);
        }
    }
    
    s_transport = NULL;
}

static pj_bool_t is_nat64(pjsip_transport *transport)
{
    //
    // instead of tracking our public IP address, we could simply check for the
    // well-known prefix "64:ff9b::" but this would not cater for IPv4/IPv6
    // translators using different schemes.
    // https://tools.ietf.org/html/rfc6052#section-2.1
    //
    return PJ_AF_INET == s_external_af && PJ_AF_INET6 == transport->local_addr.addr.sa_family;
}

static pj_bool_t msg_body_contains_sdp(pjsip_msg_body *body)
{
    if (!body)
        return PJ_FALSE;
    if (pj_stricmp(&body->content_type.type, &(pj_str_t){ "application", 11 }) != 0)
        return PJ_FALSE;
    if (pj_stricmp(&body->content_type.subtype, &(pj_str_t){ "sdp", 3 }) != 0)
        return PJ_FALSE;
    return PJ_TRUE;
}

static void set_public_address(pj_str_t *str)
{
    pj_bzero(s_external_addr_buffer, sizeof(s_external_addr_buffer));
    pj_strncpy(&s_external_addr, str, sizeof(s_external_addr_buffer));
    pj_cstr(&s_external_addr_type, NULL);
    s_external_af = 0;

    char dummy[16];
    if (pj_inet_pton(PJ_AF_INET, &s_external_addr, dummy) == PJ_SUCCESS) {
        s_external_af = PJ_AF_INET;
        pj_cstr(&s_external_addr_type, "IP4");
    } else if (pj_inet_pton(PJ_AF_INET6, &s_external_addr, dummy) == PJ_SUCCESS) {
        s_external_af = PJ_AF_INET6;
        pj_cstr(&s_external_addr_type, "IP6");
    }
}

static void digitalk_port_workaround(pjsip_rx_data *rdata)
{
    pjsip_contact_hdr *contact = pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CONTACT, NULL);
    if (!contact) {
        PJ_LOG(4, (THIS_FILE, "%s contains no contact header, skipping",
                   rdata->msg_info.info));
        return;
    }
    
    if (!PJSIP_URI_SCHEME_IS_SIP(contact->uri) && !PJSIP_URI_SCHEME_IS_SIPS(contact->uri)) {
        PJ_LOG(2, (THIS_FILE, "%s contact URI is not SIP or SIPS",
                   rdata->msg_info.info));
        return;
    }
    
    pjsip_name_addr *name_addr = (pjsip_name_addr *)contact->uri;
    pjsip_sip_uri *sip_uri = (pjsip_sip_uri *)name_addr->uri;
    if (sip_uri->port == rdata->tp_info.transport->remote_name.port) {
        return;
    }
    
    /* this fixes the incorrect port number sent by DIGITALK */
    PJ_LOG(4, (THIS_FILE, "%s changing contact header port number from %hu to %hu",
               rdata->msg_info.info,
               sip_uri->port, rdata->tp_info.transport->remote_name.port));
    sip_uri->port = rdata->tp_info.transport->remote_name.port;
}

static pj_bool_t on_rx(pjsip_rx_data *rdata)
{
    if (rdata->msg_info.msg->type == PJSIP_RESPONSE_MSG && rdata->msg_info.via && rdata->msg_info.via->recvd_param.slen) {
        set_public_address(&rdata->msg_info.via->recvd_param);
    }
    
    digitalk_port_workaround(rdata);
    
    pjsip_contact_hdr *contact = pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CONTACT, NULL);
    if (!contact || (!PJSIP_URI_SCHEME_IS_SIP(contact->uri) && !PJSIP_URI_SCHEME_IS_SIPS(contact->uri))) {
        return PJ_FALSE;
    }

    pjsip_name_addr *name_addr = (pjsip_name_addr *)contact->uri;
    pjsip_sip_uri *sip_uri = (pjsip_sip_uri *)name_addr->uri;
    pj_str_t sip_server = sip_uri->host;

    if (is_nat64(rdata->tp_info.transport) && msg_body_contains_sdp(rdata->msg_info.msg->body)) {
        pjsip_rdata_sdp_info *sdp_info = pjsip_rdata_get_sdp_info(rdata);
        if (!sdp_info || !sdp_info->body.ptr) {
            return PJ_FALSE;
        }

        /* replace the IP4 addresses with IP6 equivalents so that RTP and RTCP can be sent */
        
        if (pj_strcmp(&sdp_info->sdp->origin.addr, &sip_server) == 0) {
            sdp_info->sdp->origin.addr_type = pj_str("IP6");
            sdp_info->sdp->origin.addr = rdata->tp_info.transport->remote_name.host;
        }
        
        if (pj_strcmp(&sdp_info->sdp->conn->addr, &sip_server) == 0) {
            sdp_info->sdp->conn->addr_type = pj_str("IP6");
            sdp_info->sdp->conn->addr = rdata->tp_info.transport->remote_name.host;
        }
    }
    
    /* the contact's host must be resolvable for ACK / PRACK and other messages to be sent successfully */
    sip_uri->host = rdata->tp_info.transport->remote_name.host;
    

    
    PJ_LOG(4, (THIS_FILE, "name address = %s",
               name_addr));
    
    PJ_LOG(4, (THIS_FILE, "sip address = %s",
               sip_server));
    
    
    PJ_LOG(4, (THIS_FILE, "host address = %s",
              sip_uri->host.ptr));
    
    return PJ_FALSE;
}

static pj_status_t on_tx(pjsip_tx_data *tdata)
{
    
    if (tdata->tp_info.transport && s_transport != tdata->tp_info.transport) {
        if (s_transport) {
            pjsip_transport_dec_ref(s_transport);
        }
        s_transport = tdata->tp_info.transport;
        pjsip_transport_add_ref(s_transport);
    }
    
    if (!tdata->msg) {
        return PJ_SUCCESS;
    }
    
    if (msg_body_contains_sdp(tdata->msg->body)) {
        if (!s_external_addr.slen || !s_external_addr_type.slen) {
            PJ_LOG(2, (THIS_FILE, "external address unknown, cannot fix outgoing SDP"));
            return PJ_SUCCESS;
        }
        
        /* This fixes an issue where answering a call over NAT64.
           Having an IPv6 "received" parameter was upsetting DIGITALK */
        pjsip_via_hdr *via = pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
        if (via) {
            via->recvd_param = via->sent_by.host;
        }
        
        pjsip_msg_body *new_body = pjsip_msg_body_clone(tdata->pool, tdata->msg->body);
        pjmedia_sdp_session *sdp = new_body->data;
        
        /* IP6 addresses in the SDP offer result in no RTP data being sent, so replace with our public IP4 address */
        
        // o=
        sdp->origin.addr = s_external_addr;
        sdp->origin.addr_type = s_external_addr_type;
        
        // c=
        if (sdp->conn && sdp->conn->addr.slen) {
            sdp->conn->addr = s_external_addr;
            sdp->conn->addr_type = s_external_addr_type;
        }
        
        unsigned media_index;
        for (media_index = 0; media_index < sdp->media_count; media_index++) {
            pjmedia_sdp_media *m = sdp->media[media_index];
            if (pj_strcmp(&m->conn->net_type, &(pj_str_t){ "IN", 2 }) != 0) {
                continue;
            }
            
            // c=
            if (m->conn && m->conn->addr.slen) {
                m->conn->addr = s_external_addr;
                m->conn->addr_type = s_external_addr_type;
            }
            
            // a=rtcp
            pjmedia_sdp_rtcp_attr rtcp = {0};
            pjmedia_sdp_attr *attr = pjmedia_sdp_attr_find(m->attr_count, m->attr, &(pj_str_t){ "rtcp", 4 }, NULL);
            if (attr && pjmedia_sdp_attr_get_rtcp(attr, &rtcp) == PJ_SUCCESS) {
                attr->value.slen = sprintf((attr->value.ptr = pj_pool_alloc(tdata->pool, 120)), "%u IN %.*s %.*s", rtcp.port,
                                           (int)s_external_addr_type.slen, s_external_addr_type.ptr,
                                           (int)s_external_addr.slen, s_external_addr.ptr);
            }
        }
        
        tdata->msg->body = new_body;
        pjsip_tx_data_invalidate_msg(tdata);
        pjsip_tx_data_encode(tdata);
    }
    
    return PJ_SUCCESS;
}
