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
static int Pub_Enable[PUB_MAX];
static int int_sedona[PUB_MAX];
static int integer_sedona[PUB_MAX];
char titus_buffer [100];
int table_max = 0;
Network n;
Client c;
int rc = 0;
unsigned char buf[100];
unsigned char readbuf[100];
static int thread_status;
int enable_arr[PUB_MAX];
int test_array[3];
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
	int nw_status;
}opts_pub[PUB_MAX];
// =
//{
//	(char*)"publisher", 0, (char*)"\n", QOS2, NULL, NULL, (char*)"localhost", 1883, 0
//};

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

/*
Cell mqtt_MqttDev_doMqttArrayTest(SedonaVM* vm, Cell* params)
{
//TODO: Need to develop it
int i = 0;
Cell result;

	for(i = 0; i < 3; i++)
	test_array[i] = i;

	result.aval = params[0].aval;
	return result;
}
*/

mqtt_MqttDev_doMqttNetworkStatus(SedonaVM* vm, Cell* params)
{
int i = 0;
	for(i = 0; i < table_max; i++)
	{
		if(opts_pub[i].nw_status < 0)
		{
//		printf(" * [MQTT:PUB%d] Not able to connect, opts_pub[%d].nw_status -> %d\n", i, i, opts_pub[i].nw_status);
		return -1;
		}
	}
//Resetting to older value
	for(i = 0; i < table_max; i++)
	{
	opts_pub[i].nw_status = 1;
	enable_arr[i] = 1;
	}
return 0;
}

mqtt_MqttDev_doMqttNwErrID(SedonaVM* vm, Cell* params)
{
int i = 0;
	for(i = 0; i < table_max; i++)
	{
		if(opts_pub[i].nw_status < 0)
		{
//		printf(" * [MQTT:PUB] Not able to connect the network in Publisher ID is %d opts_pub[%d].nw_status %d\n",i,i,opts_pub[i].nw_status);
		return i;//return the pubid to sedona
		}
	}
//Resetting to older value
	for(i = 0; i < table_max; i++)
	{
	opts_pub[i].nw_status = 1;
	enable_arr[i] = 1;
	}
	return -1;//return the pubid to sedona
}

Cell mqtt_MqttDev_doMqttNwErrID_Obj_Pub(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
buf = &enable_arr;
result.aval = buf;
return result;
}

#if 0
Cell mqtt_MqttDev_doMqttArrayTest(SedonaVM* vm, Cell* params)
{
Cell result;
int *buf = (int*)params[0].aval;
//enable_arr = &buf;
buf = &enable_arr;
printf(" * [MQTT:PUB] Array Values %d %d %d\n",*buf,*(buf+1),*(buf+2));
//*buf = 11;
//*(buf+1) = 22;
//*(buf+2) = 33;
result.aval = buf;
return result;
}
#endif

mqtt_MqttDev_doMqttPublisherInt(SedonaVM* vm, Cell* params)
{
/*
	for(i = 0; i < table_max; i++)
	{
	if(opts_pub[i].enable)
		printf(" * [MQTT:PUB] Pulisher ID %d\t| Message %d\t| Enable %d\t| Host %s\t| Topic %s\t| Port %d\t| Clientid %s\t| Username %s\t| Password %s\t| QOS %d\n", opts_pub[i].pubid, opts_pub[i].topic_msg_int, opts_pub[i].enable, opts_pub[i].host, opts_pub[i].topic, opts_pub[i].port, opts_pub[i].clientid, opts_pub[i].username, opts_pub[i].password, opts_pub[i].qos);
	}
*/
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
char *message;
message = (char *) ptr;
int threadid;
//get the ID of thread
threadid = (int)syscall(224);
printf(" * [MQTT:PUB] \"%s\" thread is called, ID is %d\n",message,threadid);

	while(1)
	{
		for(i = 0; i < table_max; i++)
		{
			if(opts_pub[i].enable)
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

//			printf(" * [MQTT:PUB%d] Connected %d\n", i, c.isconnected);
//			printf(" * [MQTT:PUB] After Connecting \n");

#if 1
			if(c.isconnected)
			{
			enable_arr[i]     = 1;
			opts_pub[i].nw_status = 1;
			}
			else
			{
			printf(" * [MQTT:PUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n",i , rc);
			enable_arr[i]     = -1;
			opts_pub[i].nw_status = -1;
			}
#endif

#if 0
			if(rc != 0)
				{
				printf(" * [MQTT:PUB%d] Not able to connect, please do make sure that provided \"host\" or \"cliendid\" is correct, status %d\n",i , rc);
				enable_arr[i]     = -1;
				opts_pub[i].nw_status = -1;
				}
			else
				{
				enable_arr[i]     = 1;
				opts_pub[i].nw_status = 1;
//				printf(" * [MQTT:PUB] Connected %d\n", rc);
				}
#endif	
    	
			MQTTMessage msg;
			msg.qos = QOS1;
			msg.retained = 0;
			msg.dup = 0;
			sprintf(titus_buffer, "%d", opts_pub[i].topic_msg_int);
			msg.payload = (void *)titus_buffer;
			msg.payloadlen = strlen(titus_buffer)+1;
//			msg.payload_int = opts_pub[i].topic_msg_int;
//			msg.payloadlen = sizeof(opts_pub[i].topic_msg_int);
			rc = MQTTPublish(&c, opts_pub[i].topic, &msg);
//			printf(" * [MQTT:PUB] Published to \"%s\", status is %d\n", opts_pub[i].topic,rc);

//Need to comment out the below line if we didn't mention the clientid
//			MQTTYield(&c, 500);	
	
//			printf(" * [MQTT:PUB] Stopping\n");
			MQTTDisconnect(&c);
			n.disconnect(&n);
			}
		}
	}
}
