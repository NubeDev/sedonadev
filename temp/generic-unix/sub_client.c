/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include "mosquitto.h"
#include "client_shared.h"

#include "sedona.h"
//for sleep
#include <unistd.h>

#define	SUB_MAX	30

#define	VERSION	149

bool process_messages = true;
int msg_count = 0;

pthread_mutex_t topic_mutex,msg_mutex;

//Titus : pthread related declarations
static void *subscriber_func( void *ptr );
pthread_t thread2;
static const char *message_mqtt_sub = "Mqtt Subscriber Thread";
int  iret3;

static int malloc_done = 0;

char *rx_buffer = "***AIDAN***";
static int rx_buffer_int = 0;
static int table_max_sub = 0;
static int thread_status;
static int nw_status_arr[SUB_MAX];
static int rc_sub = 0;
static unsigned char buf[100];
static unsigned char readbuf[100];
static bool  mqtt_sub_init = false;

char *topic_rcd_new = "***AIDAN***";

struct 
{
	char* clientid;
	char* username;
	char* password;
	char* host;
	int port;
	char* topic;
	int topic_msg_int;
	bool enable;
	int subid;
}opts_sub[SUB_MAX];

void add_to_table_sub_new( int subid, bool enable, int msg, char* host, char* topic, int port, char* clientid, char* username, char* password)
{
int index = 0;
int duplicate_id = 0;

	if(!mqtt_sub_init)
	mqttNew_MqttNew_doMqttNewInit_sub();

	while(index < SUB_MAX)                                                                               
	{
	if((opts_sub[index].subid != -1) && (subid == opts_sub[index].subid))
		{
		opts_sub[index].enable           = enable;
		opts_sub[index].host             = host;
		opts_sub[index].topic            = topic;
		opts_sub[index].port             = port;
		opts_sub[index].clientid         = clientid;
		opts_sub[index].username         = username;
		opts_sub[index].password         = password;
		duplicate_id = 1;
		break;
		}
	index++;                                                                                    
	}

	if(!duplicate_id)
	{
	opts_sub[table_max_sub].subid             = subid;
	opts_sub[table_max_sub].enable            = enable;
	opts_sub[table_max_sub].host              = host;
	opts_sub[table_max_sub].topic             = topic;
	opts_sub[table_max_sub].port              = port;
	opts_sub[table_max_sub].clientid          = clientid;
	opts_sub[table_max_sub].username          = username;
	opts_sub[table_max_sub].password          = password;
	table_max_sub++;
	}
}

mqttNew_MqttNew_doMqttNewInit_sub(SedonaVM* vm, Cell* params)
{
int i = 0;

if(mqtt_sub_init)
goto exit;

	for(i = 0; i < SUB_MAX; i++)
	{
	opts_sub[i].subid = -1;
	nw_status_arr[i] = 1;
	}

mqtt_sub_init = true;

printf("** [MQTT:Subscriber] MQTT Subscriber is initalized! \n");
return 0;

exit:
printf("** [MQTT:Subscriber] MQTT Subscriber is already initalized! \n");
}

mqttNew_MqttNew_doMqttNewSubscriberMsg(SedonaVM* vm, Cell* params)
{
	add_to_table_sub_new(params[0].ival, params[1].ival, params[2].aval, params[3].aval, params[4].aval, params[5].aval, params[6].aval, params[7].aval, params[8].aval);
}

Cell mqttNew_MqttNew_doMqttNewNetworkStatus_Sub(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
buf = &nw_status_arr;
result.aval = buf;
return result;
}

mqttNew_MqttNew_doMqttNewSubscriberInt(SedonaVM* vm, Cell* params)
{
	if(thread_status == 0)
	{
	iret3 = pthread_create( &thread2, NULL, subscriber_func, (void*) message_mqtt_sub );
		if(iret3)
		{
		fprintf(stderr,"** [MQTT:Subscriber] Error - pthread_create() return code: %d\n",iret3);
		exit(EXIT_FAILURE);
		}
	else
	printf("** [MQTT:Subscriber] Thread create success: return %d\n",iret3);
	thread_status++;
	}
//pthread_mutex_lock(&msg_mutex);
//printf(" * [MQTT:SUB] RX_BUFFER %s\n",rx_buffer);
//pthread_mutex_unlock(&msg_mutex);
return rx_buffer;
}

Cell mqttNew_MqttNew_doMqttNewRead_Topic(SedonaVM* vm, Cell* params)
{
pthread_mutex_lock(&topic_mutex);
Cell result;
//printf(" * [MQTT:SUB] topic_rcd_new %s\n",topic_rcd_new);
result.aval = topic_rcd_new;
//if(malloc_done){
//free(topic_rcd_new);
//malloc_done = 0;
//}
pthread_mutex_unlock(&topic_mutex);

return result;
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct mosq_config *cfg;
	int i;
	bool res;

	for(i = 0; i < table_max_sub; i++)
	{
		if( opts_sub[i].enable )
		{
		
	if(process_messages == false) return;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(message->retain && cfg->no_retain) return;
	if(cfg->filter_outs){
		for(i=0; i<cfg->filter_out_count; i++){
			mosquitto_topic_matches_sub(cfg->filter_outs[i], message->topic, &res);
			if(res) return;
		}
	}


	if(cfg->verbose){
		if(message->payloadlen){
			printf("%s ", message->topic);
//			fwrite(message->payload, 1, message->payloadlen, stdout);
			if(cfg->eol){
				printf("\n");
			}
		}else{
			if(cfg->eol){
				printf("%s (null)\n", message->topic);
			}
		}
		fflush(stdout);
	}else{
		if(message->payloadlen){
//			fwrite(message->payload, 1, message->payloadlen, stdout);
			if(cfg->eol){
				printf("\n");
			}
			fflush(stdout);
		}
	}
	if(cfg->msg_count>0){
		msg_count++;
		if(cfg->msg_count == msg_count){
			process_messages = false;
			mosquitto_disconnect(mosq);
		}
	}

usleep(1000);

	//Titus
//topic_rcd_new = (char*) message->topic;

pthread_mutex_lock(&msg_mutex);
//rx_buffer = (char*) message->payload;
rx_buffer = malloc(sizeof(message->payload));
memset(rx_buffer, 0, sizeof(message->payload));
memcpy(rx_buffer, message->payload, sizeof(message->payload));
pthread_mutex_unlock(&msg_mutex);

pthread_mutex_lock(&topic_mutex);
//topic_rcd_new = (char*) message->topic;
topic_rcd_new = malloc(sizeof(message->topic)+1);
memset(topic_rcd_new, 0, sizeof(message->topic)+1);
memcpy(topic_rcd_new, message->topic, sizeof(message->topic)+1);
//malloc_done = 1;
pthread_mutex_unlock(&topic_mutex);

		}
	}

}

void my_connect_callback_sub(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct mosq_config *cfg;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(!result){
		for(i=0; i<cfg->topic_count; i++){
			mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
		}
	}else{
		if(result && !cfg->quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	struct mosq_config *cfg;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(!cfg->quiet) printf("** [MQTT:Subscriber] Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		if(!cfg->quiet) printf(", %d", granted_qos[i]);
	}
	if(!cfg->quiet) printf("\n");
}

void my_log_callback_sub(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}


void *subscriber_func( void *ptr )
{
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;
	int rc,i;
	unsigned int reconnects = 0;

printf("** [MQTT:Subscriber] Subscriber thread function is called\n");

int argc_new; char *argv_new[100];

while(1)
{
	for(i = 0; i < table_max_sub; i++)
	{
		if( opts_sub[i].enable )
		{

argc_new = 13;
argv_new[0] = "dummy";
argv_new[1] = "-t";
argv_new[2] = opts_sub[i].topic;
argv_new[3] = "-h";
argv_new[4] = opts_sub[i].host;
argv_new[5] = "-p";
argv_new[6] = opts_sub[i].port;
argv_new[7] = "-u";
argv_new[8] = opts_sub[i].username;
argv_new[9] = "-P";
argv_new[10] = opts_sub[i].password;
argv_new[11] = "-I";
argv_new[12] = opts_sub[i].clientid;
	
	rc = client_config_load(&cfg, CLIENT_SUB, argc_new, argv_new);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			/* --help */
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Error in arguments. FILE:%s FUNCTION:%s LINE:%d ERROR NO %d\n",i,__FILE__,__func__,__LINE__,rc);
		}else{
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Unknown Error. FILE:%s FUNCTION:%s LINE:%d ERROR NO %d\n",i,__FILE__,__func__,__LINE__,rc);
		}
		nw_status_arr[i]     = -1;
		goto exit1;
//		return 1;
	}
//	else
//		nw_status_arr[i]     = 1;		

	mosquitto_lib_init();

	if(client_id_generate(&cfg, "mosqsub")){
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Error in generating CLIENT ID. FILE:%s FUNCTION:%s LINE:%d\n",i,__FILE__,__func__,__LINE__);
		nw_status_arr[i]     = -1;
		goto exit2;
//		return 1;
	}
//	else
//		nw_status_arr[i]     = 1;

	mosq = mosquitto_new(cfg.id, cfg.clean_session, &cfg);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!cfg.quiet) fprintf(stderr, "** [MQTT:Subscriber ID %d] Error: Out of memory.\n",i);
				break;
			case EINVAL:
				if(!cfg.quiet) fprintf(stderr, "** [MQTT:Subscriber ID %d] Error: Invalid id and/or clean_session.\n",i);
				break;
		}
//		mosquitto_lib_cleanup();
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Error in creating MOSQUITTO. FILE:%s FUNCTION:%s LINE:%d ERROR NO %d\n",i,__FILE__,__func__,__LINE__,errno);
		nw_status_arr[i]     = -1;
		goto exit2;		
//		return 1;
	}
//	else
//		nw_status_arr[i]     = 1;
	if(client_opts_set(mosq, &cfg)){
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Error in setting client options. FILE:%s FUNCTION:%s LINE:%d \n",i,__FILE__,__func__,__LINE__);
		nw_status_arr[i]     = -1;
		goto exit2;
//		return 1;
	}
//	else
//		nw_status_arr[i]     = 1;

	if(cfg.debug){
		mosquitto_log_callback_set(mosq, my_log_callback_sub);
		mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	}
	mosquitto_connect_callback_set(mosq, my_connect_callback_sub);
	mosquitto_message_callback_set(mosq, my_message_callback);

	rc = client_connect(mosq, &cfg);
	if(rc) 
	{
	fprintf(stderr, "** [MQTT:Subscriber ID %d] Error in clienting connect. FILE:%s FUNCTION:%s LINE:%d Error no %d\n",i,__FILE__,__func__,__LINE__,rc);
	nw_status_arr[i]     = -2;
	goto exit3;
//	return rc;
	}
	else
	nw_status_arr[i]     = 1;
	


	usleep(10000);//Titus Fix

//	rc = mosquitto_loop_forever(mosq, -1, 1);
	rc = mosquitto_loop(mosq, -1, 1);
	rc = mosquitto_loop(mosq, -1, 1);
	rc = mosquitto_loop(mosq, -1, 1);
	rc = mosquitto_loop(mosq, -1, 1);

/*
		do{
//			printf("mosquitto_loop........\n");
			rc = mosquitto_loop(mosq, -1, 1);
			if (reconnects !=0 && rc == MOSQ_ERR_SUCCESS){
				reconnects = 0;
			}
		}while(rc == MOSQ_ERR_SUCCESS);
*/

exit3:
	mosquitto_destroy(mosq);
exit2:	
	mosquitto_lib_cleanup();

exit1:

	if(cfg.msg_count>0 && rc == MOSQ_ERR_NO_CONN){
		rc = 0;
	}
	if(rc){
		fprintf(stderr, "** [MQTT:Subscriber ID %d] Error: %s\n",i, mosquitto_strerror(rc));
	}

//printf("Successfully passed!\n");
//	return rc;

      }
    }
 }//while(1)

}

