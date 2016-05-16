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

/* Binary Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "rp.h"
#include "wp.h"
#include "bo.h"
#include "handlers.h"

#include "gpio.h"

int bo2gpio[MAX_BINARY_OUTPUTS]={5, 6, 13, 2};  //Titus : Modified for rPi2

#include "sedona.h"

static unsigned int level2 = 0;
static bool priority_change = false;
static unsigned int override_en = 0; //tell you override status from BDT
static unsigned int override_en_bkp = 0; //tell you override status from BDT
volatile static unsigned int priority_sae = 0; //new pri level which is modified by sedona
volatile static unsigned int priority_bkp = 0; // pri level comes from sedona (so taking backup for the next step)
volatile static unsigned int priority_act = 255; //default value
//volatile static unsigned int object_index = 0; // pri level comes from sedona (so taking backup for the next step)

static int ov_instance = -1;
static int level2_bo_new = 0;

//make it global
static unsigned int object_index = 0;//TODO: whether we can use this as static
static unsigned int priority = 0;
//make it to global
//BACNET_BINARY_PV level = BINARY_NULL;

static bool wp_en = false;

static unsigned int dummy = 0;


#ifndef MAX_BINARY_OUTPUTS
#define MAX_BINARY_OUTPUTS 4
#endif

/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define RELINQUISH_DEFAULT BINARY_INACTIVE
/* Here is our Priority Array.*/
static BACNET_BINARY_PV
    Binary_Output_Level[MAX_BINARY_OUTPUTS][BACNET_MAX_PRIORITY];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Out_Of_Service[MAX_BINARY_OUTPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Binary_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
    -1
};

static const int Binary_Output_Properties_Proprietary[] = {
    -1
};

void Binary_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Binary_Output_Properties_Required;
    if (pOptional)
        *pOptional = Binary_Output_Properties_Optional;
    if (pProprietary)
        *pProprietary = Binary_Output_Properties_Proprietary;

    return;
}

void Binary_Output_Init(
    void)
{
    unsigned i, j;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_BINARY_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Binary_Output_Level[i][j] = BINARY_NULL;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Binary_Output_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Binary_Output_Count(
    void)
{
    return MAX_BINARY_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Binary_Output_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Binary_Output_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_BINARY_OUTPUTS;

    if (object_instance < MAX_BINARY_OUTPUTS)
        index = object_instance;

    return index;
}

BACNET_BINARY_PV Binary_Output_Present_Value(
    uint32_t object_instance)
{
    BACNET_BINARY_PV value = RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

//printf("OVERRIDE STATUS override_en %d override_en_bkp %d Binary_Output_Level[0][9] %d  value %d\n",override_en,override_en_bkp,Binary_Output_Level[0][9], value);

    index = Binary_Output_Instance_To_Index(object_instance);

    if (index < MAX_BINARY_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
	 priority_act = i;
//	printf("index %d value %d i %d Binary_Output_Level[%d][%d] %d\n",index,value,i,index,i,Binary_Output_Level[index][i]);
            if (Binary_Output_Level[index][i] != BINARY_NULL) {
                value = Binary_Output_Level[index][i];
//	printf("SUCCESSFUL index %d value %d i %d Binary_Output_Level[%d][%d] %d\n",index,value,i,index,i,Binary_Output_Level[index][i]);

	//updating the "level2" var to sending the value to SAE.
	level2 = value;

//	printf("VALUE %d, sent to SAE! ObjectID %d\n",value,object_instance);
                break;
            }
        }
    }

    return value;
}

bool Binary_Output_Out_Of_Service(
    uint32_t object_instance)
{
    bool value = false;
    unsigned index = 0;

    index = Binary_Output_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_OUTPUTS) {
        value = Out_Of_Service[index];
    }

    return value;
}

/* note: the object name must be unique within this device */
bool Binary_Output_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_BINARY_OUTPUTS) {
        sprintf(text_string, "BINARY OUTPUT %lu",
            (unsigned long) object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Binary_Output_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    BACNET_POLARITY polarity = POLARITY_NORMAL;
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
                encode_application_object_id(&apdu[0], OBJECT_BINARY_OUTPUT,
                rpdata->object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Binary_Output_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:

           present_value =
                Binary_Output_Present_Value(rpdata->object_instance);

//	printf("present_value %d rpdata->object_instance %d \n",present_value,rpdata->object_instance);

            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index =
                Binary_Output_Instance_To_Index(rpdata->object_instance);
            state = Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(&apdu[0], polarity);
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
                    Binary_Output_Instance_To_Index(rpdata->object_instance);
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (Binary_Output_Level[object_index][i] == BINARY_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        present_value = Binary_Output_Level[object_index][i];
                        len =
                            encode_application_enumerated(&apdu[apdu_len],
                            present_value);
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
                    Binary_Output_Instance_To_Index(rpdata->object_instance);
                if (rpdata->array_index <= BACNET_MAX_PRIORITY) {
                    if (Binary_Output_Level[object_index][rpdata->array_index -
                            1] == BINARY_NULL)
                        apdu_len = encode_application_null(&apdu[apdu_len]);
                    else {
                        present_value = Binary_Output_Level[object_index]
                            [rpdata->array_index - 1];
                        apdu_len =
                            encode_application_enumerated(&apdu[apdu_len],
                            present_value);
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }

            break;
        case PROP_RELINQUISH_DEFAULT:
            present_value = RELINQUISH_DEFAULT;
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_ACTIVE_TEXT:
            characterstring_init_ansi(&char_string, "on");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_INACTIVE_TEXT:
            characterstring_init_ansi(&char_string, "off");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
bool Binary_Output_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */

//done as global
//    unsigned int object_index = 0;
//    unsigned int priority = 0;
BACNET_BINARY_PV level = BINARY_NULL;


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
        return false;
    }
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                priority = wp_data->priority;



		override_en=0;//clearing the flag; wp_data->priority is the priority from BDT and it is BOSS for level 1 to 9 priority.
		if(priority < priority_sae)
		{
//	printf("################ OVERRIDE occured for instance %d!!! ################### \n",wp_data->object_instance);
		ov_instance = wp_data->object_instance;

		override_en=1;
		}

		priority_bkp = priority;

                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (priority && (priority <= BACNET_MAX_PRIORITY) &&
                    (priority != 6 /* reserved */ ) &&
                    (value.type.Enumerated <= MAX_BINARY_PV)) {
                    level = (BACNET_BINARY_PV) value.type.Enumerated;

                    object_index =
                        Binary_Output_Instance_To_Index
                        (wp_data->object_instance);
                    priority--;

			if(override_en == 1)
			{
		printf("################ Updating the level in BDT as we received override; object_index %d priority %d ################### \n",object_index,priority);
                    Binary_Output_Level[object_index][priority] = level;
			}

                    /* Note: you could set the physical output here if we
                       are the highest priority.
                       However, if Out of Service is TRUE, then don't set the
                       physical output.  This comment may apply to the
                       main loop (i.e. check out of service before changing output) */
                    status = true;
                } else if (priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_NULL,
                    &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    level = BINARY_NULL;
                    object_index =
                        Binary_Output_Instance_To_Index
                        (wp_data->object_instance);
                    priority = wp_data->priority;
                    if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                        priority--;
                        Binary_Output_Level[object_index][priority] = level;
                        /* Note: you could set the physical output here to the next
                           highest priority, or to the relinquish default if no
                           priorities are set.
                           However, if Out of Service is TRUE, then don't set the
                           physical output.  This comment may apply to the
                           main loop (i.e. check out of service before changing output) */
                    } else {
                        status = false;
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
		//this line is required & important to update the value
		level2 = Binary_Output_Present_Value(object_index);

            break;
        case PROP_OUT_OF_SERVICE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                object_index =
                    Binary_Output_Instance_To_Index(wp_data->object_instance);
                Out_Of_Service[object_index] =
                    value.type.Boolean;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
        case PROP_EVENT_STATE:
        case PROP_POLARITY:
        case PROP_PRIORITY_ARRAY:
        case PROP_RELINQUISH_DEFAULT:
        case PROP_ACTIVE_TEXT:
        case PROP_INACTIVE_TEXT:
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
BACnet_BACnetDev_doBacnetBOOverrideInst(SedonaVM* vm, Cell* params)
{

	return ov_instance;

}


/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
Cell BACnet_BACnetDev_doBacnetBOValueStatus2(SedonaVM* vm, Cell* params)
{

//    printf("BACNET: BACnet_BACnetDev_doBacnetAOValueStatus2: params[0].ival : %d\n",params[0].ival);


	level2_bo_new = Analog_Output_Present_Value(params[0].ival);

	Cell result;
	result.ival = level2_bo_new;
	return result;

}



/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
Cell BACnet_BACnetDev_doBacnetBOValueStatus(SedonaVM* vm, Cell* params)
{


//    printf("BACnet_BACnetDev_doBacnetValueStatus: Value : %d  override_en : %d \n",level2,override_en);
//	return level2;
	if(level2 == 0)
	return zeroCell;

	if(level2 == 1)
	return oneCell;

}

/* Titus : return to sedona what BDT gives (the GPIO no will be returned) */
BACnet_BACnetDev_doBacnetBOPriorityStatus(SedonaVM* vm, Cell* params)
{
	priority_sae = params[0].ival;
	priority_change = params[1].ival;
	return priority_act;
}

/* Titus : return if override happens */
BACnet_BACnetDev_doBacnetBOOverrideStatus(SedonaVM* vm, Cell* params)
{

	override_en_bkp = override_en;//backup the override event.
	override_en = 0;//clear out override event.

	ov_instance = -1;

//    printf("BACnet_BACnetDev_doBacnetOverrideStatus: level2 : %d  override_en : %d  object_index %d priority %d \n",level2,override_en,object_index,priority);

	return override_en_bkp;
}

/* Titus : return to sedona what BDT gives (value which needs to be written into GPIO) */
BACnet_BACnetDev_doBacnetBOValueUpdate(SedonaVM* vm, Cell* params)
{

	object_index = params[2].ival;//ObjectID

	if(dummy == 0)
	{
	int i=0;
	printf("BO initialize is done!\n");
	dummy++;
	priority_act = 9;//default priority (@10)

	for(i=0;i<5;i++)
	Binary_Output_Level[i][9] = 0;//Init all the 5 objects


	}

	if(params[1].ival) {
//	printf("################ ALERT !!! WRITING by SAE! ################### object_index %d , priority_act %d value %d \n",object_index,priority_act,params[0].ival);
	Binary_Output_Level[object_index][priority_act] = params[0].ival;//Value updating in BDT
	}

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

void testBinaryOutput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint16_t decoded_type = 0;
    uint32_t decoded_instance = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Binary_Output_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_BINARY_OUTPUT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Binary_Output_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_BINARY_OUTPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Binary Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBinaryOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BINARY_INPUT */
#endif /* TEST */
