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
static int Sub_Enable[SUB_MAX];
static int int_sedona[SUB_MAX];
char titus_buffer [100];
char *titus_rx_buffer;
static int titus_rx_buffer_int = 0;
int table_max_sub = 0;
static int thread_status;
static int enable_arr[SUB_MAX];
static int i_bkp = 0;
static int table_max_sub_bkp = 0;
volatile int toStop = 0;
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
	int nw_status;
}opts_sub[SUB_MAX];
// =
//{
//	{ (char*)"pubclient", QOS2, NULL, NULL, (char*)"localhost", 1883, (char*)"topic1", 0, 0, 0, 0}
//};

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

mqtt_MqttDev_doMqttNetworkStatus_Sub(SedonaVM* vm, Cell* params)
{
int i = 0;
	for(i = 0; i < table_max_sub; i++)
	{
		if(opts_sub[i].nw_status < 0)
		{
//		printf(" * [MQTT:SUB%d] Not able to connect, opts_sub[%d].nw_status -> %d\n",i ,i ,opts_sub[i].nw_status);
		return -1;
		}
	}
//Resetting to older value
	for(i = 0; i < table_max_sub; i++)
	{
	opts_sub[i].nw_status = 1;
	enable_arr[i] = 1;
	}
return 0;
}

mqtt_MqttDev_doMqttNwErrID_Sub(SedonaVM* vm, Cell* params)
{
int i = 0;
static i_bkp = 0;
	for(i = 0; i < table_max_sub; i++)
	{
		if(opts_sub[i].nw_status < 0)
		{
//		printf(" * [MQTT:SUB] Not able to connect the network in Subscriber ID is %d opts_sub[%d].nw_status %d\n",i,i,opts_sub[i].nw_status);
		i_bkp = i;
		return i;//return the subid to sedona
		}
	}
//	return -1;//return the subid to sedona
	return i_bkp;//return the subid to sedona
}

Cell mqtt_MqttDev_doMqttNwErrID_Obj_Sub(SedonaVM* vm, Cell* params)
{
int i = 0;
	for(i = 0; i < table_max_sub; i++)
	{
		if(opts_sub[i].nw_status < 0)
		{
		Cell result;
		result.aval = &enable_arr;

		printf(" * [MQTT:SUB] address of enable_arr %u which is returned to Sedona\n",&enable_arr);
		return result;
		}
	}
	return negOneCell;//return the subid to sedona
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

//printf(" * [MQTT:SUB] : mqtt_MqttDev_doMqttSubscriberInt return %d\n",titus_rx_buffer_int);

//printf(" * [MQTT:SUB] ##################### mqtt_MqttDev_doMqttSubscriberInt, received data is %d #######################\n",titus_rx_buffer_int);

return titus_rx_buffer_int;
}

mqtt_MqttDev_doMqttSubscriberIDSend(SedonaVM* vm, Cell* params)
{
}

void messageArrived(MessageData* md)
{
MQTTMessage* message = md->message;
titus_rx_buffer = malloc(message->payloadlen);
memset(titus_rx_buffer, 0, message->payloadlen);
memcpy(titus_rx_buffer, message->payload, message->payloadlen);
titus_rx_buffer_int = atoi(titus_rx_buffer);
//printf(" * [MQTT:SUB] ##################### Message Arrived! and data is %d #######################\n",titus_rx_buffer_int);
}

void *subscriber_func( void *ptr )
{
int i;

char *message;
message = (char *) ptr;
int threadid;
//get the ID of thread
threadid = (int)syscall(224);
printf(" * [MQTT:SUB] \"%s\" thread is called, ID is %d\n",message,threadid);

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

//			printf(" * [MQTT:SUB%d] Connected %d\n", i, c.isconnected);

#if 1
			if(c.isconnected)
			{
			enable_arr[i]     = 1;
			opts_sub[i].nw_status = 1;
			}
			else
			{
			printf(" * [MQTT:SUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n", i, rc_sub);
			enable_arr[i]     = -1;
			opts_sub[i].nw_status = -1;
			}
#endif

#if 0
			if(rc_sub != 0)
				{
				printf(" * [MQTT:SUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n", i, rc_sub);
				enable_arr[i]     = -1;
				opts_sub[i].nw_status = -1;
				}
			else
				{
				enable_arr[i]     = 1;
				opts_sub[i].nw_status = 1;
//				printf(" * [MQTT:SUB] Connected %d\n", rc_sub);
				}
#endif	
   
//			printf(" * [MQTT:SUB] Subscribing to %s\n", opts_sub[i].topic);
			rc_sub = MQTTSubscribe(&c, opts_sub[i].topic, opts_sub[i].qos, messageArrived);
//			printf(" * [MQTT:SUB] Subscribed to \"%s\", status is %d\n", opts_sub[i].topic,rc_sub);
			MQTTYield(&c, 100);//Its worked! It might needed if we don't want to use "msleep"
			}
		}
	}
}
