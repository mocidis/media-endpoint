#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjlib.h>
typedef enum streamer_type_e {
    EPT_UNKNOWN = -1,
    EPT_FILE = 1,
    EPT_DEV = 2
} endpoint_type;

typedef enum streamer_state_e {
    EPS_UNINIT = -1,
    EPS_START = 1,
    EPS_STOP = 2
} endpoint_state;

typedef struct endpoint_stream_s {
    unsigned slot;
    pjmedia_stream *stream;
    struct {
        int mean_rtt_us;
        int max_rtt_us;
        int min_rtt_us;
        int mean_delay_ms;
        int max_delay_ms;
        int min_delay_ms;
    } delay;
    struct {
        unsigned int pkt;
        unsigned lost;
        unsigned discard;
    } drop;
} endpoint_stream_t;

typedef struct endpoint_s {
    endpoint_type type;
    endpoint_state state;
    pjmedia_endpt *ep;
    pj_pool_t *pool;
    pjmedia_codec_info *ci;
    
    int nstreams;
    endpoint_stream_t *streams;

    union {
        struct {
            pjmedia_conf *conf;
            union {
                pjmedia_master_port *mport;
                pjmedia_snd_port *snd_port;
            };
        } aout;
        union {
            pjmedia_master_port *mport;
            pjmedia_snd_port *snd_port;
        } ain;
    };
} endpoint_t;

void streamer_init(endpoint_t *streamer, pjmedia_endpt *ep, pj_pool_t *pool);
pj_status_t streamer_config_stream(endpoint_t *streamer, int lport, char *rhost, int rport);
void streamer_config_file_source(endpoint_t *streamer, char *file_name);
void streamer_config_dev_source(endpoint_t *streamer, int idx);
void streamer_start(endpoint_t *streamer);
void streamer_update_stats(endpoint_t *streamer);
void streamer_stop(endpoint_t *streamer);

void receiver_init(endpoint_t *receiver, pjmedia_endpt *ep, pj_pool_t *pool, int nchans);
//pj_status_t receiver_config_stream(endpoint_t *receiver, char *mcast, int lport, int idx);
pj_status_t receiver_config_stream(endpoint_t *receiver, char *mcast, int lport, int idx);
void receiver_config_file_sink(endpoint_t *receiver, char *file_name);
void receiver_config_dev_sink(endpoint_t *receiver, int idx);
void receiver_start(endpoint_t *receiver);
void receiver_update_stats(endpoint_t *receiver);
void receiver_stop(endpoint_t *receiver, int idx);
void receiver_adjust_volume(endpoint_t *receiver, int stream_idx, int incremental);
void receiver_adjust_master_volume(endpoint_t *receiver, int incremental);
void receiver_reset_volume(endpoint_t *receiver);
void receiver_dump_streams(endpoint_t *receiver);
#endif
