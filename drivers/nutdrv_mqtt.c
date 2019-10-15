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
#include "map_sensor.h"
#include <openssl/ssl.h>
#include <mosquitto.h>
#include <json-c/json.h>
#include <regex.h>

#define DRIVER_NAME	"MQTT driver"
#define DRIVER_VERSION	"0.04"
#define MAX_SUB_EXPR 5

/* driver description structure */
upsdrv_info_t upsdrv_info = {
	DRIVER_NAME,
	DRIVER_VERSION,
	"Arnaud Quette <ArnaudQuette@Eaton.com>",
	DRV_EXPERIMENTAL,
	{ NULL }
};

/* Variables */
struct mosquitto *mosq = NULL;
device_map_t *list_device_root = NULL;

/* Callbacks */
void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result);
void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result);
void mqtt_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str);
void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void mqtt_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void mqtt_reconnect();

/* Static functions */

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

typedef struct
{
	const char *in;
	const char *out;
    int (*treatment_ptr)(const char *name, const char *value, char **params, int nb_params);
} nm2_pairs;

int treatment_contact_config(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    const char *contact_config[] = {
        "normal-opened",
        "normal-closed"
    };

    int index_sensor = map_find_device(list_device_root, params[0]);
    int index_sub_sensor = map_find_sensor(list_device_root, index_sensor, TYPE_INPUTS, params[1]);
    if (index_sensor >= 0 && index_sub_sensor >= 0) {
        char *new_name = NULL;
        if (asprintf(&new_name, name, index_sensor + 1, index_sub_sensor + 1) != -1) {
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

int treatment_contact_status(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    const char *contact_status[] = {
        "active",
        "inactive"
    };

    int index_device = map_find_device(list_device_root, params[0]);
    int index_sensor = map_find_sensor(list_device_root, index_device, TYPE_INPUTS, params[1]);
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

int treatment_temperature_value(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 2) return -2;

    int index_device = map_find_device(list_device_root, params[0]);
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

int treatment_data_device(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 1) return -2;

    int index_device = map_find_device(list_device_root, params[0]);
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

int treatment_data_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!(name && value)) return -1;
    if (!params || nb_params != 3) return -2;

    const char *type_sensor_name[] = {
        "temperature",
        "humidity",
        "contacts"
    };

    char *new_name = NULL;
    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor >= 0 && type_sensor < 3) {
        int index_device = map_find_device(list_device_root, params[0]);
        int index_sensor = map_find_sensor(list_device_root, index_device, type_sensor, params[2]);
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

int treatment_device(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;

    if (map_add_devices(list_device_root, atoi(value)) == 0) {
        return 0;
    }
    return -2;
}

int treatment_index_device(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 1) return -2;

    char *pch = strrchr(value, '/');
    if (pch && (uint)(pch - value + 1) < strlen(value)) {
        if (map_init_device(list_device_root, atoi(params[0]), pch + 1) == 0) {
            return 0;
        }
    }
    return -3;
}

int treatment_sensor(const char *name, const char *value, char **params, int nb_params) {
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

int treatment_index_sensor(const char *name, const char *value, char **params, int nb_params) {
    if (!value) return -1;
    if (!params || nb_params != 3) return -2;

    int type_sensor = map_get_index_type_sensor(params[1]);
    if (type_sensor >= 0) {
        char *pch = strrchr(value, '/');
        if (pch && (uint)(pch - value + 1) < strlen(value)) {
            char *sensor_key = pch + 1;
            char *device_key = params[0];
            if (map_init_sensor(list_device_root, device_key, type_sensor, atoi(params[2]), sensor_key) == 0) {
                return 0;
            }
        }
    }
    return -3;
}

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

    { "^mbdetnrs/1.0/sensors/devices/members@count$", NULL, &treatment_device },
    { "^mbdetnrs/1.0/sensors/devices/members/([0-9]+)/@id$", NULL, &treatment_index_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/manufacturer$", "ambient.%d.mfr", &treatment_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/model$", "ambient.%d.model", &treatment_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/serial$", "ambient.%d.serial", &treatment_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/version$", "ambient.%d.firmware", &treatment_data_device },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/identification/name$", "ambient.%d.name", &treatment_data_device  },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/communication/state$", "ambient.%d.present", &treatment_data_device  },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/members@count$", NULL, &treatment_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/members/([0-9]+)/@id$", NULL, &treatment_index_sensor },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/identification/name$", "ambient.%d.%s.name", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/lowCritical$", "ambient.%d.%s.low.critical", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/lowWarning$", "ambient.%d.%s.low.warning", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/highCritical$", "ambient.%d.%s.high.critical", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/alarms/thresholds/highWarning$", "ambient.%d.%s.high.warning", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/temperatures/([^/]+)/measure/current$", "ambient.%d.temperature", &treatment_temperature_value },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(humidities)/([^/]+)/measure/current$", "ambient.%d.%s", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities)/([^/]+)/configuration/enabled$", "ambient.%d.%s.enable", &treatment_data_sensor },

    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(digitalInputs)/([^/]+)/identification/name$", "ambient.%d.%s.%d.name", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/digitalInputs/([^/]+)/configuration/activeLow$", "ambient.%d.contacts.%d.config", &treatment_contact_config },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(digitalInputs)/([^/]+)/configuration/enabled$", "ambient.%d.%s.%d.enable", &treatment_data_sensor },
    { "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/digitalInputs/([^/]+)/measure/active$", "ambient.%d.contacts.%d.status", &treatment_contact_status },

    { NULL, NULL, NULL }
};

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
                    // special treatment
                    if (it->treatment_ptr != NULL) {
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
                        int r = it->treatment_ptr(it->out, value, params, preg.re_nsub);
                        if (r < 0) {
                            printf("********************** Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                            upsdebugx(2, "Error during converting %s value %s: %d", it->out ? it->out : "", value, r);
                        }
                        for (i = 0; i < preg.re_nsub; i++) {
                            if (params[i]) free(params[i]);
                        }
                        if (params) free(params);
                    }
                    // else normal treatment
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

static void s_mqtt_nm2_message_callback(const char *topic, void *message, size_t message_len)
{
    regex_t preg;
    regmatch_t match[MAX_SUB_EXPR];

    const char *topics_list[] = {
        "^mbdetnrs/1.0/managers/1/identification$",
        "^mbdetnrs/1.0/powerDistributions/1/identification$",
        //"^mbdetnrs/1.0/powerDistributions/1/(inputs|outputs)/?????/measures$",
        "^mbdetnrs/1.0/sensors/devices$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/(identification|communication$)$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)$",
        "^mbdetnrs/1.0/sensors/devices/([^/]+)/channels/(temperatures|humidities|digitalInputs)/([^/]+)/(identification|configuration$|measure$)$"
    };

    bool is_found = false;

    do {
        /*int r = regcomp(&preg, "^mbdetnrs/1.0/sensors/devices$", REG_EXTENDED);
        if (r == 0) {
            r = regexec(&preg, topic, sizeof(match)/sizeof(match[0]), (regmatch_t*)&match, 0);
            if (r == 0) {
                // Remove all elements
                map_remove_devices(list_sensor_root);
                is_found = true;
                break;
            }
        }*/

        uint nb_topic;
        for (nb_topic = 0; nb_topic < sizeof(topics_list) / sizeof(char *); nb_topic++) {
            int r = regcomp(&preg, topics_list[nb_topic], REG_EXTENDED);
            if (r == 0) {
                r = regexec(&preg, topic, sizeof(match)/sizeof(match[0]), (regmatch_t*)&match, 0);
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
                    is_found = true;
                    break;
                }
                regfree(&preg);
                printf("Result: %s %d\n", topic, r);
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
        if (json) json_object_put(json);
        if (tok) json_tokener_free(tok);
    }
}

/* NUT routines */

void upsdrv_initinfo(void)
{
	upsdebugx(1, "entering %s()", __func__);

	/* First loop to allow connection / subscription */
	s_mqtt_loop();

	if (!dstate_getinfo("device.mfr")) {
		fatalx(EXIT_FAILURE, "Failed to process essential data during startup of driver");
	}
}

void upsdrv_updateinfo(void)
{
	upsdebugx(1, "entering %s()", __func__);

	s_mqtt_loop();
}

void upsdrv_shutdown(void)
{
	upsdebugx(1, "entering %s()", __func__);

	/* replace with a proper shutdown function */
	fatalx(EXIT_FAILURE, "shutdown not supported");
}

void upsdrv_help(void)
{
	upsdebugx(1, "entering %s()", __func__);
}

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

void upsdrv_initups(void)
{
	upsdebugx(1, "entering %s()", __func__);

#if 0
    map_test();
    exit(EXIT_SUCCESS);
#endif


	bool var_insecure = false;
	bool var_clean_session = true;
	int var_port = 1883;
	int var_keepalive = 60;

    list_device_root = (device_map_t *) malloc(sizeof(device_map_t));
    memset(list_device_root, 0, sizeof(sizeof(device_map_t)));
    list_device_root->list_device = NULL; // TBD ???

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

void upsdrv_cleanup(void)
{
	upsdebugx(1, "entering %s()", __func__);
    map_remove_devices(list_device_root);
    if (list_device_root) free(list_device_root);

	/* Cleanup Mosquitto */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}

/*
 * Mosquitto specific routines
 ******************************/

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

void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	upsdebugx(1, "Disconnected from MQTT broker, code=%d", result);
	dstate_datastale();
}

void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	upsdebugx(3, "Message received on topic[%s]: %.*s", message->topic, (int)message->payloadlen, (const char*)message->payload);
	s_mqtt_nm2_message_callback(message->topic, message->payload, message->payloadlen);
	dstate_dataok();
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	upsdebugx(1, "Subscribed on topic, mid=%d", mid);
}

void mqtt_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	upsdebugx(1+level, "moqsuitto log: %s", str);
}

void mqtt_reconnect()
{
	while (mosquitto_reconnect(mosq) != MOSQ_ERR_SUCCESS) {
		upsdebugx(2, "Reconnection attempt failed.");
		sleep(10);
	}

	upsdebugx(1, "Reconnection attempt succeeded.");
}
