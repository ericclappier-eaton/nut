/*  nutdrv_mqtt.h - Driver for MQTT
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
 */

#define DRIVER_NAME	"MQTT driver"
#define DRIVER_VERSION	"0.04"
#define MAX_SUB_EXPR 5

#define SU_VAR_CLEAN_SESSION            "clean_session"
#define SU_VAR_CLIENT_ID                "client_id"
#define SU_VAR_USERNAME                 "username"
#define SU_VAR_PASSWORD                 "password"
#define SU_VAR_TLS_INSECURE             "tls_insecure"
#define SU_VAR_TLS_CA_FILE              "tls_ca_file"
#define SU_VAR_TLS_CA_PATH              "tls_ca_path"
#define SU_VAR_TLS_CRT_FILE             "tls_crt_file"
#define SU_VAR_TLS_KEY_FILE             "tls_key_file"

#if 0
#define SU_VAR_APP_UUID                 "application_uuid"
#define SU_VAR_APP_SHORT_NAME           "application_short_name"
#define SU_VAR_APP_LONG_NAME            "application_long_name"
#define SU_VAR_APP_VENDOR               "application_vendor"
#define SU_VAR_APP_VERSION              "application_version"
#define SU_VAR_APP_OS                   "application_os"
#define SU_VAR_APP_LOCATION             "application_location"
#endif

enum ALARM_STATE { ALARM_NEW, ALARM_ACTIVE, ALARM_INACTIVE };