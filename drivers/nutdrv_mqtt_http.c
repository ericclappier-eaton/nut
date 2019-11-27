/*  nutdrv_mqtt_http.c - HTTP routines
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

#include "main.h"
#include "nutdrv_mqtt_http.h"

/*#define JSONC_VERSION_0_13 */
#undef JSONC_VERSION_0_13

/**
 * @function http_write_callback Write bytes from src to stream data
 * @param src The src object
 * @param size The number of object
 * @param nmemb The size of one object
 * @param stream The stream object
 * @return {number} Returns the data size wrote in the stream data
 */
static size_t http_write_callback(void *src, size_t size, size_t nmemb, void *stream)
{
    stream_t *s;
    char *new_data;
    size_t n_bytes;

    s = (stream_t*)stream; /* blind casted */
    if (!s) return 0;

    /* append s->data with (ptr, size*nmemb), add one byte for 0-term */
    n_bytes = size * nmemb;
    if (n_bytes == 0) return 0; /* nop */
    new_data = (char*) realloc(s->data, s->size + n_bytes + 1);
    if (!new_data) {
        upsdebugx(2, "http_write_callback: realloc failed");
        return 0;
    }
    /* append data */
    s->data = new_data;
    memcpy(s->data + s->size, src, n_bytes);
    s->size += n_bytes;
    s->data[s->size] = 0;

    return n_bytes;
}

/**
 * @function http_apply_options Apply http options on CURL and HEADER input handles
 * Note: HTTPS must be set to non 0 to apply specific https options.
 * @param curl The curl object
 * @param header The output header according input options
 * @param https Https option (0/1)
 * @param options The input options to apply
 * The header ptr must be released on return using curl_slist_free_all()
 */
static void http_apply_options(CURL *curl, struct curl_slist **header, int https, const http_options_t *options)
{
    if (!(curl && header && options)) return;

    /* set verbose mode */
    if (options->verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    char aux[128];
    /* credentials */
    if (options->login && options->password) {
        snprintf(aux, sizeof(aux), "%s:%s", options->login, options->password);
        curl_easy_setopt(curl, CURLOPT_USERPWD, aux);
        if (options->auth_basic) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC); /* basic (base64 encoding) */
            /* curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // auth (md5 digest) */
        }
    }

    /* set Auth Bearer header */
    if (options->auth_bearer) {
        snprintf(aux, sizeof(aux), "Authorization: Bearer %s", options->auth_bearer);
        *header = curl_slist_append(*header, aux);
    }

    /* set UserAgent header */
    if (options->user_agent) {
        snprintf(aux, sizeof(aux), "User-Agent: %s", options->user_agent);
        *header = curl_slist_append(*header, aux);
    }

    /* set no-proxy header (ip address) */
    if (options->no_proxy) {
        curl_easy_setopt(curl, CURLOPT_NOPROXY, options->no_proxy);
    }

    /* https specific */
    if (https) {
        /* turn off SSL/TLS cert. check (--insecure cURL option) */
        if (options->https_no_cert_check) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        /* force SSL 3.0 usage (--sslv3 cURL option) */
        if (options->https_force_ssl3) {
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
        }

        /* set 'Expect' header to empty */
        if (options->https_no_Expect) {
            *header = curl_slist_append(*header, "Expect:");
        }
    }
    /* http specific */
    else {
        /* set 'Expect' header to empty */
        if (options->http_no_Expect) {
            *header = curl_slist_append(*header, "Expect:");
        }
    }

    /* content type */
    if (options->content_type) {
        snprintf(aux, sizeof(aux), "Content-Type: %s", options->content_type);
        *header = curl_slist_append(*header, aux);
    }
}

/**
 * @function http_get Proceed a http GET request
 * @param url The request url
 * @param timeout_ms The request timeout (ms., 0 as infinite)
 * @param resp_status (optional) Set response status on return.
 * @param size (optional) Set size on return.
 * @param options Options for request
 * @return {string} Returns the loaded data as a char* (c-string)
 * Note: the returned pointer must be freed by caller
 */
char *http_get(const char *url, unsigned timeout_ms, int *resp_status, size_t *size, const http_options_t *options)
{
    char *result = (char *)NULL;
    CURL *curl = NULL;

    if (resp_status) *resp_status = 0;
    if (size) *size = 0;

    if (!(url && (*url))) return NULL;

    /* get a curl handle */
    curl = curl_easy_init();
    if (!curl) {
        upsdebugx(2, "http GET failed (%s): curl init", url);
    }
    else {
        struct curl_slist *header = NULL;
        CURLcode res;
        char https; /* boolean */

        stream_t stream;
        memset(&stream, 0, sizeof(stream));

        /* prepare request */
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms); /* 0 means infinite (eg. no timeout) */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);

        /* force to follow 30x and fail on 40x */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* handle external options */
        https = (strstr(url, "https://") == url);
        http_apply_options(curl, &header, https, options);

        /* apply headers */
        if (header) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        }

        /* perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            upsdebugx(1, "http GET failed (%s): %s [res=%d]", url, curl_easy_strerror(res), res);
        }
        else {
            /* get HTTP return code */
            if (resp_status) {
                long status;
                curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &status);
                *resp_status = (int) status;
                printf("http GET resp_status=%d\n", *resp_status);
            }
            else {
                printf("http GET failed resp_status null\n");
            }

            /* get result from stream */
            if (stream.data && (stream.size > 0))
            {
                if (size) *size = stream.size;
                result = stream.data;
                stream.data = NULL; /* owned by result/ caller */
            }
        }

        /* cleanup */
        free(stream.data);
        if (header) curl_slist_free_all(header);
    }

    /* cleanup */
    if (curl) curl_easy_cleanup(curl);

    return result;
}

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
char *http_post_data(const char *url, const char *data, unsigned timeout_ms, int *resp_status, size_t *size, const http_options_t *options)
{
    char *result = (char*)NULL;
    CURL *curl = NULL;

    if (resp_status) *resp_status = 0;
    if (size) *size = 0;
    if (!(url && (*url))) return NULL;
    if (!(data && (*data))) return NULL;

    /* get a curl handle */
    curl = curl_easy_init();
    if (!curl) {
        upsdebugx(1, "http POST DATA failed (%s): curl init", url);
    }
    else {
        struct curl_slist *header = NULL;
        CURLcode res;
        char https;

        /* stream to receive req. result */
        stream_t write_stream;
        memset(&write_stream, 0, sizeof(stream_t));

        /* stream to put data in req. body */
        stream_t read_stream;
        memset(&read_stream, 0, sizeof(stream_t));
        read_stream.data = (char*) data;
        read_stream.size = data ? strlen(data) : 0;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)read_stream.size);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_stream);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, read_stream.data);

        /* Force to follow 30x and fail on 40x */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* handle external options */
        https = (strstr(url, "https://") == url);
        http_apply_options(curl, &header, https, options);

        /* apply headers */
        if (header) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        }

        // perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            upsdebugx(3, "http POST DATA failed (%s): %s [res=%d]", url, curl_easy_strerror(res), res);
        }
        else {
            /* get HTTP return code */
            if (resp_status) {
                long status;
                curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &status);
                *resp_status = (int) status;
                printf("http POST resp_status=%d\n", *resp_status);
            }
            else {
                printf("http POST failed resp_status null\n");
            }

            /* Get result from rx stream */
            if (write_stream.data && (write_stream.size > 0))
            {
                if (size) *size = write_stream.size;
                result = write_stream.data;
                write_stream.data = NULL; // owned by result/ caller
            }
        }

        /* cleanup */
        if (write_stream.data) free(write_stream.data);
        if (header) curl_slist_free_all(header);
    }

    /* cleanup */
    if (curl) curl_easy_cleanup(curl);

    return result;
}

#ifndef JSONC_VERSION_0_13

/**
 * @function json_found_object Get recursive object value at specified path
 * @param obj The json object to parse
 * @param path The json path of the string to find
 * @param value The json object value found
 * @return {integer} Return 0 if object found else < 0
 */
static int json_found_object(struct json_object *obj, char *path, struct json_object **value)
{
    if (!(obj && path && value)) {
		return -1;
	}

	/* test validity of path */
	if (path[0] != '/') {
		return -2;
	}
    path ++;
	char *ch = strchr(path, '/');
	if (ch) {
        *ch = '\0';
    }

    int res = -1;
    struct json_object *obj_val = NULL;

	/* test if it is an array */
    if (json_object_is_type(obj, json_type_array)) {
		/* test validity of array index */
		int id = strtol(path, NULL, 10);
		if (id < 0) {
            return -3;
        }
        int length = json_object_array_length(obj);
        if (id >= length) {
            return -4;
        }
        obj_val = json_object_array_get_idx(obj, id);
        if (obj_val) {
            obj= obj_val;
            res = 0;
		}
	}

    /* test if it is simple object */
    if (json_object_object_get_ex(obj, path, &obj_val)) {
        obj = obj_val;
        res = 0;
	}

    /* next path */
	if (ch) {
		*ch = '/';
		return json_found_object(obj, ch, value);
	}
    if (res == 0) {
        *value = obj;
	}
    return res;
}
#endif

/**
 * @function http_json_get_value Get a string value at specified json path
 * @param json_obj The json object to parse
 * @param path The json path of the string to find
 * @return {string} Return the string value if find else NULL
 * Note: the returned pointer must be freed by caller
 */
char *http_json_get_value(struct json_object *json_obj, const char *path)
{
    if (!(json_obj && path)) {
        return NULL;
    }

    struct json_object *json_res = NULL;
#ifdef JSONC_VERSION_0_13
    int res = json_pointer_get(json_obj, path, &json_res);
#else
    int res = 0;
    if (path[0] == '\0') {
		json_res = json_obj;
	}
    else {
        char *path_cpy = strdup(path);
        if (!path_cpy) {
            return NULL;
        }
        res = json_found_object(json_obj, path_cpy, &json_res);
        free(path_cpy);
    }
#endif
    if (res == 0) {
        if (json_object_is_type(json_res, json_type_string)) {
            return strdup(json_object_get_string(json_res));
        }
    }
    return NULL;
}

/**
 * @function http_json_parse Parse a json string
 * @param json_str The json string to parse
 * @param json_obj The json object result of the parsing
 * @return {integer} Return 0 if no error else < 0
 * Note: Need to call json_object_put to release memory when finish using json_obj
 */
int http_json_parse(const char *json_str, struct json_object **json_obj)
{
    if (!(json_str && json_obj)) return -1;
    struct json_tokener *tok = json_tokener_new();
    *json_obj = tok ? json_tokener_parse_ex(tok, json_str, strlen(json_str)) : NULL;
    if (tok) json_tokener_free(tok);
    if (*json_obj) {
        return 0;
    }
    return -1;
}





