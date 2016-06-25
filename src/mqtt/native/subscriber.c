#include <stdio.h>
#include "MQTTClient.h"
#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <sys/time.h>
#include "sedona.h"
//for sleep
#include <unistd.h>

#define	SUB_MAX	30

//Titus : pthread related declarations
void *subscriber_func( void *ptr );
pthread_t thread2;
const char *message_mqtt_sub = "Mqtt Subscriber Thread";
int  iret3;

char *rx_buffer;
static int rx_buffer_int = 0;
int table_max_sub = 0;
static int thread_status;
static int nw_status_arr[SUB_MAX];
int rc_sub = 0;
static unsigned char buf[100];
static unsigned char readbuf[100];
static Network n;
static Client c;
bool  mqtt_sub_init = false;

struct 
{
	char* clientid;
	enum QoS qos;
	char* username;
	char* password;
	char* host;
	int port;
	char* topic;
	int topic_msg_int;
	bool enable;
	int subid;
}opts_sub[SUB_MAX];

void add_to_table_sub( int subid, bool enable, int msg, char* host, char* topic, int port, char* clientid, char* username, char* password, int qos )
{
int index = 0;
int duplicate_id = 0;

	if(!mqtt_sub_init)
	mqtt_MqttDev_doMqttInit_sub();

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
		opts_sub[index].qos              = qos;
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
	opts_sub[table_max_sub].qos               = qos;
	table_max_sub++;
	}
}

mqtt_MqttDev_doMqttInit_sub(SedonaVM* vm, Cell* params)
{
int i = 0;

if(mqtt_sub_init)
goto exit;

	for(i = 0; i < SUB_MAX; i++)
	opts_sub[i].subid = -1;

mqtt_sub_init = true;

printf(" * [MQTT:SUB] MQTT Subscriber is initalized! \n");
return 0;

exit:
printf(" * [MQTT:SUB] MQTT Subscriber is already initalized! \n");
}

mqtt_MqttDev_doMqttSubscriberMsg(SedonaVM* vm, Cell* params)
{
	add_to_table_sub(params[0].ival, params[1].ival, params[2].ival, params[3].aval, params[4].aval, params[5].ival, params[6].aval, params[7].aval, params[8].aval, params[9].ival);
}

Cell mqtt_MqttDev_doMqttNetworkStatus_Sub(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
buf = &nw_status_arr;
result.aval = buf;
return result;
}

mqtt_MqttDev_doMqttSubscriberInt(SedonaVM* vm, Cell* params)
{
	if(thread_status == 0)
	{
	iret3 = pthread_create( &thread2, NULL, subscriber_func, (void*) message_mqtt_sub );
		if(iret3)
		{
		fprintf(stderr," * [MQTT:SUB] Error - pthread_create() return code: %d\n",iret3);
		exit(EXIT_FAILURE);
		}
	else
	printf(" * [MQTT:SUB] Thread create success: return %d\n",iret3);
	thread_status++;
	}

return rx_buffer_int;
}

void messageArrived(MessageData* md)
{
MQTTMessage* message = md->message;
rx_buffer = malloc(message->payloadlen);
memset(rx_buffer, 0, message->payloadlen);
memcpy(rx_buffer, message->payload, message->payloadlen);
rx_buffer_int = atoi(rx_buffer);
//printf(" * [MQTT:SUB] ##################### Message Arrived! and data is %d #######################\n",rx_buffer_int);
}

void *subscriber_func( void *ptr )
{
int i;
int dummy;
char *message;
message = (char *) ptr;
printf(" * [MQTT:SUB] \"%s\" Subscriber thread function is called\n",message);

	while(1)
	{
		for(i = 0; i < table_max_sub; i++)
		{
			if(opts_sub[i].enable)
			{

//		printf(" * [MQTT:SUB] Subscriber ID %d\t| Enable %d\t| Host %s\t| Topic %s\t| Port %d\t| Clientid %s\t| Username %s\t| Password %s\t| QOS %d\n", opts_sub[i].subid, opts_sub[i].enable, opts_sub[i].host, opts_sub[i].topic, opts_sub[i].port, opts_sub[i].clientid, opts_sub[i].username, opts_sub[i].password, opts_sub[i].qos);

			NewNetwork(&n);
			ConnectNetwork(&n, opts_sub[i].host, opts_sub[i].port);
			MQTTClient(&c, &n, 1000, buf, 100, readbuf, 100);
 			MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
			data.willFlag = 0;
			data.MQTTVersion = 3;
			data.clientID.cstring = opts_sub[i].clientid;
			data.username.cstring = opts_sub[i].username;
			data.password.cstring = opts_sub[i].password;
			data.keepAliveInterval = 10;
			data.cleansession = 1;
//			printf(" * [MQTT:SUB] Connecting to %s %d\n", opts_sub[i].host, opts_sub[i].port);
			rc_sub = MQTTConnect(&c, &data);

//			printf(" * [MQTT:SUB%d] Connected %d, rc_sub -> %d \n", i, c.isconnected, rc_sub);

			if( c.isconnected < 0 || rc_sub < 0 )
			{
			printf(" * [MQTT:SUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n", i, rc_sub);
			nw_status_arr[i]     = -1;
			goto disconnect;
			}
			else
			nw_status_arr[i]     = 1;
   
//			printf(" * [MQTT:SUB%d] Subscribing to %s\n", i, opts_sub[i].topic);
			rc_sub = MQTTSubscribe(&c, opts_sub[i].topic, opts_sub[i].qos, messageArrived);
//			printf(" * [MQTT:SUB%d] Subscribed to \"%s\", status is %d\n", i, opts_sub[i].topic,rc_sub);
			MQTTYield(&c, 100);//Its worked! It might needed if we don't want to use "msleep"
disconnect:
			dummy=0;
			}
		}
	}
}
