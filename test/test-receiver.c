#include "my-pjlib-utils.h"
#include "my-pjmedia-utils.h"
#include "endpoint.h"

endpoint_t receiver;

int usage(char *app) {
    SHOW_LOG(1, "%s <device_idx>\n", app);  
    exit(-1);
}

int main(int argc, char *argv[]) {
    char temp[10];
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjmedia_endpt *ep;
    pjmedia_stream *stream;
    pjmedia_codec_info *ci;

    if (argc < 2) {
        usage(argv[0]);
    }

    int lport = 4321;
    char *mcast = "237.1.0.1";

    int dev_idx, vol;

    pj_init();

    pj_log_set_level(2);
    pj_caching_pool_init(&cp, NULL, 1024);
    pool = pj_pool_create(&cp.factory, "pool1", 1024, 1024, NULL);
    pjmedia_endpt_create(&cp.factory, NULL, 1, &ep);
    pjmedia_codec_g711_init(ep);

    receiver_init(&receiver, ep, pool, 2);
    receiver_config_stream(&receiver, mcast, lport, 0);

    dev_idx =atoi(argv[1]);
    fprintf(stdout, "=========== Using device idx:%d ===========\n",dev_idx);

    receiver_config_dev_sink(&receiver, dev_idx);
    receiver_start(&receiver);

    while(1) {
        fprintf(stdout, "s=Stop - r=Resume:\n");
        fflush(stdout);
        if (fgets(temp, sizeof(temp), stdin) == NULL)
            exit(-1);
        fprintf(stdout, "%s\n",temp);
        switch(temp[0]) {
            case 'm':
            dev_idx = atoi(&temp[2]);
            vol = atoi(&temp[4]);
            vol = (vol * 256)/100 - 128;

            printf("vol = %d, idx = %d\n", vol, dev_idx);           
            receiver_adjust_volume(&receiver, dev_idx, vol);
            break;
        case 'd':
            receiver_dump_streams(&receiver);
            break;
        case 'v':
            vol = atoi(&temp[4]);
            vol = (vol * 256)/100 - 128;

            printf("vol = %d\n", vol);           
            receiver_adjust_master_volume(&receiver, vol);
            break;
        }

        pj_thread_sleep(5*100);
    }

    return 0;
}
