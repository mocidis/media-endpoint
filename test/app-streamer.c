#include "my-pjlib-utils.h"
#include "my-pjmedia-utils.h"
#include "endpoint.h"

endpoint_t streamer;

int main() {
    char temp[10];
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjmedia_endpt *ep;
    pjmedia_stream *stream;
    pjmedia_codec_info *ci;

    int lport = 2345;
    //int lport = -1;
    char *file = "NHP-mono.wav";
    //char *rhost = "239.1.0.1";
    char *rhost = "127.0.0.1";
    int rport = 4321;

    pj_init();

    pj_log_set_level(5);
    pj_caching_pool_init(&cp, NULL, 1024);
    pool = pj_pool_create(&cp.factory, "pool1", 1024, 1024, NULL);
    pjmedia_endpt_create(&cp.factory, NULL, 1, &ep);
    pjmedia_codec_g711_init(ep);

    streamer_init(&streamer, ep, pool);
    streamer_config_dev_source(&streamer, -1);
    streamer_config_stream(&streamer, lport, rhost, rport);
    streamer_start(&streamer);
    
    while(1) {
        fprintf(stdout, "s=Stop - r=Resume: ");
        fflush(stdout);
        if (fgets(temp, sizeof(temp), stdin) == NULL)
            exit(-1);
        fprintf(stdout, "%s\n",temp);
        switch(temp[0]) {
        case 's':
        case 'S':
            streamer_stop(&streamer);
            break;
        case 'r':
        case 'R':
            streamer_start(&streamer);
            break;
        case '+':
            rport++;
            streamer_stop(&streamer);
            streamer_config_stream(&streamer, lport, rhost, rport);
            streamer_start(&streamer);
            break;
        case '-':
            rport--;
            streamer_stop(&streamer);
            streamer_config_stream(&streamer, lport, rhost, rport);
            streamer_start(&streamer);
            break;
        case 'v':
            streamer_update_stats(&streamer);
            fprintf(stdout, "rtt:%d - delay:%d - pkt:%d - lost: %d - discard:%d\n",
                            streamer.delay.mean_rtt_us, streamer.delay.mean_delay_ms,
                            streamer.drop.pkt, streamer.drop.lost, streamer.drop.discard);
            break;
        }
        pj_thread_sleep(5*100);
    }

    return 0;
}
