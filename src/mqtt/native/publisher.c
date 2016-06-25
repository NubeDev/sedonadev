#include <stdio.h>
#include "MQTTClient.h"
#include <stdio.h>
#include <signal.h>
#include <memory.h>
//Titus : pthread include file
#include <pthread.h>
#include <sys/time.h>
#include "sedona.h"


#define	PUB_MAX	30

//Titus : pthread related declarations
void *publisher_func( void *ptr );
pthread_t thread1;
const char *message_mqtt_pub = "Mqtt Publisher Thread";
int  iret2;
char buffer [100];
int table_max = 0;
Network n;
Client c;
int rc = 0;
unsigned char buf[100];
unsigned char readbuf[100];
static int thread_status;
int nw_status_arr[PUB_MAX];
bool  mqtt_pub_init = false;

struct 
{
	char* clientid;
	enum QoS qos;
	char* username;
	char* password;
	char* host;
	int port;
	int showtopics;
	char* topic;
	char* topic_msg;
	int topic_msg_int;
	bool enable;
	int pubid;
}opts_pub[PUB_MAX];

void add_to_table( int pubid, bool enable, int msg, char* host, char* topic, int port, char* clientid, char* username, char* password, int qos )
{
int index = 0;
int duplicate_id = 0;

	if(!mqtt_pub_init)
	mqtt_MqttDev_doMqttInit_pub();

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
		opts_pub[index].qos              = qos;
		duplicate_id = 1;
		break;
		}
	index++;                                                                                    
	}

	if(!duplicate_id)
	{
	opts_pub[table_max].pubid             = pubid;
	opts_pub[table_max].enable            = enable;
	opts_pub[table_max].topic_msg_int     = msg;
	opts_pub[table_max].host              = host;
	opts_pub[table_max].topic             = topic;
	opts_pub[table_max].port              = port;
	opts_pub[table_max].clientid          = clientid;
	opts_pub[table_max].username          = username;
	opts_pub[table_max].password          = password;
	opts_pub[table_max].qos               = qos;

	table_max++;
	}
}   

mqtt_MqttDev_doMqttInit_pub(SedonaVM* vm, Cell* params)
{
int i = 0;

if(mqtt_pub_init)
goto exit;

	for(i = 0; i < PUB_MAX; i++)
	opts_pub[i].pubid = -1;

mqtt_pub_init = true;
printf(" * [MQTT:PUB] MQTT Publisher is initalized! \n");
return 0;

exit:
printf(" * [MQTT:PUB] MQTT Publisher is already initalized! \n");
}

mqtt_MqttDev_doMqttPublisherMsg(SedonaVM* vm, Cell* params)
{
	add_to_table(params[0].ival, params[1].ival, params[2].ival, params[3].aval, params[4].aval, params[5].ival, params[6].aval, params[7].aval, params[8].aval, params[9].ival);
}

Cell mqtt_MqttDev_doMqttNetworkStatus_Pub(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
buf = &nw_status_arr;
result.aval = buf;
return result;
}

mqtt_MqttDev_doMqttPublisherInt(SedonaVM* vm, Cell* params)
{
	//Don't call second time.
	if(thread_status == 0)
	{
	iret2 = pthread_create( &thread1, NULL, publisher_func, (void*) message_mqtt_pub );
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

void *publisher_func( void *ptr )
{
int i;
int dummy;
char *message;
message = (char *) ptr;
int threadid;

printf(" * [MQTT:PUB] \"%s\" Publisher thread function is called\n",message);

	while(1)
	{
		for(i = 0; i < table_max; i++)
		{
			if( opts_pub[i].enable )
			{
			NewNetwork(&n);
			ConnectNetwork(&n, opts_pub[i].host, opts_pub[i].port);
			MQTTClient(&c, &n, 1000, buf, 100, readbuf, 100);
 		
			MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
			data.willFlag = 0;
			data.MQTTVersion = 3;
			data.clientID.cstring = opts_pub[i].clientid;
			data.username.cstring = opts_pub[i].username;
			data.password.cstring = opts_pub[i].password;
		
			data.keepAliveInterval = 10;
			data.cleansession = 1;
//			printf(" * [MQTT:PUB] Connecting to %s %d table_max %d\n", opts_pub[i].host, opts_pub[i].port,table_max);
		
			rc = MQTTConnect(&c, &data);

//			printf(" * [MQTT:PUB%d] Connected %d, rc -> %d\n", i, c.isconnected, rc);
//			printf(" * [MQTT:PUB%d] After Connecting \n",i);

			if( c.isconnected < 0 || rc < 0 )
			{
			printf(" * [MQTT:PUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n",i , rc);
			nw_status_arr[i]     = -1;
			goto disconnect;
			}
			else
			nw_status_arr[i]     = 1;


			MQTTMessage msg;
			msg.qos = opts_pub[i].qos;
			msg.retained = 0;
			msg.dup = 0;
			sprintf(buffer, "%d", opts_pub[i].topic_msg_int);
			msg.payload = (void *)buffer;
			msg.payloadlen = strlen(buffer)+1;
			rc = MQTTPublish(&c, opts_pub[i].topic, &msg);
//			printf(" * [MQTT:PUB%d] Published to \"%s\", status is %d\n",i , opts_pub[i].topic,rc);

//Need to comment out the below line if we didn't mention the clientid then it leads to hang
//			MQTTYield(&c, 500);	
	
//			printf(" * [MQTT:PUB] Stopping\n");
			MQTTDisconnect(&c);
			n.disconnect(&n);
disconnect:	
			dummy=0;
			}
		}
	}
}
