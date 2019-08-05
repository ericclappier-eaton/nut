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
#include <openssl/ssl.h>
#include <mosquitto.h>
#include <json-c/json.h>

#define DRIVER_NAME	"MQTT driver"
#define DRIVER_VERSION	"0.04"

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
} nm2_pairs;

static const nm2_pairs s_nm2_mapping[] = {
	{ "mbdetnrs/1.0/managers/1/identification/firmwareVersion", "ups.firmware.aux" },

	{ "mbdetnrs/1.0/powerDistributions/1/identification/vendor", "device.mfr" },
	{ "mbdetnrs/1.0/powerDistributions/1/identification/model", "device.model" },
	{ "mbdetnrs/1.0/powerDistributions/1/identification/serialNumber", "device.serial" },
	{ "mbdetnrs/1.0/managers/1/identification/location", "device.location" },
	{ "mbdetnrs/1.0/managers/1/identification/contact", "device.contact" },
	{ "mbdetnrs/1.0/powerDistributions/1/identification/firmwareVersion", "ups.firmware" },

	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/current", "output.current" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/frequency", "output.frequency" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/voltage", "output.voltage" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/measures/realtime/percentLoad", "ups.load" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/voltage", "battery.voltage" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/remainingChargeCapacity", "battery.charge" },
	{ "mbdetnrs/1.0/powerDistributions/1/inputs/1/batteries/measures/remainingTime", "battery.runtime" },

	{ NULL, NULL }
};

static void s_mqtt_nm2_process(const char *path, struct json_object *obj)
{
	const nm2_pairs *it = s_nm2_mapping;

	while (it->in) {
		if (strcmp(path, it->in) == 0) {
			switch (json_object_get_type(obj)) {
				case json_type_int:
				case json_type_double:
					dstate_setinfo(it->out, "%lf", json_object_get_double(obj));
					upsdebugx(2, "Mapped property %s to %s value %lf", path, it->out, json_object_get_double(obj));
					break;
				case json_type_string:
					dstate_setinfo(it->out, "%s", json_object_get_string(obj));
					upsdebugx(2, "Mapped property %s to %s value %s", path, it->out, json_object_get_string(obj));
					break;
				default:
					upsdebugx(2, "No match for property %s", path);
					break;
			}
			return;
		}
		it++;
	}
}

static void s_mqtt_nm2_walk(const char *path, struct json_object *obj)
{
	char newPath[256] = { '\0' };
	struct json_object_iterator it, itEnd;
	int index;

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

	bool var_insecure = false;
	bool var_clean_session = true;
	int var_port = 1883;
	int var_keepalive = 60;

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
			(void)strerror_r(errno, err, sizeof(err));
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
