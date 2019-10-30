/*  nutdrv_mqtt.c - Driver for MQTT
 *
 *  Copyright (C)
 *    2015  Arnaud Quette <ArnaudQuette@Eaton.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * TODO list:
 * - everything
 * - data mapping + use in mqtt_message_callback()
 * - driver completion as per drivers/skel.c and others (shutdown, instcmd,
 *   setvar, ...)
 */

#include "main.h"
#include "nutdrv_mqtt.h"
#include "nutdrv_mqtt_map_sensor.h"
#include "nutdrv_mqtt_map_alarm.h"
#include <openssl/ssl.h>
#include <mosquitto.h>
#include <json-c/json.h>
#include <regex.h>

/*-------------------------------------------------------------*/
/* driver description structure                                */
/*-------------------------------------------------------------*/

upsdrv_info_t upsdrv_info = {
	DRIVER_NAME,
	DRIVER_VERSION,
	"Arnaud Quette <ArnaudQuette@Eaton.com>",
	DRV_EXPERIMENTAL,
	{ NULL }
};

typedef struct
{
	const char *in;
	const char *out;
    int (*process_ptr)(const char *name, const char *value, char **params, int nb_params);
} nm2_pairs;

typedef struct {
	int level;
	const char *value;
} object_level_t;

typedef struct {
	int level;
    int high;
	const char *value;
} extended_object_level_t;

typedef struct
{
    const char *code;
	const char *topic;
    const char *object_name;
    const char *alarm_value;
    const char *status_value;
    int (*alarm_object_ptr)(alarm_t *alarm, const char *value, int alarm_state, char **params, int nb_params);
    int (*alarm_ptr)(alarm_t *alarm, const char *value, int alarm_state, char **params, int nb_params);
    int (*alarm_status_ptr)(alarm_t *alarm, const char *value, int alarm_state, char **params, int nb_params);
} alarm_nut_t;


/*-------------------------------------------------------------*/
/* Variables                                                   */
/*-------------------------------------------------------------*/

struct mosquitto *mosq = NULL;
device_map_t *list_device_root = NULL;
alarm_map_t *list_alarm_root = NULL;
alarm_t *current_alarm_array = NULL;
int nb_current_alarm_array = -1;

const char *type_sensor_name[] = {
    "temperature",
    "humidity",
    "contacts"
};

const char *yes_no_value[] = {
    "yes",
    "no"
};

const char *ambient_object_name[] =  {

    // TBD: TO REMOVE (FOR TEST)
    "ambient.%d.key",

    "ambient.%d.firmware",
    "ambient.%d.mfr",
    "ambient.%d.model",
    "ambient.%d.name",
    "ambient.%d.present",
    "ambient.%d.serial",
    "ambient.%d.temperature.name",
    "ambient.%d.temperature.low.critical",
    "ambient.%d.temperature.low.warning",
    "ambient.%d.temperature.high.critical",
    "ambient.%d.temperature.high.warning",
    "ambient.%d.temperature.status",
    "ambient.%d.temperature.unit",
    "ambient.%d.temperature",
    "ambient.%d.humidity.name",
    "ambient.%d.humidity.low.critical",
    "ambient.%d.humidity.low.warning",
    "ambient.%d.humidity.high.critical",
    "ambient.%d.humidity.high.warning",
    "ambient.%d.humidity",
    "ambient.%d.contacts.1.name",
    "ambient.%d.contacts.1.config",
    "ambient.%d.contacts.1.status",
    "ambient.%d.contacts.2.name",
    "ambient.%d.contacts.2.config",
    "ambient.%d.contacts.2.status"
};

static object_level_t basic_status_info[] = {
    { 4, "good" },     // No threshold triggered
    { 5, "info" },     // info threshold triggered
	{ 6, "warning" },  // Warning threshold triggered
	{ 7, "critical" },  // Critical threshold triggered
    { 0, NULL }
};

static extended_object_level_t extended_status_info[] = {
	{ 0, 0, "good" },          // No threshold triggered
	{ 6, 0, "warning-low" },   // Warning low threshold triggered
	{ 7, 0, "critical-low" },  // Critical low threshold triggered
	{ 6, 1, "warning-high" },  // Warning high threshold triggered
	{ 7, 1, "critical-high" }, // Critical high threshold triggered
    { 0, 0, NULL }
};

static extended_object_level_t temperature_alarms_info[] = {
	{ 6, 0, "low temperature warning!" },   // Warning low threshold triggered
	{ 7, 0, "low temperature critical!" },  // Critical low threshold triggered
	{ 6, 1, "high temperature warning!" },  // Warning high threshold triggered
	{ 7, 1, "high temperature critical!" }, // Critical high threshold triggered
    { 0, 0, NULL }
};

static extended_object_level_t humidity_alarms_info[] = {
	{ 6, 0, "low humidity warning!" },   // Warning low threshold triggered
	{ 7, 0, "low humidity critical!" },  // Critical low threshold triggered
	{ 6, 1, "high humidity warning!" },  // Warning high threshold triggered
	{ 7, 1, "high humidity critical!" }, // Critical high threshold triggered
    { 0, 0, NULL }
};

static object_level_t contacts_alarms_info[] = {
	{ 6, "dry contact warning!" },  // Warning triggered
	{ 7, "dry contact critical!" },  // Critical triggered
    { 0, NULL }
};

/*-------------------------------------------------------------*/
/* Driver functions                                            */
/*-------------------------------------------------------------*/

/* Callbacks */
void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result);
void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result);
void mqtt_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str);
void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void mqtt_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void mqtt_reconnect();

/**
 * @function get_basic_value Search and return value in structure according level value
 * @param object_levelThe structure in which to search
 * @param level Level value to search
 * @return {string} value found
 */
static const char *get_basic_value(object_level_t *object_level, int level) {
    if (!object_level) return NULL;

    object_level_t *it = object_level;
    while (it->value) {
        if (it->level == level) {
            return it->value;
        }
        it ++;
    }
    return NULL;
}

/**
 * @function get_extended_value Search and return value in structure according level and code values
 * @param object_level The structure in which to search
 * @param level Level value to search
 * @param code Code value to search
 * @return {string} value found
 */
static const char *get_extended_value(extended_object_level_t *extended_object_level, int level, char *code) {
    if (!(extended_object_level)) return NULL;

    /* Temperature is warning low : 1202 */
    /* Temperature is critical low : 1201 */
    /* Temperature is warning high : 1203 */
    /* Temperature is critical high : 1204 */
    /* Humidity is warning low : 1212 */
    /* Humidity is critical low : 1211 */
    /* Humidity is warning high : 1213 */
    /* Humidity is critical high : 1214 */
    int high = code && ((strcmp(code , "1203") == 0) || (strcmp(code , "1213") == 0) ||
               (strcmp(code , "1204") == 0) || (strcmp(code , "1214") == 0)) ? 1 : 0;
    extended_object_level_t *it = extended_object_level;
    while (it->value) {
        if (it->level == level &&
           (it->level == 0 || it->high == high)) {
            return it->value;
        }
        it ++;
    }
    return NULL;
}

/**
 * @function nut_remove_device Remove all nut sensor objects according sensor device index
 * @param index_device The index of the sensor device
 * @return {integer} 0 if success else < 0
 */
int nut_remove_device(int index_device) {
    if (index_device < 0) return -1;

    char *object_name = NULL;
    uint iObject;
    for (iObject = 0; iObject < sizeof(ambient_object_name) / sizeof(char *) ; iObject++) {
        if (asprintf(&object_name, ambient_object_name[iObject], index_device + 1) != -1) {
            dstate_delinfo(object_name);
            upsdebugx(2, "Remove %s", object_name);
            free(object_name);
        }
    }
    return 0;
}

/**
 * @function nut_move_device Move all nut sensor objects from a position to another one
 * @param index_device The index of the sensor device to move
 * @param new_index_device The new index of the sensor device which move
 * @return {integer} 0 if success else < 0
 */
int nut_move_device(int index_device, int new_index_device) {
    if (!(index_device >= 0 && new_index_device >=0)) return -1;

    char *object_name = NULL;
    char *new_object_name = NULL;
    uint iObject;
    for (iObject = 0; iObject < sizeof(ambient_object_name) / sizeof(char *) ; iObject++) {
        if (asprintf(&object_name, ambient_object_name[iObject], index_device + 1) != -1 &&
            asprintf(&new_object_name, ambient_object_name[iObject], new_index_device + 1) != -1) {
            const char *object_value = dstate_getinfo(object_name);
            upsdebugx(2, "%s -> %s\n", object_name, object_value);
            if (object_value) {
                upsdebugx(2, "Change %s -> %s: %s", object_name, new_object_name, object_value);
                dstate_setinfo(new_object_name, "%s", object_value);
                dstate_delinfo(object_name);
            }
        }
        if (object_name) free(object_name);
        if (new_object_name) free(new_object_name);
    }
    return 0;
}

// TBD: TO REMOVE
/*int workaround_device_not_found(char *key_device) {
    if (map_add_device(list_device_root, -1, key_device) != 0) {
        return -1;
    }
    return map_get_index_device(list_device_root, key_device);
}

int workaround_sensor_not_found(int index_device, char *key_sensor) {
    //if (map_add_device(list_device_root, -1, key_device) != 0) {
    //    return -1;
    //}
    //return map_find_index_device(list_device_root, key_device);
    return 0;
} */

/**
 * @function process_contact_config (Callback) Process configuration for digitals contacts
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of parameters
 * @return {integer} 0 if success else < 0
 */
int process_contact_config(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    const char *contact_config[] = {
        "normal-opened",
        "normal-closed"
    };

    int index_device = map_get_index_device(list_device_root, params[0]);
    int index_sensor = map_get_index_sensor(list_device_root, index_device, TYPE_INPUTS, params[1]);
    if (index_device >= 0 && index_sensor >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_device + 1, index_sensor + 1) != -1) {
            uint index = atoi(value);
            if(index >= 0 && index < sizeof(contact_config) / sizeof(char *)) {
                dstate_setinfo(new_name, "%s", (char *)contact_config[index]);
                upsdebugx(2, "Convert %s value %s -> %s", new_name, value, (char *)contact_config[index]);
                free(new_name);
                return 0;
            }
            free(new_name);
        }
    }
    return -3;
}

/**
 * @function process_contact_status (Callback) Process status for digitals contacts
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_contact_status(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    const char *contact_status[] = {
        "active",
        "inactive"
    };

    int index_device = map_get_index_device(list_device_root, params[0]);
    int index_sensor = map_get_index_sensor(list_device_root, index_device, TYPE_INPUTS, params[1]);
    if (index_device >= 0 && index_sensor >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_device + 1, index_sensor + 1) != -1) {
            uint index = atoi(value);
            if(index >= 0 && index < sizeof(contact_status) / sizeof(char *)) {
                dstate_setinfo(new_name, "%s", (char *)contact_status[index]);
                upsdebugx(2, "Convert %s value %s -> %s", new_name, value, (char *)contact_status[index]);
                free(new_name);
                return 0;
            }
            free(new_name);
        }
    }
    return -3;
}

/**
 * @function process_temperature_value (Callback) Process value for temperature sub-sensor
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_temperature_value(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    int index_device = map_get_index_device(list_device_root, params[0]);
    if (index_device >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_device + 1) != -1) {
            double double_value = atof(value) - 273.15;
            dstate_setinfo(new_name, "%.2lf", double_value);
            upsdebugx(2, "Convert %s value %s -> %.2lf", new_name, value, double_value);
            free(new_name);
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_data_device (Callback) Process data for sensor device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_data_device(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 1) return -2;

    int index_device = map_get_index_device(list_device_root, params[0]);
    if (index_device >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_device + 1) != -1) {
            dstate_setinfo(new_name, "%s", value);
            upsdebugx(2, "Convert %s value %s", new_name, value);
            free(new_name);
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_present_device (Callback) Process present object for sensor device into a device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_present_device(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 1) return -2;

    int index_device = map_get_index_device(list_device_root, params[0]);
    if (index_device >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_device + 1) != -1) {
            /* 0: Unknown */
            /* 1: Not available */
            /* 2: Communication OK */
            /* 3: Communication lost */
            /* 4: Not initiated / No contact */
            const char *new_value = (strcmp(value, "2") == 0) ? yes_no_value[0] : yes_no_value[1];
            dstate_setinfo(new_name, "%s", new_value);
            upsdebugx(2, "Convert %s value %s", new_name, new_value);
            free(new_name);
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_data_sensor (Callback) Process data for sub-sensor into a device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_data_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 3) return -2;

    char *new_name = NULL;
    int type_sensor = map_get_index_type_sensor(params[1]);
    if (map_is_index_type_sensor(type_sensor)) {
        int index_device = map_get_index_device(list_device_root, params[0]);
        int index_sensor = map_get_index_sensor(list_device_root, index_device, type_sensor, params[2]);
        if (index_device >= 0 && index_sensor >= 0) {
            if (asprintf(&new_name, name, index_device + 1, type_sensor_name[type_sensor], index_sensor + 1) != -1) {
                dstate_setinfo(new_name, "%s", value);
                upsdebugx(2, "Convert %s value %s", new_name, value);
                free(new_name);
                return 0;
            }
        }
    }
    return -3;
}

/**
 * @function process_enable_sensor (Callback) Process enable data for sub-sensor into a device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_enable_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 3) return -2;

    const char *ambient_object_name[] =  {
        "ambient.%d.%s"
        /* TBD: Not notified when sensor is enabled
        "ambient.%d.%s.name",
        "ambient.%d.%s.low.critical",
        "ambient.%d.%s.low.warning",
        "ambient.%d.%s.high.critical",
        "ambient.%d.%s.high.warning" */
    };

    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor == TYPE_TEMPERATURE || type_sensor == TYPE_HUMIDITY) {
        int index_device = map_get_index_device(list_device_root, params[0]);
        int index_sensor = map_get_index_sensor(list_device_root, index_device, type_sensor, params[2]);
        if (index_device >= 0 && index_sensor >= 0) {
            int enable = strcmp(value, "1") == 0 ? 1 : 0;
            map_set_sensor_state(list_device_root, index_device, type_sensor, index_device, enable);
            if (!enable) {
                char *object_name = NULL;
                uint iObject;
                for (iObject = 0; iObject < sizeof(ambient_object_name) / sizeof(char *) ; iObject++) {
                    if (asprintf(&object_name, ambient_object_name[iObject],
                        index_device + 1, type_sensor_name[type_sensor], index_sensor + 1) != -1) {
                        dstate_delinfo(object_name);
                        upsdebugx(2, "Remove %s", object_name);
                        free(object_name);
                    }
                }
            }
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_enable_contacts (Callback) Process enable data for digitals contacts
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_enable_contacts(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 3) return -2;

    const char *ambient_contacts_name[] =  {
        /* TBD: Not notified when sensor is enabled ??? */
        "ambient.%d.contacts.%d.status"
        /* TBD: Not notified when sensor is enabled
        "ambient.%d.contacts.%d.name",
        "ambient.%d.contacts.%d.config" */
    };
    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor == TYPE_INPUTS) {
        int index_device = map_get_index_device(list_device_root, params[0]);
        int index_sensor = map_get_index_sensor(list_device_root, index_device, type_sensor, params[2]);
        if (index_device >= 0 && index_sensor >= 0) {
            int enable = strcmp(value, "1") == 0 ? 1 : 0;
            map_set_sensor_state(list_device_root, index_device, type_sensor, index_device, enable);
            if (!enable) {
                char *object_name = NULL;
                uint iObject;
                for (iObject = 0; iObject < sizeof(ambient_contacts_name) / sizeof(char *) ; iObject++) {
                    if (asprintf(&object_name, ambient_contacts_name[iObject], index_device + 1, index_sensor + 1) != -1) {
                        dstate_delinfo(object_name);
                        upsdebugx(2, "Remove %s", object_name);
                        free(object_name);
                    }
                }
            }
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_device (Callback) Process number of sensor device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_device(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;

    int nb_init_device = atoi(value);
    if (map_set_nb_init_devices(list_device_root, nb_init_device) == 0) {
        dstate_setinfo("ambient.count", "%d", nb_init_device);
        if (nb_init_device == 0) {
            int iDevice;
            int nb_device = map_get_nb_devices(list_device_root);
            for (iDevice = 0; iDevice < nb_device; iDevice++) {
                nut_remove_device(iDevice);
            }
            map_remove_all_devices(list_device_root);
        }
        return 0;
    }
    return -2;
}

/**
 * @function process_device (Callback) Process index of sensor device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_index_device(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 1) return -2;

    int res = 0;

    char *pch = strrchr(value, '/');
    if (!(pch && (uint)(pch - value + 1) < strlen(value))) {
        return -3;
    }
    char *key_device = pch + 1;
    int position_device = atoi(params[0]);
    int index_device = map_get_index_device(list_device_root, key_device);
    /* if device exist, move it if it is not the good place */
    if (index_device >=0) {
        /* If not at the good place */
        if (position_device != index_device) {
            if (map_move_device(list_device_root, index_device, position_device) != 0) {
                res = -4;
            }
            /* move nut device */
            if (nut_move_device(index_device, position_device) != 0) {
                res = -5;
            }
        }
        /* else do nothing */
    }
    /* else device not existed, need to added it */
    else {
        if (map_add_device(list_device_root, position_device, key_device) != 0) {
            res = -6;
        }

        /* TBD: TO REMOVE (FOR TEST) */
        char *object_name = NULL;
        if (asprintf(&object_name, "ambient.%d.key", position_device + 1) != -1) {
            dstate_setinfo(object_name, "%s", key_device);
            free(object_name);
        }
    }
    /* test if some device need to be removed */
    int nb_init_device = map_get_nb_init_devices(list_device_root);
    int nb_device = map_get_nb_devices(list_device_root);
    printf("nb_device=%d nb_init_device=%d\n", nb_device, nb_init_device);
    if (res == 0 && nb_init_device >=0 && nb_device >= 0 && position_device == nb_init_device -1)
    {
        int iDevice;
        for (iDevice = position_device + 1; res >=0 && iDevice < nb_device; iDevice++) {
            printf("Remove iDevice=%d position_device=%d\n", iDevice, position_device);
            if (map_remove_device(list_device_root, iDevice) != 0) {
                res = -7;
            }
            /* remove nut device */
            if (nut_remove_device(iDevice) != 0) {
                res = -8;
            }
        }
    }
    return res;
}

/**
 * @function process_device (Callback) Process number of sub-sensor into a device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 2) return -2;

    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor >= 0) {
        char *device_key = params[0];
        if (map_add_sensors(list_device_root, device_key, type_sensor, atoi(value)) == 0) {
            return 0;
        }
    }
    return -3;
}

/**
 * @function process_device (Callback) Process index of sub-sensor into a device
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_index_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 3) return -2;

    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor >= 0) {
        char *pch = strrchr(value, '/');
        if (pch && (uint)(pch - value + 1) < strlen(value)) {
            char *sensor_key = pch + 1;
            char *device_key = params[0];
            if (map_init_sensor(list_device_root, device_key, type_sensor, atoi(params[2]), sensor_key) == 0) {
                /* Set default value for sensor status ("good") */
                if (type_sensor == TYPE_TEMPERATURE || type_sensor == TYPE_HUMIDITY) {
                    int index_device = map_get_index_device(list_device_root, device_key);
                    if (index_device >= 0) {
                        char *object_name = NULL;
                        if (asprintf(&object_name, "ambient.%d.%s.status", index_device + 1, type_sensor_name[type_sensor]) != -1) {
                            const char *value = get_extended_value(extended_status_info, 0, NULL);
                            dstate_setinfo(object_name, "%s", value);
                            upsdebugx(2, "Set %s -> %s", object_name, value);
                            free(object_name);
                        }
                    }
                }
                return 0;
            }
        }
    }
    return -3;
}

/**
 * @function alarm_object_default (Callback) Default treatment for alarm object
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_object_default(alarm_t *alarm, const char *object_name, int alarm_state, char **params, int nb_params) {
    if (!(alarm && object_name)) return -1;
    if (!params || nb_params != 2) return -2;

    const char *value = get_basic_value(basic_status_info, alarm->level);
    dstate_setinfo(object_name, "%s", value);
    upsdebugx(2, "Alarm %s -> %s", object_name, value);
    return 0;
}

/**
 * @function alarm_object_default (Callback) Default treatment for alarm
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_default(alarm_t *alarm, const char *alarm_value, int alarm_state, char **params, int nb_params) {
    if (!(alarm && alarm_value)) return -1;
    if (!params || nb_params != 2) return -2;

    status_set(alarm_value);
    upsdebugx(2, "Add alarm %s", alarm_value);
    return 0;
}

/**
 * @function alarm_status_default (Callback) Default treatment for alarm status
 * @param alarm Alarm object
 * @param status_value Status value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_status_default(alarm_t *alarm, const char *status_value, int alarm_state, char **params, int nb_params) {
    if (!(alarm && status_value)) return -1;
    if (!params || nb_params != 2) return -2;

    alarm_set(status_value);
    upsdebugx(2, "Add status alarm %s", status_value);
    return 0;
}

/**
 * @function alarm_object_contact (Callback) Special treatment for digitals contacts alarm object
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_object_contact(alarm_t *alarm, const char *object_name, int alarm_state, char **params, int nb_params) {
    if (!(alarm && object_name)) return -1;
    if (!params || nb_params != 2) return -2;

    int index_device = map_get_index_device(list_device_root, params[0]);
    int index_sensor = map_get_index_sensor(list_device_root, index_device, TYPE_INPUTS, params[1]);
    if (index_device >= 0 && index_sensor >= 0) {
        char *new_object_name = NULL;
        if (asprintf(&new_object_name, object_name, index_device + 1, index_sensor + 1) != -1) {
            const char *value = get_basic_value(basic_status_info, (alarm_state == ALARM_NEW) ? alarm->level : 0);
            dstate_setinfo(new_object_name, "%s", value);
            upsdebugx(2, "Alarm %s -> %s", new_object_name, value);
            free(new_object_name);
            return 0;
        }
    }
    return -3;
}

/**
 * @function alarm_object_contact (Callback) Special treatment for sub-sensor alarm object
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_object_sensor(alarm_t *alarm, const char *object_name, int alarm_state, char **params, int nb_params) {
    if (!(alarm && object_name)) return -1;
    if (!params || nb_params != 3) return -2;

    char *new_object_name = NULL;
    int type_sensor = map_get_index_type_sensor(params[1]);
    if (map_is_index_type_sensor(type_sensor)) {
        int index_device = map_get_index_device(list_device_root, params[0]);
        int index_sensor = map_get_index_sensor(list_device_root, index_device, type_sensor, params[2]);
        if (index_device >= 0 && index_sensor >= 0) {
            if (asprintf(&new_object_name, object_name, index_device + 1, type_sensor_name[type_sensor], index_sensor + 1) != -1) {
                const char *value = NULL;
                if (alarm_state == ALARM_NEW) value = get_extended_value(extended_status_info, alarm->level, alarm->code);
                else value = get_extended_value(extended_status_info, 0, 0);
                dstate_setinfo(new_object_name, "%s", value);
                upsdebugx(2, "Alarm %s value %s", new_object_name, value);
                free(new_object_name);
                return 0;
            }
        }
    }
    return -3;
}

/**
 * @function alarm_object_contact (Callback) Special treatment for sub-sensor alarm
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_sensor(alarm_t *alarm, const char *object_name, int alarm_state, char **params, int nb_params) {
    if (!alarm) return -1;
    if (!params || nb_params != 3) return -2;

    int type_sensor = map_get_index_type_sensor(params[1]);
    if (map_is_index_type_sensor(type_sensor)) {
        int index_device = map_get_index_device(list_device_root, params[0]);
        int index_sensor = map_get_index_sensor(list_device_root, index_device, type_sensor, params[2]);
        if (index_device >= 0 && index_sensor >= 0) {
            const char *alarm_value = NULL;
            extended_object_level_t *extended_object_level = (type_sensor == TYPE_TEMPERATURE) ?
                temperature_alarms_info : humidity_alarms_info;
            alarm_value = get_extended_value(extended_object_level, alarm->level, alarm->code);
            if (alarm_value) {
                alarm_set(alarm_value);
                upsdebugx(2, "Alarm value %s", alarm_value);
            }
            return 0;
        }
    }
    return -3;
}

/**
 * @function alarm_object_contact (Callback) Special treatment for digitals contacts alarm
 * @param alarm Alarm object
 * @param object_name Object name value
 * @param alarm_state Alarm state
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int alarm_contact(alarm_t *alarm, const char *object_name, int alarm_state, char **params, int nb_params) {
    if (!alarm) return -1;
    if (!params || nb_params != 2) return -2;

    int index_device = map_get_index_device(list_device_root, params[0]);
    int index_sensor = map_get_index_sensor(list_device_root, index_device, TYPE_INPUTS, params[1]);
    if (index_device >= 0 && index_sensor >= 0) {
        const char *alarm_value = get_basic_value(contacts_alarms_info, alarm->level);
        if (alarm_value) {
            alarm_set(alarm_value);
            upsdebugx(2, "Alarm value %s", alarm_value);
        }
        return 0;
    }
    return -3;
}

/**
 * nm2 alarms mapping
 */
const alarm_nut_t s_alarm_mapping[] =  {
    // error codes list, topic name, alarm object name (fac.), alarm value (fac.), status value (fac.), alarm object callback (fac.), alarm callback (fac.), alarm status callback (fac.)
    { "705",  "^mbdetnrs/1.0/([^/]+)$", NULL, "Output overload!", "OVER", NULL, NULL, NULL },
    // FOR TEST { "A0F",  "^mbdetnrs/1.0/powerDistributions/1/outlets/([^/]+)$", "outlet.%d.status", NULL, NULL, &alarm_object_outlet, &alarm_outlet, NULL },
    { "1200", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1201", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1203", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1204", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1211", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1212", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1213", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1214", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)$", "ambient.%d.%s.status", NULL, NULL, &alarm_object_sensor, &alarm_sensor, NULL },
    { "1221", "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/digitalInputs/([^/]+)$", "ambient.%d.contacts.%d.alarm", NULL, NULL, &alarm_object_contact, &alarm_contact, NULL/*&alarm_contact, alarm_status_contact*/ },

    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/**
 * @function manage_alarm Manage an alarm
 * @param alarm Alarm object
 * @param alarm_state Alarm state
 * @return {integer} 0 if success else < 0
 */
int manage_alarm(alarm_t *alarm, int alarm_state) {
    if (!alarm) return -1;

    regex_t preg;
    regmatch_t match[MAX_SUB_EXPR];

    const alarm_nut_t *it = s_alarm_mapping;
    while (it->code) {
        if (map_alarm_code_find(it->code, alarm->code)) {
            char **params = NULL;
            uint nb_params = 0;
            if (it->topic) {
                int r = regcomp(&preg, it->topic, REG_EXTENDED);
                if (r == 0) {
                    r = regexec(&preg, alarm->topic, sizeof(match)/sizeof(match[0]), (regmatch_t*)&match, 0);
                    printf("Result: %s %d\n", alarm->topic, r);
                    if (r == 0) {
                        printf("preg.re_nsub=%zu\n", preg.re_nsub);
                        if (preg.re_nsub > 0) {
                            nb_params = preg.re_nsub;
                            params = malloc(sizeof(char *) * nb_params);
                            uint i;
                            for (i = 0; i < nb_params; i++) {
                                params[i] = strndup(alarm->topic + match[i + 1].rm_so, match[i + 1].rm_eo - match[i + 1].rm_so);
                                printf("%u: param=%s\n", i, params[i]);
                            }
                        }
                    }
                }
                else {
                   printf("Result ERROR: %s %d\n", it->topic, r);
                }
            }

            /* update object alarm only if new alarm or alarm is inactive */
            if (alarm_state != ALARM_ACTIVE) {
                if (it->alarm_object_ptr != NULL) {
                    int r = it->alarm_object_ptr(alarm, it->object_name, alarm_state, params, nb_params);
                    if (r < 0) {
                        printf("********************** Error during alarm treatment for %s: %d\n", it->object_name, r);
                        //upsdebugx(2, "Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                    }
                }
            }
            /* update alarm and status alarm only if new alarm or if alarm is active */
            if (alarm_state != ALARM_INACTIVE) {
                /* If special treatment for alarm (ups.alarm) */
                if (it->alarm_ptr != NULL) {
                    int r = it->alarm_ptr(alarm, it->alarm_value, alarm_state, params, nb_params);
                    if (r < 0) {
                        printf("********************** Error during alarm treatment for %s: %d\n", it->alarm_value, r);
                        //upsdebugx(2, "Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                    }
                }
                /* else default treatment for alarm (ups.alarm) */
                else {

                }
                /* if special treatment for status alarm (ups.status) */
                if (it->alarm_status_ptr != NULL) {
                    int r = it->alarm_ptr(alarm, it->status_value, alarm_state, params, nb_params);
                    if (r < 0) {
                        printf("********************** Error during alarm treatment for %s: %d\n", it->status_value, r);
                        //upsdebugx(2, "Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                    }
                }
                /* else default treatment for status alarm (ups.status) */
                else {

                }
            }
            uint i;
            for (i = 0; i < nb_params; i++) {
                if (params[i]) free(params[i]);
            }
            if (params) free(params);
            regfree(&preg);
        }
        it ++;
    }
    return 0;
}

/**
 * @function Alarms reception treatment
 * @return {integer} 0 if success else < 0
 */
int process_alarms() {
    int res = 0;
    int iAlarm;
    int alarm_state;
    int alarm_count = 0;

    printf("process_alarm: DEBUT\n");
    if (nb_current_alarm_array == -1) return 0;

    /* init ups.alarm */
    alarm_init();
    /* init ups.status */
    status_init();

    /* test first if new alarms appear */
    if (current_alarm_array && nb_current_alarm_array > 0) {
        for (iAlarm = 0; iAlarm < nb_current_alarm_array; iAlarm++) {
            printf("process_alarm: id=%s\n", current_alarm_array[iAlarm].id);
            if (!map_find_alarm(list_alarm_root, current_alarm_array[iAlarm].id)) {
                printf("process_alarm: id=%s not find\n", current_alarm_array[iAlarm].id);
                if (map_add_alarm(list_alarm_root, &current_alarm_array[iAlarm]) != 0) {
                    upsdebugx(2, "Error during adding alarm %s", current_alarm_array[iAlarm].id);
                    res = -1;
                }
                alarm_state = ALARM_NEW;
                printf("process_alarm: id=%s added\n", current_alarm_array[iAlarm].id);
            }
            else {
                alarm_state = ALARM_ACTIVE;
            }
            /* treatment nut alarm */
            if (manage_alarm(&current_alarm_array[iAlarm], alarm_state) != 0) {
                upsdebugx(2, "Error during activating alarm %s", current_alarm_array[iAlarm].id);
            }
            alarm_count ++;
        }
    }
    /* then test if old alarms disappear */
    alarm_elm_t *current_alarm = list_alarm_root->list_alarm;
    while (current_alarm) {
        printf("process_alarm: test id=%s\n", current_alarm->alarm.id);
        int isFound = 0;
        for(iAlarm = 0; current_alarm_array && iAlarm < nb_current_alarm_array; iAlarm++) {
            if (strcmp(current_alarm->alarm.id, current_alarm_array[iAlarm].id) == 0) {
                isFound = 1;
                break;
            }
        }
        if (!isFound) {
            /* treatment deactivate nut alarm */
            printf("process_alarm: deactivated id=%s\n", current_alarm->alarm.id);
            alarm_state = ALARM_INACTIVE;
            manage_alarm(&current_alarm->alarm, alarm_state);
            map_remove_alarm(list_alarm_root, current_alarm->alarm.id);
        }
        current_alarm = current_alarm->next_alarm;
        iAlarm++;
    }
    if (current_alarm_array) {
        for(iAlarm = 0; iAlarm < nb_current_alarm_array; iAlarm++) {
            if (current_alarm_array[iAlarm].id) free(current_alarm_array[iAlarm].id);
            if (current_alarm_array[iAlarm].code) free(current_alarm_array[iAlarm].code);
            if (current_alarm_array[iAlarm].topic) free(current_alarm_array[iAlarm].topic);
        }
        free(current_alarm_array);
        current_alarm_array = NULL;
        nb_current_alarm_array = -1;
    }

    /* update ups.alarm if some alarms have been detected */
    if (alarm_count > 0) {
       alarm_commit();
    }
    /* update ups.status */
    status_commit();
    printf("process_alarm: FIN : %d\n", res);
    return res;
}

/**
 * @function process_nb_alarm (Callback) Special treatment for number of alarms received
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_nb_alarm(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;

    int res = 0;
    int nb_init_alarm = atoi(value);
    if (current_alarm_array) {
        int iAlarm;
        for(iAlarm = 0; iAlarm < nb_current_alarm_array; iAlarm++) {
            if (current_alarm_array[iAlarm].id) free(current_alarm_array[iAlarm].id);
            if (current_alarm_array[iAlarm].code) free(current_alarm_array[iAlarm].code);
            if (current_alarm_array[iAlarm].topic) free(current_alarm_array[iAlarm].topic);
        }
        free(current_alarm_array);
        current_alarm_array = NULL;
        nb_current_alarm_array = -1;
    }
    if (nb_init_alarm >= 0) {
        printf("process_nb_alarm: nb_init_alarm=%d\n", nb_init_alarm);
        current_alarm_array = (alarm_t *) malloc(sizeof(alarm_t) * nb_init_alarm);
        if (current_alarm_array) {
            nb_current_alarm_array = nb_init_alarm;
            memset(current_alarm_array, 0, sizeof(alarm_t) * nb_current_alarm_array);
        }
        else {
            res = -2;
        }
    }
    return res;
}

/**
 * @function process_data_alarm (Callback) Special treatment for data alarm received
 * @param name Object name
 * @param value Object value
 * @param params Parameters array built from regex expression
 * @param nb_params Number of element in the parameters array
 * @return {integer} 0 if success else < 0
 */
int process_data_alarm(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 2) return -2;

    int index_alarm = atoi(params[0]);
    char *data_name = params[1];
    printf("ALARMS %d: %s = %s\n", index_alarm, data_name, value);
    if (index_alarm < nb_current_alarm_array) {
        if (strcmp(data_name, "id") == 0) {
            current_alarm_array[index_alarm].id = strdup(value);
        }
        else if (strcmp(data_name, "level") == 0) {
            current_alarm_array[index_alarm].level = atoi(value);
        }
        else if (strcmp(data_name, "code") == 0) {
            current_alarm_array[index_alarm].code = strdup(value);
        }
        else if (strcmp(data_name, "device/@id") == 0) {
            current_alarm_array[index_alarm].topic = strdup(value);
        }
    }
    return 0;
}

/**
 * nm2 topics mapping
 */
static const nm2_pairs s_nm2_mapping[] =  {
    { "^mbdetnrs/1.0/managers/1/identification/firmwareVersion$", "ups.firmware.aux", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/identification/vendor$", "device.mfr", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/identification/model$", "device.model", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/identification/serialNumber$", "device.serial", NULL },
	{ "^mbdetnrs/1.0/managers/1/identification/location$", "device.location", NULL },
	{ "^mbdetnrs/1.0/managers/1/identification/contact$", "device.contact", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/identification/firmwareVersion$", "ups.firmware", NULL },

	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/current$", "output.current", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/frequency$", "output.frequency", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/voltage$", "output.voltage", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/percentLoad$", "ups.load", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/voltage$", "battery.voltage", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/remainingChargeCapacity$", "battery.charge", NULL },
	{ "^mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/remainingTime$", "battery.runtime", NULL },

    { "^mbdetnrs/1.0/sensors/devices/members@count$", NULL, &process_device },
    { "^mbdetnrs/1.0/sensors/devices/members/([0-9]+)/@id$", NULL, &process_index_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/manufacturer$", "ambient.%d.mfr", &process_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/model$", "ambient.%d.model", &process_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/serial$", "ambient.%d.serial", &process_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/version$", "ambient.%d.firmware", &process_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/name$", "ambient.%d.name", &process_data_device  },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/communication/state$", "ambient.%d.present", &process_present_device  },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/members@count$", NULL, &process_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/members/([0-9]+)/@id$", NULL, &process_index_sensor },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/identification/name$", "ambient.%d.%s.name", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/lowCritical$", "ambient.%d.%s.low.critical", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/lowWarning$", "ambient.%d.%s.low.warning", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/highCritical$", "ambient.%d.%s.high.critical", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/highWarning$", "ambient.%d.%s.high.warning", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/temperatures/([^/]+)/measure/current$", "ambient.%d.temperature", &process_temperature_value },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)/measure/current$", "ambient.%d.%s", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/configuration/enabled$", NULL, &process_enable_sensor },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(digitalInputs)/([^/]+)/identification/name$", "ambient.%d.%s.%d.name", &process_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/digitalInputs/([^/]+)/configuration/activeLow$", "ambient.%d.contacts.%d.config", &process_contact_config },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(digitalInputs)/([^/]+)/configuration/enabled$", NULL, &process_enable_contacts },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/digitalInputs/([^/]+)/measure/active$", "ambient.%d.contacts.%d.status", &process_contact_status },

    { "^mbdetnrs/1.0/alarmService/activeAlarms/members@count$", NULL, &process_nb_alarm },
    { "^mbdetnrs/1.0/alarmService/activeAlarms/members/([0-9]+)/(@id|id|level|code|device/@id)$", NULL, &process_data_alarm },

    { NULL, NULL, NULL }
};

/*-------------------------------------------------------------*/
/* MQTT routines                                                */
/*-------------------------------------------------------------*/

/**
 * @function s_mqtt_loop
 */
static void s_mqtt_loop(void)
{
	const time_t startTime = time(NULL);
	time_t curTime;
	int rc;

	do {
		upsdebugx(4, "entering mosquitto_loop()");
		rc = mosquitto_loop(mosq, 5000 /* timeout */, 1 /* max_packets */);
		upsdebugx(4, "exited mosquitto_loop(), rc=%d", rc);
		curTime = time(NULL);
	} while ((rc == MOSQ_ERR_SUCCESS) && (startTime + 5 > curTime));

	if(rc != MOSQ_ERR_SUCCESS) {
		upsdebugx(1, "Error: %s", mosquitto_strerror(rc));
		if (rc == MOSQ_ERR_CONN_LOST)
			mqtt_reconnect();
	}
}

/**
 * @function s_mqtt_nm2_process
 * @param path Topic path
 * @param obj Json object
 */
static void s_mqtt_nm2_process(const char *path, struct json_object *obj)
{
	const nm2_pairs *it = s_nm2_mapping;
    regex_t preg;
    regmatch_t match[MAX_SUB_EXPR];
    int r;

    upsdebugx(2, "s_mqtt_nm2_process: %s", path);
	while (it->in) {
        r = regcomp(&preg, it->in, REG_EXTENDED);
        if (r == 0) {
            r = regexec(&preg, path, sizeof(match)/sizeof(match[0]), (regmatch_t*)&match, 0);
            if (r == 0) {
                upsdebugx(2, "s_mqtt_nm2_process: %s found", path);
                bool no_error = true;
                char value[ST_MAX_VALUE_LEN] = "";
                switch (json_object_get_type(obj)) {
                    case json_type_boolean:
                        snprintf(value, sizeof(value), "%d", json_object_get_boolean(obj));
                        break;
                    case json_type_int:
                        snprintf(value, sizeof(value), "%ld", json_object_get_int64(obj));
                        break;
                    case json_type_double:
                        snprintf(value, sizeof(value), "%.2lf", json_object_get_double(obj));
                        break;
                    case json_type_string:
                        snprintf(value, sizeof(value), "%s", json_object_get_string(obj));
                        break;
                    default:
                        upsdebugx(2, "No match for property %s of type %d", path, json_object_get_type(obj));
                        no_error = false;
                        break;
                }
                if (no_error) {
                    /* special treatment */
                    if (it->process_ptr != NULL) {
                        //printf("preg.re_nsub=%d\n", preg.re_nsub);
                        char **params = NULL;
                        uint i;
                        if (preg.re_nsub > 0) {
                            params = malloc(sizeof(char *) * preg.re_nsub);
                            for (i = 0; i < preg.re_nsub; i++) {
                                params[i] = strndup(path + match[i + 1].rm_so, match[i + 1].rm_eo - match[i + 1].rm_so);
                                printf("%d: param=%s\n", i, params[i]);
                            }
                        }
                        int r = it->process_ptr(it->out, value, params, preg.re_nsub);
                        if (r < 0) {
                            printf("********************** Error during converting %s value %s: %d\n", it->out ? it->out : "", value, r);
                            upsdebugx(2, "Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                        }
                        for (i = 0; i < preg.re_nsub; i++) {
                            if (params[i]) free(params[i]);
                        }
                        if (params) free(params);
                    }
                    /* else normal treatment */
                    else {
                        upsdebugx(2, "Mapped property %s to %s value=%s", path, it->out, value);
                        dstate_setinfo(it->out, "%s", value);
                    }
                }
                regfree(&preg);
                return;
            }
            regfree(&preg);
		}
		it++;
	}
}

/**
 * @function s_mqtt_nm2_walk
 * @param path Topic path
 * @param obj Json object
 */
static void s_mqtt_nm2_walk(const char *path, struct json_object *obj)
{
	char newPath[256] = { '\0' };
	struct json_object_iterator it, itEnd;
	int index;
    upsdebugx(2, "s_mqtt_nm2_walk: %s", path);
	switch (json_object_get_type(obj)) {
		case json_type_object:
			itEnd = json_object_iter_end(obj);
			for (it = json_object_iter_begin(obj); !json_object_iter_equal(&it, &itEnd); json_object_iter_next(&it)) {
				snprintf(newPath, sizeof(newPath), "%s/%s", path, json_object_iter_peek_name(&it));
				s_mqtt_nm2_walk(newPath, json_object_iter_peek_value(&it));
			}
			break;
		case json_type_array:
			for (index = 0; index < json_object_array_length(obj); index++) {
				snprintf(newPath, sizeof(newPath), "%s/%d", path, index);
				s_mqtt_nm2_walk(newPath, json_object_array_get_idx(obj, index));
			}
			break;
		default:
			s_mqtt_nm2_process(path, obj);
			break;
	}
}

/**
 * @function s_mqtt_nm2_message_callback
 * @param path Topic path
 * @param message Message received
 * @param message_len Message received length
 */
static void s_mqtt_nm2_message_callback(const char *topic, void *message, size_t message_len)
{
    regex_t preg;
    regmatch_t match[MAX_SUB_EXPR];

    const char *topics_list[] = {
        "^mbdetnrs/1.0/managers/1/identification$",
        "^mbdetnrs/1.0/powerDistributions/1/identification$",
        // TBD
        //"^mbdetnrs/1.0/powerDistributions/1/(inputs|outputs)/?????/measures$",
        "^mbdetnrs/1.0/sensors/devices$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/(identification|communication$)$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/([^/]+)/(identification|configuration$|alarms|measure$)$",

        "^mbdetnrs/1.0/alarmService/activeAlarms$",

        "^mbdetnrs/1.0/powerService/suppliers$",
        "^mbdetnrs/1.0/powerService/suppliers/([^/]+)/(identification|configuration|summary|measures)$"
    };

   unsigned char is_found = 0;

    do {
        uint nb_topic;
        for (nb_topic = 0; nb_topic < sizeof(topics_list) / sizeof(char *); nb_topic++) {
            int r = regcomp(&preg, topics_list[nb_topic], REG_EXTENDED);
            if (r == 0) {
                r = regexec(&preg, topic, sizeof(match)/sizeof(match[0]), (regmatch_t*)&match, 0);
                //printf("Result: %s %d\n", topic, r);
                if (r == 0) {
                    // TBD: TO REMOVE ???
                    char *id_sensor = NULL;
                    char *id_sub_sensor = NULL;
                    char *type_sensor = NULL;
                    char *type_function = NULL;

                    if (preg.re_nsub >= 1) {
                        id_sensor = strndup(topic + match[1].rm_so,
                                    match[1].rm_eo - match[1].rm_so);
                    }
                    if (preg.re_nsub == 2) {
                        type_function = strndup(topic + match[2].rm_so,
                                        match[2].rm_eo - match[2].rm_so);
                    }
                    else if (preg.re_nsub == 4) {
                        type_sensor = strndup(topic + match[2].rm_so,
                                        match[2].rm_eo - match[2].rm_so);

                        id_sub_sensor = strndup(topic + match[3].rm_so,
                                        match[3].rm_eo - match[3].rm_so);

                        type_function = strndup(topic + match[4].rm_so,
                                    match[4].rm_eo - match[4].rm_so);
                    }
                    printf("id_sensor=%s id_sub_sensor=%s type_sensor=%s type_function=%s\n",
                           id_sensor ? id_sensor : "", id_sub_sensor ? id_sub_sensor : "",
                           type_sensor ? type_sensor : "", type_function ? type_function : "");
                    if (id_sensor) free(id_sensor);
                    if (id_sub_sensor) free(id_sub_sensor);
                    if (type_sensor) free(type_sensor);
                    if (type_function) free(type_function);
                    regfree(&preg);
                    is_found = 1;
                    break;
                }
                regfree(&preg);
            }
            else {
                printf("Result ERROR: %s %d\n", topic, r);
            }
        }
        break;
    } while (0);

    if (is_found) {
        struct json_tokener *tok = json_tokener_new();
        struct json_object *json = tok ? json_tokener_parse_ex(tok, (const char*)message, message_len) : NULL;

        if (json) {
            s_mqtt_nm2_walk(topic, json);
        }
        else {
            upsdebugx(1, "Error: could not parse JSON data on topic %s", topic);
        }
        process_alarms();

        if (json) json_object_put(json);
        if (tok) json_tokener_free(tok);
    }
}

/*-------------------------------------------------------------*/
/* NUT routines                                                */
/*-------------------------------------------------------------*/

/**
 * @function upsdrv_initinfo
 */
void upsdrv_initinfo(void)
{
	upsdebugx(1, "entering %s()", __func__);

	/* First loop to allow connection / subscription */
	s_mqtt_loop();

	if (!dstate_getinfo("device.mfr")) {
		fatalx(EXIT_FAILURE, "Failed to process essential data during startup of driver");
	}
}

/**
 * @function upsdrv_updateinfo
 */
void upsdrv_updateinfo(void)
{
	upsdebugx(1, "entering %s()", __func__);

	s_mqtt_loop();
}

/**
 * @function upsdrv_shutdown
 */
void upsdrv_shutdown(void)
{
	upsdebugx(1, "entering %s()", __func__);

	/* replace with a proper shutdown function */
	fatalx(EXIT_FAILURE, "shutdown not supported");
}

/**
 * @function upsdrv_help
 */
void upsdrv_help(void)
{
	upsdebugx(1, "entering %s()", __func__);
}

/**
 * @function upsdrv_makevartable
 */
/* list flags and values that you want to receive via -x */
void upsdrv_makevartable(void)
{
	upsdebugx(1, "entering %s()", __func__);

	addvar(VAR_FLAG, SU_VAR_CLEAN_SESSION,
		"When set, client session won't persist on the broker");

	addvar(VAR_VALUE, SU_VAR_CLIENT_ID,
		"Client ID of MQTT session");
	addvar(VAR_VALUE | VAR_SENSITIVE, SU_VAR_USERNAME,
		"Username for authentification");
	addvar(VAR_VALUE | VAR_SENSITIVE, SU_VAR_PASSWORD,
		"Password for authentification");

	addvar(VAR_FLAG, SU_VAR_TLS_INSECURE,
		"When set, do not check server certificate (TLS mode)");
	addvar(VAR_VALUE, SU_VAR_TLS_CA_FILE,
		"Path to server certificate authority file (TLS mode)");
	addvar(VAR_VALUE, SU_VAR_TLS_CA_PATH,
		"Path to server certificate authority directory (TLS mode)");
	addvar(VAR_VALUE, SU_VAR_TLS_CRT_FILE,
		"Path to client certificate (TLS mode)");
	addvar(VAR_VALUE, SU_VAR_TLS_KEY_FILE,
		"Path to client private key file (TLS mode)");
}

/**
 * @function upsdrv_initups
 */
void upsdrv_initups(void)
{
	upsdebugx(1, "entering %s()", __func__);

	bool var_insecure = false;
	bool var_clean_session = true;
	int var_port = 1883;
	int var_keepalive = 60;

    list_device_root = (device_map_t *) malloc(sizeof(device_map_t));
    memset(list_device_root, 0, sizeof(device_map_t));
    list_device_root->list_device = NULL; // TBD ???

    list_alarm_root = (alarm_map_t *) malloc(sizeof(alarm_map_t));
    memset(list_alarm_root, 0, sizeof(alarm_map_t));
    list_alarm_root->list_alarm = NULL; // TBD ???

	if (testvar(SU_VAR_TLS_INSECURE))
		var_insecure = true;
	if (testvar(SU_VAR_CLEAN_SESSION))
		var_clean_session = true;

	if (testvar(SU_VAR_TLS_INSECURE) ||
		testvar(SU_VAR_TLS_CA_FILE) ||
		testvar(SU_VAR_TLS_CA_PATH) ||
		testvar(SU_VAR_TLS_CRT_FILE) ||
		testvar(SU_VAR_TLS_KEY_FILE)) {
		var_port = 8883;
	}

	/* Initialize Mosquitto */
	mosquitto_lib_init();

	{
		/* Generate default client id */
		char default_client_id[128] = { '\0' };
		char hostname[HOST_NAME_MAX] = { '\0' };
		gethostname(hostname, sizeof(hostname));
		snprintf(default_client_id, sizeof(default_client_id), "nutdrv_mqtt/%s-%d", hostname, getpid());

		/* Create client instance */
		const char *client_id = testvar(SU_VAR_CLIENT_ID) ? getval(SU_VAR_CLIENT_ID) : default_client_id;
		mosq = mosquitto_new(client_id, var_clean_session, NULL);
	}

	if(!mosq) {
		mosquitto_lib_cleanup();
		fatalx(EXIT_FAILURE, "Error while creating client instance: %s", strerror(errno));
	}

    printf("client_id=%s username=%s password=%s tls_insecure=%s tls_ca_file=%s tls_ca_path=%s tls_crt_file=%s tls_key_file=%s\n",
            getval(SU_VAR_CLIENT_ID),
            getval(SU_VAR_USERNAME),
            getval(SU_VAR_PASSWORD),
            getval(SU_VAR_TLS_INSECURE),
            getval(SU_VAR_TLS_CA_FILE),
            getval(SU_VAR_TLS_CA_PATH),
            getval(SU_VAR_TLS_CRT_FILE),
            getval(SU_VAR_TLS_KEY_FILE));

	/* Set configuration points */
	mosquitto_username_pw_set(mosq, getval(SU_VAR_USERNAME), getval(SU_VAR_PASSWORD));
	mosquitto_tls_set(mosq,
		getval(SU_VAR_TLS_CA_FILE),
		getval(SU_VAR_TLS_CA_PATH),
		getval(SU_VAR_TLS_CRT_FILE),
		getval(SU_VAR_TLS_KEY_FILE),
		NULL);
	mosquitto_tls_insecure_set(mosq, var_insecure);
	mosquitto_tls_opts_set(mosq,
		var_insecure ? SSL_VERIFY_NONE : SSL_VERIFY_PEER,
		NULL,
		NULL);

	/* Setup callbacks for connection, subscription and messages */
	mosquitto_log_callback_set(mosq, mqtt_log_callback);
	mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
	mosquitto_disconnect_callback_set(mosq, mqtt_disconnect_callback);
	mosquitto_message_callback_set(mosq, mqtt_message_callback);
	mosquitto_subscribe_callback_set(mosq, mqtt_subscribe_callback);

	/* Connect to the broker */
	int rc = mosquitto_connect(mosq, device_path, var_port, var_keepalive);
	if(rc) {
		mosquitto_lib_cleanup();

		if(rc == MOSQ_ERR_ERRNO) {
			char err[1024] = { '\0' };
#ifdef WIN32
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, sizeof(err), NULL);
#else
            strncpy(err, strerror(errno), sizeof(err));
#endif
			fatalx(EXIT_FAILURE, "Error while connecting to MQTT broker: %s (%s)", mosquitto_strerror(rc), err);
		} else {
			fatalx(EXIT_FAILURE, "Error while connecting to MQTT broker: %s", mosquitto_strerror(rc));
		}
	}

	upsdebugx(1, "Connected to MQTT broker %s", device_path);
}

/**
 * @function upsdrv_cleanup
 */
void upsdrv_cleanup(void)
{
	upsdebugx(1, "entering %s()", __func__);

    /* clear sensor device list */
    map_remove_all_devices(list_device_root);
    if (list_device_root) free(list_device_root);

    /* clear alarm list */
    map_remove_all_alarms(list_alarm_root);
    if (list_alarm_root) free(list_alarm_root);

	/* Cleanup Mosquitto */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}

/*-------------------------------------------------------------*/
/* Mosquitto specific routines                                 */
/*-------------------------------------------------------------*/

/**
 * @function mqtt_connect_callback
 * @param mosq Mosquitto object
 * @param obj
 * @param result
 */
void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	if (result != 0) {
		upsdebugx(1, "Error inside connection callback, code=%d", result);
	}
	else {
		upsdebugx(2, "Connected to MQTT broker");

		int var_qos = 0;
		const char *topics[] = {
			"mbdetnrs/1.0/managers/1/identification",
			"mbdetnrs/1.0/powerDistributions/1/identification",
			"mbdetnrs/1.0/powerDistributions/1/inputs/+/measures",
			"mbdetnrs/1.0/powerDistributions/1/outputs/+/measures",
            "mbdetnrs/1.0/sensors",
            "mbdetnrs/1.0/sensors/devices",

            "mbdetnrs/1.0/sensors/devices/+",
            "mbdetnrs/1.0/sensors/devices/+/identification",
            "mbdetnrs/1.0/sensors/devices/+/communication",
            "mbdetnrs/1.0/sensors/devices/+/channels/temperatures",
            //"mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+",
            "mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+/identification",
            "mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+/configuration",
            //"mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+/alarms",
            //"mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+/alarms/thresholds",
            "mbdetnrs/1.0/sensors/devices/+/channels/temperatures/+/measure",
            "mbdetnrs/1.0/sensors/devices/+/channels/humidities",
            //"mbdetnrs/1.0/sensors/devices/+/channels/humidities/+",
            "mbdetnrs/1.0/sensors/devices/+/channels/humidities/+/identification",
            "mbdetnrs/1.0/sensors/devices/+/channels/humidities/+/configuration",
            "mbdetnrs/1.0/sensors/devices/+/channels/humidities/+/measure",
            //"mbdetnrs/1.0/sensors/devices/+/channels/humidities/+/alarms",
            "mbdetnrs/1.0/sensors/devices/+/channels/digitalInputs",
            "mbdetnrs/1.0/sensors/devices/+/channels/digitalInputs/+/identification",
            "mbdetnrs/1.0/sensors/devices/+/channels/digitalInputs/+/configuration",
            "mbdetnrs/1.0/sensors/devices/+/channels/digitalInputs/+/measure",

            "mbdetnrs/1.0/powerService/suppliers",
            "mbdetnrs/1.0/powerService/suppliers/+/identification",
            "mbdetnrs/1.0/powerService/suppliers/+/configuration",
            "mbdetnrs/1.0/powerService/suppliers/+/summary",
            "mbdetnrs/1.0/powerService/suppliers/+/measures",

            "mbdetnrs/1.0/alarmService/activeAlarms",

			NULL
		} ;
		const char **topic;

		for (topic = topics; *topic; topic++) {
			int rc = mosquitto_subscribe(mosq, NULL, *topic, var_qos);
			if (rc) {
				upsdebugx(1, "Error while subscribing to topic %s: %s", *topic, mosquitto_strerror(rc));
			}
			else {
				upsdebugx(2, "Sent subscribe request on topic %s", *topic);
			}
		}

	}
}

/**
 * @function mqtt_disconnect_callback
 * @param mosq Mosquitto object
 * @param obj
 * @param result
 */
void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	upsdebugx(1, "Disconnected from MQTT broker, code=%d", result);
	dstate_datastale();
}

/**
 * @function upsdrv_initinfo
 * @param mosq Mosquitto object
 * @param obj
 * @param message
 */
void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	upsdebugx(3, "Message received on topic[%s]: %.*s", message->topic, (int)message->payloadlen, (const char*)message->payload);
	s_mqtt_nm2_message_callback(message->topic, message->payload, message->payloadlen);
	dstate_dataok();
}

/**
 * @function mqtt_subscribe_callback
 * @param mosq Mosquitto object
 * @param obj
 * @param mid
 * @param qos_count
 * @param granted_qos
 */
void mqtt_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	upsdebugx(1, "Subscribed on topic, mid=%d", mid);
}

/**
 * @function mqtt_log_callback
 * @param mosq Mosquitto object
 * @param obj Object
 * @param level Log level
 * @param str String to log
 */
void mqtt_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	upsdebugx(1+level, "moqsuitto log: %s", str);
}

/**
 * @function mqtt_reconnect
 */
void mqtt_reconnect()
{
	while (mosquitto_reconnect(mosq) != MOSQ_ERR_SUCCESS) {
		upsdebugx(2, "Reconnection attempt failed.");
		sleep(10);
	}
	upsdebugx(1, "Reconnection attempt succeeded.");
}
