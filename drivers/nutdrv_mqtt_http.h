/*  nutdrv_mqtt_http.h - Http routines
 *
 *  Copyright (C)
 *    2019  Eric Clappier <EricClappier@Eaton.com>
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

#ifndef MQTT_HTTP_H
#define MQTT_HTTP_H

#include <curl/curl.h>
#include <json-c/json.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* stream typedef */
typedef struct {
    char *data;  /* mem. block pointer */
    size_t size; /* #bytes pointed */
} stream_t;

/* structure to choose specific options for GET/POST requests */
typedef struct _http_options_t {
    const char *login; /* login credentials */
    const char *password; /* password credentials */
    const char *auth_bearer; /* Authorization bearer header (access token) */
    const char *user_agent; /* UserAgent header */
    const char *no_proxy; /* No-proxy header (ip address) */
    const char *content_type; /* Content type */
    char verbose; /* boolean, activate verbosity */
    char auth_basic; /* boolean, basic auth */
    char http_no_Expect; /* boolean, set 'Expect' header to empty (http req. only) */
    char https_no_cert_check; /* boolean, disable certs checking (https req. only) */
    char https_force_ssl3; /* boolean, force SSL3 usage, https only (https req. only) */
    char https_no_Expect; /* boolean, set 'Expect' header to empty (https req. only) */
} http_options_t;

/**
 * @function http_get Proceed a http GET request
 * @param timeout_ms The request timeout (ms., 0 as infinite)
 * @param resp_status (optional) Set response status on return.
 * @param size (optional) Set size on return.
 * @param options Options for request
 * @return {string} Returns the loaded data as a char* (c-string)
 * Note: the returned pointer must be freed by caller
 */
char *http_get(const char *url, unsigned timeout_ms, int *resp_status, size_t *size, const http_options_t *options);

/**
 * @function http_post_data Proceed a http POST data to the given URL.
 * @param data The data to send
 * @param timeout_ms The request timeout (ms., 0 as infinite)
 * @param resp_status (optional) Set response status on return.
 * @param size (optional) Set size on return.
 * @param options Options for request
 * @return {string} Returns the loaded data as a char* (c-string)
 * Note: the returned pointer must be freed by caller
 */
char *http_post_data(const char *url, const char *data, unsigned timeout_ms, int *resp_status, size_t *size, const http_options_t *options);

/**
 * @function http_json_get_value Get a string value at specified json path
 * @param json_obj The json object to parse
 * @param path The json path of the string to find
 * @return {string} Return the string value if find else NULL
 * Note: the returned pointer must be freed by caller
 */
char *http_json_get_value(struct json_object *json_obj, const char *path);

/**
 * @function http_json_parse Parse a json string
 * @param json_str The json string to parse
 * @param json_obj The json object result of parsing
 * @return {integer} Return 0 if no error else < 0
 * Note: Need to call json_object_put to release memory when finish using json_obj
 */
int http_json_parse(const char *json_str, struct json_object **json_obj) ;

#ifdef __cplusplus
}
#endif

#endif /* MQTT_HTTP_H */

