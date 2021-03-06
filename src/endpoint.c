#include "endpoint.h"
#include "ansi-utils.h"
#include "my-pjlib-utils.h"
static pj_status_t create_audio_splitter_stream( 
		pj_pool_t *pool, pjmedia_endpt *med_endpt, 
		const pjmedia_codec_info *codec_info, pjmedia_dir dir, 
		pjmedia_transport **transport, pjmedia_stream **p_stream ) {
	pjmedia_stream_info info;
	pj_status_t status;

	/* Reset stream info. */
	pj_bzero(&info, sizeof(info));

	/* Initialize stream info formats */
	info.type = PJMEDIA_TYPE_AUDIO;
	info.dir = dir;
	pj_memcpy(&info.fmt, codec_info, sizeof(pjmedia_codec_info));
	info.tx_pt = codec_info->pt;
	info.rx_pt = codec_info->pt;
	info.ssrc = pj_rand();

	if (info.rem_addr.addr.sa_family == 0) {
		const pj_str_t addr = pj_str("127.0.0.1");
		pj_sockaddr_in_init(&info.rem_addr.ipv4, &addr, 0);
	}
	if (*transport == NULL) {
		PJ_LOG(2, (__FILE__, "------------------------CREATE TRANSPORT LOOPBACK--------------------"));
		pjmedia_transport_loop_create(med_endpt, transport);
	} else {
		PJ_LOG(2, (__FILE__, "------------------------TRANSPORT LOOPBACK IS CREATED--------------------"));
	}
	/* Now that the stream info is initialized, we can create the 
	 * stream.
	 */

	status = pjmedia_stream_create( med_endpt, pool, &info, *transport, NULL, p_stream);

	if (status != PJ_SUCCESS) {
		PJ_LOG(2, (__FILE__, "------------------------FAILED--------------------"));
		pjmedia_transport_close(transport);
		return status;
	}
	return PJ_SUCCESS;
}
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


	ANSI_CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &si.rtp_sock));
	ANSI_CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &si.rtcp_sock));

	int optval = 1;
	ANSI_CHECK(__FILE__, pj_sock_setsockopt(si.rtp_sock, PJ_SOL_SOCKET, PJ_SO_REUSEADDR, &optval, sizeof(optval)));
	ANSI_CHECK(__FILE__, pj_sock_setsockopt(si.rtcp_sock, PJ_SOL_SOCKET, PJ_SO_REUSEADDR, &optval, sizeof(optval)));

	if( local_port > 0 ) {
		setup_addr_with_port(&si.rtp_addr_name.ipv4, local_port);
		ANSI_CHECK(__FILE__, pj_sock_bind(si.rtp_sock, &si.rtp_addr_name.ipv4, sizeof(si.rtp_addr_name.ipv4)));
		setup_addr_with_port(&si.rtcp_addr_name.ipv4, local_port+1);
		ANSI_CHECK(__FILE__, pj_sock_bind(si.rtcp_sock, &si.rtcp_addr_name.ipv4, sizeof(si.rtcp_addr_name.ipv4)));
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
			ANSI_CHECK(__FILE__, join_mcast_group(si.rtp_sock, remote_addr));
			ANSI_CHECK(__FILE__, join_mcast_group(si.rtcp_sock, remote_addr));
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
	SHOW_LOG(3, "Stream: %p\n", stream);
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
	return create_mstream(pool, endpt, codec_info, PJMEDIA_DIR_ENCODING, local_port, remote_addr, remote_port, 0, 0, stream);
}

static pj_status_t mistream_create(pj_pool_t *pool, pjmedia_endpt *endpt, 
		const pjmedia_codec_info *ci, int lport,
		char *mcast, pjmedia_stream **stream) 
{
	return create_mstream(pool, endpt, ci, PJMEDIA_DIR_DECODING, lport, mcast, 0, 0, 0, stream);
}

static void endpoint_init(endpoint_t *eep, pjmedia_endpt *ep, pj_pool_t *pool) {
	pjmedia_codec_mgr *cmgr;

	pj_bzero(eep, sizeof(endpoint_t));

	eep->ep = ep;
	eep->pool = pool;
	eep->type = EPT_UNKNOWN;
	eep->state = EPS_UNINIT;

	cmgr = pjmedia_endpt_get_codec_mgr(eep->ep);
	CHECK_NULL(__FILE__, cmgr);
	ANSI_CHECK(__FILE__, pjmedia_codec_mgr_get_codec_info(cmgr, PJMEDIA_RTP_PT_PCMU, 
				(const pjmedia_codec_info **)&eep->ci));

}

void streamer_init(endpoint_t *streamer, pjmedia_endpt *ep, pj_pool_t *pool) {
	int i=0;
	endpoint_init(streamer, ep, pool);
	streamer->nstreams = 1;
	streamer->streams = pj_pool_zalloc(streamer->pool, streamer->nstreams * sizeof(endpoint_stream_t));
	streamer->streams[0].stream = NULL;
}

pj_status_t streamer_config_stream(endpoint_t *streamer, int lport, char *rhost, int rport) {
	pjmedia_transport *tp;
	if(streamer->streams[0].stream != NULL) {
		tp = pjmedia_stream_get_transport(streamer->streams[0].stream);
		pjmedia_stream_destroy(streamer->streams[0].stream);
		pjmedia_transport_close(tp);
		streamer->streams[0].stream = NULL;
	}
	CHECK_R(__FILE__, ostream_create(streamer->pool, streamer->ep, streamer->ci, \
				lport, rhost, rport, &streamer->streams[0].stream));
	ANSI_CHECK(__FILE__, pjmedia_stream_start(streamer->streams[0].stream));
	return PJ_SUCCESS;
}
void streamer_config_file_source(endpoint_t *streamer, char *file_name) {
	pjmedia_port *port, *fport;

	PJ_LOG(3, (__FILE__, "File name: %s\n", file_name));    

	ANSI_CHECK(__FILE__, pjmedia_wav_player_port_create(streamer->pool, file_name, 0, 0, -1, &fport));

	PJ_LOG(3,(__FILE__, "File port = %p\n", fport));

	if(streamer->streams[0].stream != NULL) {
		PJ_LOG(3, (__FILE__, "=======Stream not NULL======="));
		ANSI_CHECK(__FILE__, pjmedia_stream_get_port(streamer->streams[0].stream, &port));
		ANSI_CHECK(__FILE__, pjmedia_master_port_create(streamer->pool,
					fport, 
					port,
					0,
					&streamer->ain.mport));
		PJ_LOG(3, (__FILE__, "Master port: %p", streamer->ain.mport));
	}
	streamer->type = EPT_FILE;
	streamer->state = EPS_STOP;
}

void streamer_config_dev_source(endpoint_t *streamer, int idx) {
	ANSI_CHECK(__FILE__, pjmedia_snd_port_create_rec(streamer->pool, idx, 
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
	CHECK_NULL(__FILE__, streamer->streams[0].stream);
	if( streamer->state == EPS_STOP ) {
		switch( streamer->type ) {
			case EPT_FILE:
				PJ_LOG(3, (__FILE__, "Start stream file"));
				ANSI_CHECK(__FILE__, pjmedia_master_port_start(streamer->ain.mport));
				streamer->state = EPS_START;
				break;
			case EPT_DEV:
				PJ_LOG(3, (__FILE__, "Start stream from sound dev"));
				ANSI_CHECK(__FILE__, pjmedia_stream_get_port(streamer->streams[0].stream, &port));
				ANSI_CHECK(__FILE__, pjmedia_snd_port_connect(streamer->ain.snd_port, port));
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

static void endpoint_stream_update_stats(endpoint_stream_t *stream) {
	pjmedia_rtcp_stat stat;
	pjmedia_jb_state jbstate;
	pjmedia_stream_get_stat(stream->stream, &stat);
	pjmedia_stream_get_stat_jbuf(stream->stream, &jbstate);
	stream->delay.mean_rtt_us = stat.rtt.mean;
	stream->delay.max_rtt_us = stat.rtt.max;
	stream->delay.min_rtt_us = stat.rtt.min;
	stream->delay.mean_delay_ms = jbstate.avg_delay;
	stream->delay.min_delay_ms = jbstate.min_delay;
	stream->delay.max_delay_ms = jbstate.max_delay;
	stream->drop.lost = stat.rx.loss;
	stream->drop.discard = stat.rx.discard;
	stream->drop.pkt = stat.rx.pkt;
}


void streamer_update_stats(endpoint_t *streamer) {
	int i;
	for (i = 0; i < streamer->nstreams; i++) {
		endpoint_stream_update_stats(&streamer->streams[i]); 
	}
}

void streamer_stop(endpoint_t *streamer) {
	PJ_LOG(3, (__FILE__, "Stop"));
	if( streamer->state == EPS_START) {
		switch(streamer->type) {
			case EPT_FILE:
				ANSI_CHECK(__FILE__, pjmedia_master_port_stop(streamer->ain.mport));
				streamer->state = EPS_STOP;
				break;
			case EPT_DEV:
				ANSI_CHECK(__FILE__, pjmedia_snd_port_disconnect(streamer->ain.snd_port));
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
	int i;
	pj_status_t status;
	pj_str_t ip = pj_str(LOCAL_IP_SOUND);
	endpoint_init(receiver, ep, pool);

	receiver->nstreams = nchans - 1;
	receiver->streams = pj_pool_zalloc(receiver->pool, receiver->nstreams * sizeof(endpoint_stream_t));
	for (i = 0; i < nchans - 1 ; i++) {
		receiver->streams[i].stream = NULL;
	}

	ANSI_CHECK(__FILE__, pjmedia_conf_create(pool, nchans,\
				receiver->ci->clock_rate,\
				receiver->ci->channel_cnt,\
				receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,\
				16, PJMEDIA_CONF_NO_DEVICE, &receiver->aout.conf));
	ANSI_CHECK(__FILE__, pjmedia_conf_create(pool, nchans,\
				receiver->ci->clock_rate,\
				receiver->ci->channel_cnt,\
				receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,\
				16, PJMEDIA_CONF_NO_DEVICE, &receiver->aout.conf2));

	receiver->splitter = malloc(MAX_SOUND_DEVICE * sizeof(audio_splitter));
	for (i=0;i<MAX_SOUND_DEVICE;i++) {
		receiver->splitter->r_stream[i] = NULL;
		receiver->splitter->s_stream = NULL;
		receiver->splitter->r_stream_port[i] = NULL;
		receiver->splitter->s_stream_port = NULL;
		receiver->splitter->m_port[1] = NULL;
		receiver->splitter->m_port[2] = NULL;
		receiver->splitter->master_port = NULL;
		receiver->splitter->snd_port[i] = NULL;
		receiver->splitter->port_array[i] = -1;
		receiver->splitter->transport = NULL;
	}
	strcpy(receiver->splitter->play_file, "vnt.wav");
}

pj_status_t receiver_config_stream(endpoint_t *receiver, char *mcast, int lport, int i) {
	pjmedia_transport *tp;
	pjmedia_port *port;

	CHECK_FALSE(__FILE__, i >= 0 && i < receiver->nstreams);

	if(receiver->streams[i].stream != NULL) {
		tp = pjmedia_stream_get_transport(receiver->streams[i].stream);
		pjmedia_stream_destroy(receiver->streams[i].stream);
		pjmedia_conf_remove_port(receiver->aout.conf, receiver->streams[i].slot);
		pjmedia_transport_close(tp);
		receiver->streams[i].stream = NULL;
	}

	CHECK_R(__FILE__, mistream_create(receiver->pool, receiver->ep,\
				receiver->ci, lport, mcast, &receiver->streams[i].stream));

	ANSI_CHECK(__FILE__, pjmedia_stream_start(receiver->streams[i].stream));

	ANSI_CHECK(__FILE__, pjmedia_stream_get_port(receiver->streams[i].stream, &port));

	ANSI_CHECK(__FILE__, pjmedia_conf_add_port(receiver->aout.conf, receiver->pool, port, NULL, &receiver->streams[i].slot));
	pjmedia_conf_connect_port(receiver->aout.conf, receiver->streams[i].slot, 0, 0);

	PJ_LOG(1, (__FILE__,"======= CONFIG STREAM: (Stream idx: %d - Slot: %d) ======\n", i, receiver->streams[i].slot));
	return PJ_SUCCESS;
}

void receiver_config_file_sink(endpoint_t *receiver, char *file_name) {
	pjmedia_port *port, *fport;
	ANSI_CHECK(__FILE__, pjmedia_wav_writer_port_create(receiver->pool, 
				file_name, 
				receiver->ci->clock_rate,
				receiver->ci->channel_cnt,
				receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,
				16,
				0, 0,
				&fport));

	port = pjmedia_conf_get_master_port(receiver->aout.conf);
	CHECK_NULL(__FILE__, port);

	ANSI_CHECK(__FILE__, pjmedia_master_port_create(receiver->pool,
				fport, 
				port,
				0,
				&receiver->aout.mport));

	receiver->type = EPT_FILE;
	receiver->state = EPS_STOP;
}
void receiver_config_dev_sink(endpoint_t *receiver, int idx) {
	SHOW_LOG(3, "clock rate:%d, channel: %d, spf:%d, idx: %d\n", receiver->ci->clock_rate,
			receiver->ci->channel_cnt,
			receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000, idx);
	ANSI_CHECK(__FILE__, pjmedia_snd_port_create_player(receiver->pool, idx,
				receiver->ci->clock_rate,
				receiver->ci->channel_cnt,
				receiver->ci->clock_rate * receiver->ci->channel_cnt * 20 / 1000,
				16,
				0, &receiver->aout.snd_port));
	//set default output sound device
	receiver->splitter->port_array[0] = idx;
	receiver->type = EPT_DEV;
	receiver->state = EPS_STOP;
}
void receiver_start(endpoint_t *receiver) {
	pjmedia_port *port;
	PJ_LOG(2, (__FILE__, "Receiver Start"));
	int i;
	for (i = 0 ; i < receiver->nstreams; i++) {
		CHECK_NULL(__FILE__, receiver->streams[i].stream);
	}
	if( receiver->state == EPS_STOP ) {
		switch (receiver->type) {
			case EPT_FILE:
				PJ_LOG(2, (__FILE__, "Start"));
				ANSI_CHECK(__FILE__, pjmedia_master_port_start(receiver->aout.mport));
				receiver->state = EPS_START;
				break;
			case EPT_DEV:
				PJ_LOG(2, (__FILE__, "Start stream to sound dev"));

				port = pjmedia_conf_get_master_port(receiver->aout.conf);
				CHECK_NULL(__FILE__, port);
				ANSI_CHECK(__FILE__, pjmedia_snd_port_connect(receiver->aout.snd_port, port));

				receiver->state = EPS_START;
				break;
			default:
				PJ_LOG(2, (__FILE__, "Unknown endpoint type"));
		}
	}
	else {
		PJ_LOG(2, (__FILE__, "Not start"));
	}
}

void receiver_update_stats(endpoint_t *receiver) {
	int i;
	for (i = 0; i < receiver->nstreams; i++) {
		endpoint_stream_update_stats(&receiver->streams[i]); 
	}
}
void receiver_stop(endpoint_t *receiver, int i) {
	PJ_LOG(1, (__FILE__, "Stop"));
	pjmedia_transport *tp;

	if( receiver->state == EPS_START) {
		switch( receiver->type ) {
			case EPT_FILE:
				ANSI_CHECK(__FILE__, pjmedia_master_port_stop(receiver->aout.mport));
				receiver->state = EPS_STOP;
				break;
			case EPT_DEV: 
				if(receiver->streams[i].stream != NULL) {
					tp = pjmedia_stream_get_transport(receiver->streams[i].stream);
					pjmedia_stream_destroy(receiver->streams[i].stream);
					pjmedia_conf_remove_port(receiver->aout.conf, receiver->streams[i].slot);
					pjmedia_transport_close(tp);
					receiver->streams[i].stream = NULL;
				}
				//ANSI_CHECK(__FILE__, pjmedia_snd_port_disconnect(receiver->aout.snd_port));
				//receiver->state = EPS_STOP;
				break;
			default:
				PJ_LOG(3, (__FILE__, "Unknown endpoint type"));
		}
	}
	else {
		PJ_LOG(1, (__FILE__, "Not start"));
	}
}

static int receiver_volume_inc(pjmedia_snd_port *snd_port, int incremental) {
	pjmedia_aud_stream *aud_stream = pjmedia_snd_port_get_snd_stream(snd_port);
	if (aud_stream == NULL) return -1;
	unsigned vol;
	CHECK_R(__FILE__, pjmedia_aud_stream_get_cap(aud_stream, PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING, &vol));
	vol += incremental;
	vol = (vol > 100)?100:vol;
	//vol = (vol < 0)?0:vol;
	CHECK_R(__FILE__, pjmedia_aud_stream_set_cap(aud_stream, PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING, &vol));
	return vol;
}

void receiver_adjust_volume(endpoint_t *receiver, int stream_idx, int incremental) {
	incremental = (incremental < -128)?(-128):incremental;
	ANSI_CHECK(__FILE__, pjmedia_conf_adjust_rx_level(receiver->aout.conf, receiver->streams[stream_idx].slot, incremental));
}
void receiver_adjust_master_volume(endpoint_t *receiver, int incremental) {
	CHECK_FALSE(__FILE__, receiver->type == EPT_DEV);
	receiver_volume_inc(receiver->aout.snd_port, incremental);
}
void receiver_reset_volume(endpoint_t *receiver) {
	CHECK_FALSE(__FILE__, receiver->type == EPT_DEV);
	receiver_volume_inc(receiver->aout.snd_port, 50);
}

void receiver_dump_streams(endpoint_t *receiver) {
	int i;
	receiver_update_stats(receiver);
	for (i = 0; i < receiver->nstreams; i++) {
		PJ_LOG(1,(__FILE__, "SLOT: %d, pkt=%d\n", receiver->streams[i].slot, receiver->streams[i].drop.pkt)); 
	}
}
void receiver_splitter_stop(endpoint_t *receiver) {
	int i=0;
	pj_status_t status;
	for (i = 0; i < MAX_SOUND_DEVICE; i++) {
		if (receiver->splitter->snd_port[i] != NULL) {
			status = pjmedia_snd_port_disconnect(receiver->splitter->snd_port[i]);
			status = pjmedia_snd_port_destroy(receiver->splitter->snd_port[i]);
			receiver->splitter->snd_port[i] = NULL;
		}
	}
}
void receiver_splitter_start(endpoint_t *receiver) {
	pjmedia_port *port;
	if( receiver->state == EPS_STOP ) {
		PJ_LOG(2, (__FILE__, "Splitt audio stream to sound devices"));

		port = pjmedia_conf_get_master_port(receiver->aout.conf);
		CHECK_NULL(__FILE__, port);
		pj_status_t status;
		int i = 0;

		receiver->splitter->m_port[0] = pjmedia_conf_get_master_port(receiver->aout.conf);
		receiver->splitter->m_port[1] = pjmedia_conf_get_master_port(receiver->aout.conf2);
		//create master clock to controll flow internal audio stream
		status = pjmedia_master_port_create(receiver->pool, receiver->splitter->m_port[1], receiver->splitter->m_port[0], 0, &receiver->splitter->master_port);
		status = pjmedia_master_port_start(receiver->splitter->master_port);

		//testing
		/*status = pjmedia_wav_player_port_create(receiver->pool, receiver->splitter->play_file, 0, 0, 0, &receiver->splitter->play_file_port);
		pjmedia_conf_add_port(receiver->aout.conf, receiver->pool, receiver->splitter->play_file_port, NULL, &receiver->splitter->f_slot);
		status = pjmedia_conf_connect_port(receiver->aout.conf, receiver->splitter->f_slot, 0, 0);
		*/

		// end of testing

		//create 'send streams' and then get port's streams. (Only need a 'send stream')
		status = create_audio_splitter_stream(receiver->pool, receiver->ep, receiver->ci, PJMEDIA_DIR_ENCODING, &receiver->splitter->transport, &receiver->splitter->s_stream);
		status = pjmedia_stream_get_port(receiver->splitter->s_stream, &receiver->splitter->s_stream_port);
		pjmedia_conf_add_port(receiver->aout.conf2, receiver->pool, receiver->splitter->s_stream_port, NULL, &receiver->splitter->s_slot);
		pjmedia_conf_connect_port(receiver->aout.conf2, 0, receiver->splitter->s_slot, 0);
		for (i=0; i<MAX_SOUND_DEVICE; i++) {
			//create MAX_SOUND_DEVICE (4) 'receive streams' and then get (4) port's streams.
			status = create_audio_splitter_stream(receiver->pool, receiver->ep, receiver->ci, PJMEDIA_DIR_DECODING, &receiver->splitter->transport, &receiver->splitter->r_stream[i]);
			status = pjmedia_stream_get_port(receiver->splitter->r_stream[i], &receiver->splitter->r_stream_port[i]);
			if (receiver->splitter->port_array[i] >= 0) {
				status = pjmedia_snd_port_create_player(receiver->pool, receiver->splitter->port_array[i], 8000, 1, 160, 16, 0, &receiver->splitter->snd_port[i]);
				status = pjmedia_snd_port_connect(receiver->splitter->snd_port[i], receiver->splitter->r_stream_port[i]);
			}
			pjmedia_stream_start(receiver->splitter->r_stream[i]);
		}
		PJ_LOG(2, (__FILE__, "------------------------DEBUG--------------------"));
		pjmedia_stream_start(receiver->splitter->s_stream);
		pjmedia_transport_loop_disable_rx(receiver->splitter->transport, receiver->splitter->s_stream, PJ_TRUE);
		receiver->state = EPS_START;
	}
	else {
		pj_status_t status;
		int i = 0;
		for (i=0; i<MAX_SOUND_DEVICE; i++) {
			if (receiver->splitter->port_array[i] >= 0 && receiver->splitter->snd_port[i] == NULL) {
				status = pjmedia_snd_port_create_player(receiver->pool, receiver->splitter->port_array[i], 8000, 1, 160, 16, 0, &receiver->splitter->snd_port[i]);
				status = pjmedia_snd_port_connect(receiver->splitter->snd_port[i], receiver->splitter->r_stream_port[i]);
			}
		}
		PJ_LOG(2, (__FILE__, "SPLITTER"));
	}
}
