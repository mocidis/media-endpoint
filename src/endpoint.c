#include "endpoint.h"
#include "ansi-utils.h"
#include "my-pjlib-utils.h"
static pj_status_t create_mstream(pj_pool_t *pool, 
                pjmedia_endpt *endpt, 
                const pjmedia_codec_info *codec_info, 
                pjmedia_dir dir, 
                int local_port,
                const char *remote_addr,
                int remote_port,
                unsigned vad,
                unsigned plc,
                pjmedia_stream **stream) 
{
    pjmedia_stream_info stream_info;
    pjmedia_codec_param codec_param;

    pjmedia_transport *transport = NULL;

    pjmedia_codec_mgr *cmgr = pjmedia_endpt_get_codec_mgr(endpt);

    pj_bzero(&stream_info, sizeof(stream_info));

    stream_info.type = PJMEDIA_TYPE_AUDIO;
    stream_info.dir = dir;
    pj_memcpy(&stream_info.fmt, codec_info, sizeof(pjmedia_codec_info));
    stream_info.tx_pt = codec_info->pt;
    stream_info.ssrc = pj_rand();

    pjmedia_codec_mgr_get_default_param(cmgr, codec_info, &codec_param);
    codec_param.setting.vad = vad;
    codec_param.setting.plc = plc;
    PJ_LOG(4, (__FILE__, "+++fpp=%d-vad=%d-cng=%d-plc=%d", \
                                codec_param.setting.frm_per_pkt,\
                                codec_param.setting.vad,\
                                codec_param.setting.cng,\
                                codec_param.setting.plc));
    stream_info.param = &codec_param;

    pjmedia_sock_info si;

    si.rtp_addr_name.addr.sa_family = PJ_AF_INET;
    si.rtcp_addr_name.addr.sa_family = PJ_AF_INET;

    
    CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &si.rtp_sock));
    CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &si.rtcp_sock));
    if( local_port > 0 ) {
        setup_addr_with_port(&si.rtp_addr_name.ipv4, local_port);
        CHECK(__FILE__, pj_sock_bind(si.rtp_sock, &si.rtp_addr_name.ipv4, sizeof(si.rtp_addr_name.ipv4)));
        setup_addr_with_port(&si.rtcp_addr_name.ipv4, local_port+1);
        CHECK(__FILE__, pj_sock_bind(si.rtcp_sock, &si.rtcp_addr_name.ipv4, sizeof(si.rtcp_addr_name.ipv4)));
    }
    //setup_udp_socket(local_port, &si.rtp_sock, &si.rtp_addr_name.ipv4);
    //setup_udp_socket(local_port + 1, &si.rtcp_sock, &si.rtcp_addr_name.ipv4);

    PJ_LOG(3, (__FILE__, "What"));
    if(dir == PJMEDIA_DIR_DECODING) {
        setup_addr_with_host_and_port(
             &(stream_info.rem_addr.ipv4), 
             "0.0.0.0", 0
        );
        if( remote_addr != NULL ) {
            CHECK(__FILE__, join_mcast_group(si.rtp_sock, remote_addr));
            CHECK(__FILE__, join_mcast_group(si.rtcp_sock, remote_addr));
        }
    }
    else if(dir == PJMEDIA_DIR_ENCODING) {
        setup_addr_with_host_and_port(
             &(stream_info.rem_addr.ipv4), 
             remote_addr, remote_port
        );
    }

    CHECK_R(__FILE__, pjmedia_transport_udp_attach(endpt, NULL, &si, 0, &transport));
    CHECK_R(__FILE__, pjmedia_stream_create(endpt, pool, &stream_info, transport, NULL, stream));
    return PJ_SUCCESS;
}
static pj_status_t ostream_create(pj_pool_t *pool, 
                pjmedia_endpt *endpt, 
                const pjmedia_codec_info *codec_info, 
                int local_port,
                const char *remote_addr,
                int remote_port,
                pjmedia_stream **stream) 
{
    return create_mstream(pool, endpt, codec_info, PJMEDIA_DIR_ENCODING, local_port, remote_addr, remote_port, 0, 1, stream);
}

static pj_status_t mistream_create(pj_pool_t *pool, pjmedia_endpt *endpt, 
                                   const pjmedia_codec_info *ci, int lport,
                                   char *mcast, pjmedia_stream **stream) 
{
    return create_mstream(pool, endpt, ci, PJMEDIA_DIR_DECODING, lport, mcast, 0, 1, 1, stream);
}

static void endpoint_init(endpoint_t *eep, pjmedia_endpt *ep, pj_pool_t *pool) {
    pjmedia_codec_mgr *cmgr;
    
    pj_bzero(eep, sizeof(endpoint_t));

    eep->ep = ep;
    eep->pool = pool;
    eep->stream.stream = NULL;
    eep->type = EPT_UNKNOWN;
    eep->state = EPS_UNINIT;

    cmgr = pjmedia_endpt_get_codec_mgr(eep->ep);
    CHECK_NULL(__FILE__, cmgr);
    CHECK(__FILE__, pjmedia_codec_mgr_get_codec_info(cmgr, PJMEDIA_RTP_PT_PCMU, 
                                                (const pjmedia_codec_info **)&eep->ci));

}

void streamer_init(endpoint_t *streamer, pjmedia_endpt *ep, pj_pool_t *pool) {
    endpoint_init(streamer, ep, pool);
}

pj_status_t streamer_config_stream(endpoint_t *streamer, int lport, char *rhost, int rport) {
    pjmedia_transport *tp;
    if(streamer->stream.stream != NULL) {
        tp = pjmedia_stream_get_transport(streamer->stream.stream);
        pjmedia_stream_destroy(streamer->stream.stream);
        pjmedia_transport_close(tp);
        streamer->stream.stream = NULL;
    }
    CHECK_R(__FILE__, ostream_create(streamer->pool, streamer->ep, streamer->ci, \
                                      lport, rhost, rport, &streamer->stream.stream));
    CHECK(__FILE__, pjmedia_stream_start(streamer->stream.stream));
    return PJ_SUCCESS;
}
void streamer_config_file_source(endpoint_t *streamer, char *file_name) {
    pjmedia_port *port, *fport;
    
    CHECK(__FILE__, pjmedia_wav_player_port_create(streamer->pool, file_name, 0, 0, -1, &fport));

    if(streamer->stream.stream != NULL) {
        CHECK(__FILE__, pjmedia_stream_get_port(streamer->stream.stream, &port));
        CHECK(__FILE__, pjmedia_master_port_create(streamer->pool,
                                fport, 
                                port,
                                0,
                                &streamer->ain.mport));
    }
    streamer->type = EPT_FILE;
    streamer->state = EPS_STOP;
}

void streamer_config_dev_source(endpoint_t *streamer, int idx) {
    CHECK(__FILE__, pjmedia_snd_port_create_rec(streamer->pool, idx, 
                   streamer->ci->clock_rate,
                   streamer->ci->channel_cnt,
                   streamer->ci->clock_rate * streamer->ci->channel_cnt * 20 / 1000,
                   16,
                   0, &streamer->ain.snd_port));

    streamer->type = EPT_DEV;
    streamer->state = EPS_STOP;
}

void streamer_start(endpoint_t *streamer) {
    pjmedia_port *port;
    PJ_LOG(3, (__FILE__, "Streamer Start"));
    CHECK_NULL(__FILE__, streamer->stream.stream);
    if( streamer->state == EPS_STOP ) {
        switch( streamer->type ) {
        case EPT_FILE:
            PJ_LOG(3, (__FILE__, "Start stream file"));
            CHECK(__FILE__, pjmedia_master_port_start(streamer->ain.mport));
            streamer->state = EPS_START;
            break;
        case EPT_DEV:
            PJ_LOG(3, (__FILE__, "Start stream from sound dev"));
            CHECK(__FILE__, pjmedia_stream_get_port(streamer->stream.stream, &port));
            CHECK(__FILE__, pjmedia_snd_port_connect(streamer->ain.snd_port, port));
            streamer->state = EPS_START;
            break;
        default:
            PJ_LOG(3, (__FILE__, "Unknown endpoint type"));
            break;
        }
    }
    else {
        PJ_LOG(3, (__FILE__, "Not start"));
    }
}

void streamer_update_stats(endpoint_t *streamer) {
    pjmedia_rtcp_stat stat;
    pjmedia_jb_state jbstate;
    pjmedia_stream_get_stat(streamer->stream.stream, &stat);
    pjmedia_stream_get_stat_jbuf(streamer->stream.stream, &jbstate);
    streamer->stream.delay.mean_rtt_us = stat.rtt.mean;
    streamer->stream.delay.max_rtt_us = stat.rtt.max;
    streamer->stream.delay.min_rtt_us = stat.rtt.min;
    streamer->stream.delay.mean_delay_ms = jbstate.avg_delay;
    streamer->stream.delay.min_delay_ms = jbstate.min_delay;
    streamer->stream.delay.max_delay_ms = jbstate.max_delay;
    streamer->stream.drop.lost = stat.tx.loss;
    streamer->stream.drop.discard = stat.tx.discard;
    streamer->stream.drop.pkt = stat.tx.pkt;
}

void streamer_stop(endpoint_t *streamer) {
    PJ_LOG(3, (__FILE__, "Stop"));
    if( streamer->state == EPS_START) {
        switch(streamer->type) {
        case EPT_FILE:
            CHECK(__FILE__, pjmedia_master_port_stop(streamer->ain.mport));
            streamer->state = EPS_STOP;
            break;
        case EPT_DEV:
            CHECK(__FILE__, pjmedia_snd_port_disconnect(streamer->ain.snd_port));
            streamer->state = EPS_STOP;
            break;
        default:
            PJ_LOG(3, (__FILE__, "Unknown endpoint type"));
        }

    }
    else {
        PJ_LOG(3, (__FILE__, "Not start"));
    }
}

void receiver_init(endpoint_t *receiver, pjmedia_endpt *ep, pj_pool_t *pool, int nchans) {
    endpoint_init(receiver, ep, pool);
    CHECK(__FILE__, pjmedia_conf_create(pool, nchans,\
                   receiver->ci->clock_rate,\
                   receiver->ci->channel_cnt,\
                   receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,\
                   16, PJMEDIA_CONF_NO_DEVICE, &receiver->aout.conf));
}

pj_status_t receiver_config_stream(endpoint_t *receiver, char *mcast, int lport) {
    pjmedia_transport *tp;
    pjmedia_port *port;
    if(receiver->stream.stream != NULL) {
        tp = pjmedia_stream_get_transport(receiver->stream.stream);
        pjmedia_stream_destroy(receiver->stream.stream);
        pjmedia_transport_close(tp);
        receiver->stream.stream = NULL;
    }
    CHECK_R(__FILE__, mistream_create(receiver->pool, receiver->ep,\
                                       receiver->ci, lport, mcast, &receiver->stream.stream));

    CHECK(__FILE__, pjmedia_stream_start(receiver->stream.stream));
    CHECK(__FILE__, pjmedia_stream_get_port(receiver->stream.stream, &port));

    CHECK(__FILE__, pjmedia_conf_add_port(receiver->aout.conf, receiver->pool, port, NULL, &receiver->stream.slot)); // !!!
    pjmedia_conf_connect_port(receiver->aout.conf, receiver->stream.slot, 0, 0);

    return PJ_SUCCESS;
}

void receiver_config_file_sink(endpoint_t *receiver, char *file_name) {
    pjmedia_port *port, *fport;
    CHECK(__FILE__, pjmedia_wav_writer_port_create(receiver->pool, 
                                    file_name, 
                                    receiver->ci->clock_rate,
                                    receiver->ci->channel_cnt,
                                    receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,
                                    16,
                                    0, 0,
                                    &fport));

    port = pjmedia_conf_get_master_port(receiver->aout.conf);
    CHECK_NULL(__FILE__, port);

    CHECK(__FILE__, pjmedia_master_port_create(receiver->pool,
                            fport, 
                            port,
                            0,
                            &receiver->aout.mport));
    
    receiver->type = EPT_FILE;
    receiver->state = EPS_STOP;
}
void receiver_config_dev_sink(endpoint_t *receiver, int idx) {
    CHECK(__FILE__, pjmedia_snd_port_create_player(receiver->pool, idx,
                    receiver->ci->clock_rate,
                    receiver->ci->channel_cnt,
                    receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,
                    16,
                    0, &receiver->aout.snd_port));
    receiver->type = EPT_DEV;
    receiver->state = EPS_STOP;
}
void receiver_start(endpoint_t *receiver) {
    pjmedia_port *port;
    PJ_LOG(3, (__FILE__, "Receiver Start"));
    CHECK_NULL(__FILE__, receiver->stream.stream);
    if( receiver->state == EPS_STOP ) {
        switch (receiver->type) {
        case EPT_FILE:
            PJ_LOG(3, (__FILE__, "Start"));
            CHECK(__FILE__, pjmedia_master_port_start(receiver->aout.mport));
            receiver->state = EPS_START;
            break;
        case EPT_DEV:
            PJ_LOG(3, (__FILE__, "Start stream to sound dev"));

            port = pjmedia_conf_get_master_port(receiver->aout.conf);
            CHECK_NULL(__FILE__, port);
            CHECK(__FILE__, pjmedia_snd_port_connect(receiver->aout.snd_port, port));

            receiver->state = EPS_START;
            break;
        default:
            PJ_LOG(3, (__FILE__, "Unknown endpoint type"));
        }
    }
    else {
        PJ_LOG(3, (__FILE__, "Not start"));
    }
}
void receiver_update_stats(endpoint_t *receiver) {
    pjmedia_rtcp_stat stat;
    pjmedia_jb_state jbstate;
    pjmedia_stream_get_stat(receiver->stream.stream, &stat);
    pjmedia_stream_get_stat_jbuf(receiver->stream.stream, &jbstate);
    receiver->stream.delay.mean_rtt_us = stat.rtt.mean;
    receiver->stream.delay.max_rtt_us = stat.rtt.max;
    receiver->stream.delay.min_rtt_us = stat.rtt.min;
    receiver->stream.delay.mean_delay_ms = jbstate.avg_delay;
    receiver->stream.delay.min_delay_ms = jbstate.min_delay;
    receiver->stream.delay.max_delay_ms = jbstate.max_delay;
    receiver->stream.drop.lost = stat.rx.loss;
    receiver->stream.drop.discard = stat.rx.discard;
    receiver->stream.drop.pkt = stat.rx.pkt;
}
void receiver_stop(endpoint_t *receiver) {
    PJ_LOG(3, (__FILE__, "Stop"));
    if( receiver->state == EPS_START) {
        switch( receiver->type ) {
        case EPT_FILE:
            CHECK(__FILE__, pjmedia_master_port_stop(receiver->aout.mport));
            receiver->state = EPS_STOP;
            break;
        case EPT_DEV: 
            CHECK(__FILE__, pjmedia_snd_port_disconnect(receiver->aout.snd_port));
            receiver->state = EPS_STOP;
            break;
        default:
            PJ_LOG(3, (__FILE__, "Unknown endpoint type"));
        }
    }
    else {
        PJ_LOG(3, (__FILE__, "Not start"));
    }
}
