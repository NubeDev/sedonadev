/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* Analog Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"
#include "ao.h"
#include "handlers.h"


#include "sedona.h"

volatile static float level2_ao = 0.0;
static float level2_ao_new = 0.0;
static bool priority_change_ao = false;
static unsigned int override_en_ao = 0; //tell you override status from BDT
static unsigned int override_en_bkp_ao = 0; //tell you override status from BDT
volatile static unsigned int priority_sae_ao = 0; //new pri level which is modified by sedona
volatile static unsigned int priority_bkp_ao = 0; // pri level comes from sedona (so taking backup for the next step)
volatile static unsigned int priority_act_ao = 255; //default value

static int ov_instance = -1;

//make it global
unsigned int object_index = 0;//TODO: whether we can use this as static
unsigned int priority = 0;
//make it to global
//BACNET_BINARY_PV level = BINARY_NULL;

static unsigned int dummy_ao = 0;
static unsigned int dummy_ao2 = 0;


#ifndef MAX_ANALOG_OUTPUTS
#define MAX_ANALOG_OUTPUTS 4
#endif

/* we choose to have a NULL level in our system represented by */
/* a particular value.  When the priorities are not in use, they */
/* will be relinquished (i.e. set to the NULL level). */
#define AO_LEVEL_NULL 255
/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define AO_RELINQUISH_DEFAULT 0
/* Here is our Priority Array.  They are supposed to be Real, but */
/* we don't have that kind of memory, so we will use a single byte */
/* and load a Real for returning the value when asked. */
//Titus
//static uint8_t Analog_Output_Level[MAX_ANALOG_OUTPUTS][BACNET_MAX_PRIORITY];

static float Analog_Output_Level[MAX_ANALOG_OUTPUTS][BACNET_MAX_PRIORITY];


/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Analog_Output_Out_Of_Service[MAX_ANALOG_OUTPUTS];

/* we need to have our arrays initialized before answering any calls */
static bool Analog_Output_Initialized = false;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Properties_Optional[] = {
    -1
};

static const int Properties_Proprietary[] = {
    -1
};

void Analog_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Properties_Required;
    if (pOptional)
        *pOptional = Properties_Optional;
    if (pProprietary)
        *pProprietary = Properties_Proprietary;

    return;
}

void Analog_Output_Init(
    void)
{
    unsigned i, j;

    if (!Analog_Output_Initialized) {
        Analog_Output_Initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_ANALOG_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Analog_Output_Level[i][j] = AO_LEVEL_NULL;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Output_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_ANALOG_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Output_Count(
    void)
{
    return MAX_ANALOG_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Output_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Output_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_ANALOG_OUTPUTS;

    if (object_instance < MAX_ANALOG_OUTPUTS)
        index = object_instance;

    return index;
}

float Analog_Output_Present_Value(
    uint32_t object_instance)
{
    float value = AO_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {

//	printf("Analog_Output_Level[%d][%d] %f\n",index,i,Analog_Output_Level[index][i]);

            if (Analog_Output_Level[index][i] != AO_LEVEL_NULL) {
                value = Analog_Output_Level[index][i];
                break;
            }
        }
    }

//Updating the float value and priority which are going to send to sedona.
	level2_ao = value;
	priority_act_ao = i;

//	printf("################Analog_Output_Present_Value priority_act_ao %d i %d level2_ao %f value %f!!! ################### \n",priority_act_ao,i,level2_ao,value);

    return value;
}

unsigned Analog_Output_Present_Value_Priority(
    uint32_t object_instance)
{

//	printf("################ Analog_Output_Present_Value_Priority!!! ################### \n");

    unsigned index = 0; /* instance to index conversion */
    unsigned i = 0;     /* loop counter */
    unsigned priority = 0;      /* return value */

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Analog_Output_Level[index][i] != AO_LEVEL_NULL) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Analog_Output_Present_Value_Set(
    uint32_t object_instance,
    float value,
    unsigned priority)
{

//	printf("################ Analog_Output_Present_Value_Set!!! ################### \n");

    unsigned index = 0;
    bool status = false;

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ ) &&
            (value >= 0.0) && (value <= 100.0)) {
//            Analog_Output_Level[index][priority - 1] = (uint8_t) value;
            Analog_Output_Level[index][priority - 1] = value;


//	printf("################ Analog_Output_Present_Value_Set!!! priority %d value %f  ################### \n",priority,value);
//Titus
//	 priority_act_ao = priority;

//Titus
//	if(override_en_ao == 1)
//	level2_ao = value;

//	printf("################ Analog_Output_Present_Value_Set!!! level2_ao %f priority_act_ao %d ################### \n",level2_ao,priority_act_ao);

            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

bool Analog_Output_Present_Value_Relinquish(
    uint32_t object_instance,
    unsigned priority)
{

//	printf("################ Analog_Output_Present_Value_Relinquish!!! ################### \n");

    unsigned index = 0;
    bool status = false;

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )) {
            Analog_Output_Level[index][priority - 1] = AO_LEVEL_NULL;
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

/* note: the object name must be unique within this device */
bool Analog_Output_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ANALOG_OUTPUTS) {
        sprintf(text_string, "ANALOG OUTPUT %lu",
            (unsigned long) object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Analog_Output_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{


//	printf("################ Analog_Output_Read_Property!!! ################### \n");


    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float) 1.414;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_OUTPUT,
                rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Analog_Output_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Analog_Output_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index =
                Analog_Output_Instance_To_Index(rpdata->object_instance);
            state = Analog_Output_Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], UNITS_PERCENT);
            break;
        case PROP_PRIORITY_ARRAY:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0)
                apdu_len =
                    encode_application_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                object_index =
                    Analog_Output_Instance_To_Index(rpdata->object_instance);
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (Analog_Output_Level[object_index][i] == AO_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        real_value = Analog_Output_Level[object_index][i];
                        len =
                            encode_application_real(&apdu[apdu_len],
                            real_value);
                    }
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU)
                        apdu_len += len;
                    else {
                        rpdata->error_class = ERROR_CLASS_SERVICES;
                        rpdata->error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = BACNET_STATUS_ERROR;
                        break;
                    }
                }
            } else {
                object_index =
                    Analog_Output_Instance_To_Index(rpdata->object_instance);
                if (rpdata->array_index <= BACNET_MAX_PRIORITY) {
                    if (Analog_Output_Level[object_index][rpdata->array_index -
                            1] == AO_LEVEL_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        real_value = Analog_Output_Level[object_index]
                            [rpdata->array_index - 1];
                        apdu_len =
                            encode_application_real(&apdu[0], real_value);
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            real_value = AO_RELINQUISH_DEFAULT;
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Analog_Output_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{

//	printf("################ Analog_Output_Write_Property!!! ################### \n");

    bool status = false;        /* return value */
//    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
	printf("BACNET: PROBE1 ################ wp_data->error_code %d ################### \n",ERROR_CODE_VALUE_OUT_OF_RANGE);
        return false;
    }
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
//Titus
                priority = wp_data->priority;
		if(priority < priority_sae_ao)
		{
//	printf("################ OVERRIDE occured for instance %d!!! ################### \n",wp_data->object_instance);
		ov_instance = wp_data->object_instance;
		override_en_ao = 1;
		}
		priority_bkp_ao = priority;

                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */

			if(override_en_ao == 1)
			{
//		printf("################ Updating the level in BDT as we received override; object_index %d priority %d value.type.Real %f ################### \n",object_index,priority,value.type.Real);
//                    Binary_Output_Level[object_index][priority] = level;

//Titus: Issue fixed
//                    Analog_Output_Present_Value_Set(wp_data->object_instance,
//                    value.type.Real, wp_data->priority);

                status =
                    Analog_Output_Present_Value_Set(wp_data->object_instance,
                    value.type.Real, wp_data->priority);

			}

                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (!status) {
	printf("BACNET: PROBE2 ################ wp_data->error_code %d ################### \n",ERROR_CODE_VALUE_OUT_OF_RANGE);
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_NULL,
                    &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    object_index =
                        Analog_Output_Instance_To_Index
                        (wp_data->object_instance);
                    status =
                        Analog_Output_Present_Value_Relinquish
                        (wp_data->object_instance, wp_data->priority);
                    if (!status) {
	printf("BACNET: PROBE3 ################ wp_data->error_code %d ################### \n",ERROR_CODE_VALUE_OUT_OF_RANGE);
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }

	//this line is required & important to update the value
	level2_ao = Analog_Output_Present_Value(wp_data->object_instance);
            break;

        case PROP_OUT_OF_SERVICE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                object_index =
                    Analog_Output_Instance_To_Index(wp_data->object_instance);
                Analog_Output_Out_Of_Service[object_index] =
                    value.type.Boolean;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_UNITS:
        case PROP_PRIORITY_ARRAY:
        case PROP_RELINQUISH_DEFAULT:
        case PROP_DESCRIPTION:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}


/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
Cell BACnet_BACnetDev_doBacnetAOValueStatus2(SedonaVM* vm, Cell* params)
{

//    printf("BACNET: BACnet_BACnetDev_doBacnetAOValueStatus2: params[0].ival : %d\n",params[0].ival);


	level2_ao_new = Analog_Output_Present_Value(params[0].ival);

	Cell result;
	result.fval = level2_ao_new;
	return result;

}


/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
BACnet_BACnetDev_doBacnetOverrideInst(SedonaVM* vm, Cell* params)
{

	return ov_instance;

}


/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
Cell BACnet_BACnetDev_doBacnetAOValueStatus(SedonaVM* vm, Cell* params)
{

	Cell result;
//    printf("BACNET: BACnet_BACnetDev_doBacnetAOValueStatus: Value : %f\n",level2_ao);
	result.fval = level2_ao;
	return result;

}


/* Titus : return to sedona what BDT gives (the Priority no will be returned) */
BACnet_BACnetDev_doBacnetAOPriorityStatus(SedonaVM* vm, Cell* params)
{
	priority_sae_ao = params[0].ival;

//	priority_change_ao = params[1].ival;

//    printf("BACnet_BACnetDev_doBacnetAOPriorityStatus: priority_sae_ao : %d  priority_act_ao : %d \n",priority_sae_ao,priority_act_ao);

	return priority_act_ao;
}

/* Titus : return if override happens */
BACnet_BACnetDev_doBacnetAOOverrideStatus(SedonaVM* vm, Cell* params)
{

	override_en_bkp_ao = override_en_ao;//backup the override event.

//Fixing temp
	override_en_ao = 0;//clear out override event.
	ov_instance = -1;



//    printf("BACnet_BACnetDev_doBacnetOverrideStatus: level2 : %d  override_en : %d  object_index %d priority %d \n",level2,override_en,object_index,priority);

	return override_en_bkp_ao;
}

/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
BACnet_BACnetDev_doBacnetAOValueUpdate(SedonaVM* vm, Cell* params)
{

	object_index = params[2].ival;//ObjectID

	if(dummy_ao == 0)
	{
	int i=0;
	printf("AO initialize is done!\n");
	dummy_ao++;
	priority_act_ao = 9;//default priority (@10)
//	Analog_Output_Level[object_index][--priority_sae_ao] = params[0].fval;//Float Value updating in BDT


/*
	Analog_Output_Level[1][9] = 0.0;//Init the second object
	Analog_Output_Level[2][9] = 0.0;//Init the third object
	Analog_Output_Level[3][9] = 0.0;//Init the fourth object
	Analog_Output_Level[4][9] = 0.0;//Init the fifth object
*/
	for(i=0;i<5;i++)
	Analog_Output_Level[i][9] = 0.0;//Init all the 5 objects

	}


	if(params[1].ival) {

//	printf("################ ALERT !!! WRITING by SAE! ################### object_index %d , priority_act %d value %f\n",object_index,priority_act_ao,params[0].fval);

	Analog_Output_Level[object_index][priority_act_ao] = params[0].fval;//Float Value updating in BDT

	}
}

#define NUM_OF_IDS 4
static int allowedIDs[NUM_OF_IDS] = { 0, 1, 2, 3};

bool isValidID(int which)
{
  int g;
  for (g=0; g<NUM_OF_IDS; g++)
    if (which == allowedIDs[g]) return TRUE;
  return FALSE;
}



Cell BACnet_BACnetDev_doBacnetInit2(SedonaVM* vm, Cell* params)
{

  int ID = params[0].ival;

  //
  // Check for invalid DIO
  //
  if ( !isValidID(ID) )
    return negOneCell;
  else
    return zeroCell;		

}


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

bool WPValidateArgType(
    BACNET_APPLICATION_DATA_VALUE * pValue,
    uint8_t ucExpectedTag,
    BACNET_ERROR_CLASS * pErrorClass,
    BACNET_ERROR_CODE * pErrorCode)
{
    pValue = pValue;
    ucExpectedTag = ucExpectedTag;
    pErrorClass = pErrorClass;
    pErrorCode = pErrorCode;

    return false;
}

void testAnalogOutput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    uint16_t decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Analog_Output_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ANALOG_OUTPUT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Analog_Output_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ANALOG_OUTPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Analog Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAnalogOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ANALOG_INPUT */
#endif /* TEST */
