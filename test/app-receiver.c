#include "my-pjlib-utils.h"
#include "my-pjmedia-utils.h"
#include "endpoint.h"

endpoint_t receiver;

int main() {
    char temp[10];
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjmedia_endpt *ep;
    pjmedia_stream *stream;
    pjmedia_codec_info *ci;

    int lport = 4321;
    char *file = "NHP-mono.wav";
    char *mcast = "239.1.0.1";

    pj_init();

    pj_log_set_level(5);
    pj_caching_pool_init(&cp, NULL, 1024);
    pool = pj_pool_create(&cp.factory, "pool1", 1024, 1024, NULL);
    pjmedia_endpt_create(&cp.factory, NULL, 1, &ep);
    pjmedia_codec_g711_init(ep);

    //receiver_init(&receiver, ep, pool);
    receiver_init(&receiver, ep, pool, 2);
    receiver_config_dev_sink(&receiver, 1);
    receiver_config_stream(&receiver, mcast, lport);
    receiver_start(&receiver);

    while(1) {
        fprintf(stdout, "s=Stop - r=Resume: ");
        fflush(stdout);
        fgets(temp, sizeof(temp), stdin);
        fprintf(stdout, "%s\n",temp);
        switch(temp[0]) {
        case 's':
        case 'S':
            receiver_stop(&receiver);
            break;
        case 'r':
        case 'R':
            receiver_start(&receiver);
            break;
        case '+':
            lport++;
            receiver_stop(&receiver);
            receiver_config_stream(&receiver, mcast, lport);
            receiver_start(&receiver);
            break;
        case '-':
            lport--;
            receiver_stop(&receiver);
            receiver_config_stream(&receiver, mcast, lport);
            receiver_start(&receiver);
            break;
        case 'v':
            receiver_update_stats(&receiver);
            fprintf(stdout, "rtt:%d - delay:%d - pkt:%d - lost: %d - discard:%d\n",
                            receiver.delay.mean_rtt_us, receiver.delay.mean_delay_ms,
                            receiver.drop.pkt, receiver.drop.lost, receiver.drop.discard);
            break;
        }
        pj_thread_sleep(5*100);
    }

    return 0;
}
