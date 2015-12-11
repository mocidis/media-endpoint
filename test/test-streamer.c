#include "my-pjlib-utils.h"
#include "my-pjmedia-utils.h"
#include "endpoint.h"

endpoint_t streamer;

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

    int rport = 4321;
    char *rhost = "239.1.0.1";
    //char *rhost = "192.168.2.50";

    char *file = argv[2];
    int lport = 2345;
  
    int dev_idx;

    pj_init();

    pj_log_set_level(5);
    pj_caching_pool_init(&cp, NULL, 1024);
    streamer.pool = pj_pool_create(&cp.factory, "pool1", 1024, 1024, NULL);
    pjmedia_endpt_create(&cp.factory, NULL, 1, &streamer.ep);
    pjmedia_codec_g711_init(streamer.ep);

    streamer_init(&streamer, streamer.ep, streamer.pool);
    streamer_config_stream(&streamer, lport, rhost, rport);

    dev_idx =atoi(argv[1]);
    fprintf(stdout, "=========== Using device idx:%d ===========\n",dev_idx);

    streamer_config_dev_source(&streamer, dev_idx);
    streamer_start(&streamer);

    while(1) {
        pj_thread_sleep(5*100);
    }

    return 0;
}
