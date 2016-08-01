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


#include <errno.h>
#include <fcntl.h>
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

#define STATUS_CONNECTING 0
#define STATUS_CONNACK_RECVD 1
#define STATUS_WAITING 2

#define	VERSION	149

/* Global variables for use in callbacks. See sub_client.c for an example of
 * using a struct to hold variables for use in callbacks. */
static char *topic = NULL;
static char *message = NULL;
static long msglen = 0;
static int qos = 0;
static int retain = 0;
static int mode = MSGMODE_NONE;
static int status = STATUS_CONNECTING;
static int mid_sent = 0;
static int last_mid = -1;
static int last_mid_sent = -1;
static bool connected = true;
static char *username = NULL;
static char *password = NULL;
static bool disconnect_sent = false;
static bool quiet = false;

#define	PUB_MAX	30

//Titus : pthread related declarations
void *publisher_func_new( void *ptr );
pthread_t thread1;
const char *message_mqtt_pub_new = "Mqtt Publisher Thread";
int  iret2;
int table_max_new = 0;
int rc_new = 0;
unsigned char buf[100];
static int thread_status;
int nw_status_arr[PUB_MAX];
bool  mqtt_pub_init_new = false;

struct 
{
	char* clientid;
//	char* qos;
	char* username;
	char* password;
	char* host;
	int port;
	int showtopics;
	char* topic;
	char* topic_msg;
	char* topic_msg_int;
	bool enable;
	int pubid;
}opts_pub[PUB_MAX];

void add_to_table_new( int pubid, bool enable, char* msg, char* host, char* topic, int port, char* clientid, char* username, char* password )
{
int index = 0;
int duplicate_id = 0;

	if(!mqtt_pub_init_new)
	mqttNew_MqttNew_doMqttNewInit_pub();

	while(index < PUB_MAX)                                                                               
	{
	if((opts_pub[index].pubid != -1) && (pubid == opts_pub[index].pubid))
		{
		opts_pub[index].enable           = enable;
		opts_pub[index].topic_msg_int    = msg;
		opts_pub[index].host             = host;
		opts_pub[index].topic            = topic;
		opts_pub[index].port             = port;
		opts_pub[index].clientid         = clientid;
		opts_pub[index].username         = username;
		opts_pub[index].password         = password;
//		opts_pub[index].qos              = qos;
		duplicate_id = 1;
		break;
		}
	index++;                                                                                    
	}

	if(!duplicate_id)
	{
	opts_pub[table_max_new].pubid             = pubid;
	opts_pub[table_max_new].enable            = enable;
	opts_pub[table_max_new].topic_msg_int     = msg;
	opts_pub[table_max_new].host              = host;
	opts_pub[table_max_new].topic             = topic;
	opts_pub[table_max_new].port              = port;
	opts_pub[table_max_new].clientid          = clientid;
	opts_pub[table_max_new].username          = username;
	opts_pub[table_max_new].password          = password;
//	opts_pub[table_max_new].qos               = qos;

	table_max_new++;
	}
}   

mqttNew_MqttNew_doMqttNewInit_pub(SedonaVM* vm, Cell* params)
{
int i = 0;

if(mqtt_pub_init_new)
goto exit;

	for(i = 0; i < PUB_MAX; i++)
	{
	opts_pub[i].pubid = -1;
	nw_status_arr[i] = 1;
	}

mqtt_pub_init_new = true;
printf(" * [MQTT:PUB] MQTT Publisher is initalized! \n");
return 0;

exit:
printf(" * [MQTT:PUB] MQTT Publisher is already initalized! \n");
}

//doMqttNewPublisherMsg(int Pub_ID, bool enable, Str payload, Str host, Str topic, Str port, Str clientid, Str username, Str password)
mqttNew_MqttNew_doMqttNewPublisherMsg(SedonaVM* vm, Cell* params)
{
	add_to_table_new(params[0].ival, params[1].ival, params[2].aval, params[3].aval, params[4].aval, params[5].aval, params[6].aval, params[7].aval, params[8].aval);
}

Cell mqttNew_MqttNew_doMqttNewNetworkStatus_Pub(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
buf = &nw_status_arr;
result.aval = buf;
return result;
}

mqttNew_MqttNew_doMqttNewPublisherInt(SedonaVM* vm, Cell* params)
{
	//Don't call second time.
	if(thread_status == 0)
	{
	iret2 = pthread_create( &thread1, NULL, publisher_func_new, (void*) message_mqtt_pub_new );
		if(iret2)
		{
		fprintf(stderr," * [MQTT:PUB] Error - pthread_create() return code: %d\n",iret2);
		exit(EXIT_FAILURE);
		}
	else
	printf(" * [MQTT:PUB] Thread create success: return %d\n",iret2);
	thread_status++;
	}
}

void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int rc_new = MOSQ_ERR_SUCCESS;

	if(!result){
		switch(mode){
			case MSGMODE_CMD:
			case MSGMODE_FILE:
			case MSGMODE_STDIN_FILE:
				rc_new = mosquitto_publish(mosq, &mid_sent, topic, msglen, message, qos, retain);
				break;
			case MSGMODE_NULL:
				rc_new = mosquitto_publish(mosq, &mid_sent, topic, 0, NULL, qos, retain);
				break;
			case MSGMODE_STDIN_LINE:
				status = STATUS_CONNACK_RECVD;
				break;
		}
		if(rc_new){
			if(!quiet){
				switch(rc_new){
					case MOSQ_ERR_INVAL:
						fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
						break;
					case MOSQ_ERR_NOMEM:
						fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
						break;
					case MOSQ_ERR_NO_CONN:
						fprintf(stderr, "Error: Client not connected when trying to publish.\n");
						break;
					case MOSQ_ERR_PROTOCOL:
						fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
						break;
					case MOSQ_ERR_PAYLOAD_SIZE:
						fprintf(stderr, "Error: Message payload is too large.\n");
						break;
				}
			}
			mosquitto_disconnect(mosq);
		}
	}else{
		if(result && !quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc_new)
{
	connected = false;
}

void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	last_mid_sent = mid;
	if(mode == MSGMODE_STDIN_LINE){
		if(mid == last_mid){
			mosquitto_disconnect(mosq);
			disconnect_sent = true;
		}
	}else if(disconnect_sent == false){
		mosquitto_disconnect(mosq);
		disconnect_sent = true;
	}
}

void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}

int load_stdin(void)
{
	long pos = 0, rlen;
	char buf[1024];
	char *aux_message = NULL;

	mode = MSGMODE_STDIN_FILE;

	while(!feof(stdin)){
		rlen = fread(buf, 1, 1024, stdin);
		aux_message = realloc(message, pos+rlen);
		if(!aux_message){
			if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
			free(message);
			return 1;
		} else
		{
			message = aux_message;
		}
		memcpy(&(message[pos]), buf, rlen);
		pos += rlen;
	}
	msglen = pos;

	if(!msglen){
		if(!quiet) fprintf(stderr, "Error: Zero length input.\n");
		return 1;
	}

	return 0;
}

int load_file(const char *filename)
{
	long pos, rlen;
	FILE *fptr = NULL;

	fptr = fopen(filename, "rb");
	if(!fptr){
		if(!quiet) fprintf(stderr, "Error: Unable to open file \"%s\".\n", filename);
		return 1;
	}
	mode = MSGMODE_FILE;
	fseek(fptr, 0, SEEK_END);
	msglen = ftell(fptr);
	if(msglen > 268435455){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is too large (>268,435,455 bytes).\n", filename);
		return 1;
	}else if(msglen == 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is empty.\n", filename);
		return 1;
	}else if(msglen < 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Unable to determine size of file \"%s\".\n", filename);
		return 1;
	}
	fseek(fptr, 0, SEEK_SET);
	message = malloc(msglen);
	if(!message){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	pos = 0;
	while(pos < msglen){
		rlen = fread(&(message[pos]), sizeof(char), msglen-pos, fptr);
		pos += rlen;
	}
	fclose(fptr);
	return 0;
}

void *publisher_func_new( void *ptr )
{
struct mosq_config cfg;
struct mosquitto *mosq = NULL;
int rc_new;
int rc_new2;
char *buf;
int buf_len = 1024;
int buf_len_actual;
int read_len;
int pos,i;
unsigned int reconnects = 0;
int argc_new; char *argv_new[20];

printf(" * [MQTT:PUB] \"%s, %s\" Publisher thread function is called\n",message_mqtt_pub_new, ptr);

while(1)
{
	for(i = 0; i < table_max_new; i++)
	{
		if( opts_pub[i].enable )
		{

argc_new = 15;
argv_new[0] = "dummy";
argv_new[1] = "-t";
argv_new[2] = opts_pub[i].topic;
argv_new[3] = "-m";
argv_new[4] = opts_pub[i].topic_msg_int;
argv_new[5] = "-h";
argv_new[6] = opts_pub[i].host;
argv_new[7] = "-p";
argv_new[8] = opts_pub[i].port;
argv_new[9] = "-u";
argv_new[10] = opts_pub[i].username;
argv_new[11] = "-P";
argv_new[12] = opts_pub[i].password;
argv_new[13] = "-I";
argv_new[14] = opts_pub[i].clientid;
//argv_new[15] = "-q";
//argv_new[16] = opts_pub[i].qos;


//printf("ARGS topic %s message %s host %s port %s retain %d mode %d username %s password %s clientid %s \n",opts_pub[i].topic,opts_pub[i].topic_msg_int,opts_pub[i].host,opts_pub[i].port,retain,mode,opts_pub[i].username,opts_pub[i].password,opts_pub[i].clientid);

	buf = malloc(buf_len);
	if(!buf){
		fprintf(stderr, "Error: Out of memory.\n");
		nw_status_arr[i] = -1;
		goto exit1;
//		return 1;
	}

	memset(&cfg, 0, sizeof(struct mosq_config));
	rc_new = client_config_load(&cfg, CLIENT_PUB, argc_new, argv_new);
	if(rc_new){
		client_config_cleanup(&cfg);
		if(rc_new == 2){
		fprintf(stderr, "** [MQTT:Publisher ID %d] Error in arguments. FILE:%s FUNCTION:%s LINE:%d ERROR NO %d\n",i,__FILE__,__func__,__LINE__,rc_new);

		}else{
		fprintf(stderr, "** [MQTT:Publisher ID %d] Unknown Error. FILE:%s FUNCTION:%s LINE:%d ERROR NO %d\n",i,__FILE__,__func__,__LINE__,rc_new);

		}
		nw_status_arr[i] = -1;
		goto exit1;
//		return 1;
	}

	topic = cfg.topic;
	message = cfg.message;
	msglen = cfg.msglen;
	qos = cfg.qos;
	retain = cfg.retain;
	mode = cfg.pub_mode;
	username = cfg.username;
	password = cfg.password;
	quiet = cfg.quiet;

	if(cfg.pub_mode == MSGMODE_STDIN_FILE){
		if(load_stdin()){
			fprintf(stderr, "Error loading input from stdin.\n");
			nw_status_arr[i] = -1;
			goto exit1;
//			return 1;
		}
	}else if(cfg.file_input){
		if(load_file(cfg.file_input)){
			fprintf(stderr, "Error loading input file \"%s\".\n", cfg.file_input);
			nw_status_arr[i] = -1;
			goto exit1;
//			return 1;
		}
	}

	if(!topic || mode == MSGMODE_NONE){
		fprintf(stderr, "Error: Both topic and message must be supplied.\n");
		fprintf(stderr, "\nUse 'Please do check your args.\n");
		nw_status_arr[i] = -1;
		goto exit1;
//		return 1;
	}

	mosquitto_lib_init();

	if(client_id_generate(&cfg, "mosqpub")){
		fprintf(stderr, "** [MQTT:Publisher ID %d] Error in generating CLIENT ID. FILE:%s FUNCTION:%s LINE:%d\n",i,__FILE__,__func__,__LINE__);
		nw_status_arr[i] = -1;
		goto exit2;
//		return 1;
	}

	mosq = mosquitto_new(cfg.id, true, NULL);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!quiet) fprintf(stderr, "Error: Invalid id.\n");
				break;
		}
//		mosquitto_lib_cleanup();
		nw_status_arr[i] = -1;
		goto exit2;
//		return 1;
	}
	if(cfg.debug){
		mosquitto_log_callback_set(mosq, my_log_callback);
	}
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);

	if(client_opts_set(mosq, &cfg)){
		nw_status_arr[i] = -1;
		goto exit2;
//		return 1;
	}
	rc_new = client_connect(mosq, &cfg);
	if(rc_new)
	{
	printf("############# Not able to connect the client ################\n");
//	return rc_new;
	nw_status_arr[i] = -2;
	goto exit3;
	}

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_start(mosq);
	}

	do{
		if(mode == MSGMODE_STDIN_LINE){
			if(status == STATUS_CONNACK_RECVD){
				pos = 0;
				read_len = buf_len;
				while(fgets(&buf[pos], read_len, stdin)){
					buf_len_actual = strlen(buf);
					if(buf[buf_len_actual-1] == '\n'){
						buf[buf_len_actual-1] = '\0';
						rc_new2 = mosquitto_publish(mosq, &mid_sent, topic, buf_len_actual-1, buf, qos, retain);
						if(rc_new2){
							if(!quiet) fprintf(stderr, "Error: Publish returned %d, disconnecting.\n", rc_new2);
							mosquitto_disconnect(mosq);
						}
						break;
					}else{
						buf_len += 1024;
						pos += 1023;
						read_len = 1024;
						buf = realloc(buf, buf_len);
						if(!buf){
							fprintf(stderr, "Error: Out of memory.\n");
							nw_status_arr[i] = -1;
							goto exit2;
//							return 1;
						}
					}
				}
				if(feof(stdin)){
					last_mid = mid_sent;
					status = STATUS_WAITING;
				}
			}else if(status == STATUS_WAITING){
				if(last_mid_sent == last_mid && disconnect_sent == false){
					mosquitto_disconnect(mosq);
					disconnect_sent = true;
				}
#ifdef WIN32
				Sleep(100);
#else
				usleep(100000);
#endif
			}
			rc_new = MOSQ_ERR_SUCCESS;
		}else{
			usleep(100000);//Titus fix
			rc_new = mosquitto_loop(mosq, -1, 1);
			if(rc_new == 3)
			{
			rc_new = mosquitto_loop(mosq, -1, 1);
			printf("$$$$$$$$$$$$$$ mosquitto_loop rc_new is after reconnecting %d rc_new \n",rc_new);
			}
	
		}
	}while(rc_new == MOSQ_ERR_SUCCESS && connected);

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_stop(mosq, false);
	}

	if(message && mode == MSGMODE_FILE){
		free(message);
	}
exit3:
	mosquitto_destroy(mosq);

exit2:
	mosquitto_lib_cleanup();

exit1:
	if(rc_new){
		fprintf(stderr, "#################################### %s Error: %s\n", opts_pub[i].topic, mosquitto_strerror(rc_new));
//		return rc_new;
		nw_status_arr[i] = -2;
		}
	else
		nw_status_arr[i] = 1;

	free(buf);

     }//if	
    }//for
  }//while(1)
}
