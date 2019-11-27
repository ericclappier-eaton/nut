/* nutdrv_http_test - CppUnit http routines test

   Copyright (C) 2019  Clappier Eric <EricClappier@Eaton.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#define CPPUNIT_ASSERT_STREAM(MSG, CONDITION)\
    do {\
        std::ostringstream oss;\
        CPPUNIT_ASSERT_MESSAGE(\
            static_cast<std::ostringstream &>(oss << MSG).str(),\
            CONDITION);\
    } while (0);

class Http_test : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Http_test);
		CPPUNIT_TEST(http_safety_test);
		CPPUNIT_TEST(http_functional_test);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void http_safety_test();
	void http_functional_test();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(Http_test);

#include "../drivers/nutdrv_mqtt_http.h"
//#include <secw_producer_accessor.h>

void Http_test::setUp()
{
    /* cURL init */
    curl_global_init(CURL_GLOBAL_ALL);
}

void Http_test::tearDown()
{
    /* cURL cleanup */
    curl_global_cleanup();
}

void Http_test::http_safety_test()
{
    CPPUNIT_ASSERT(http_json_parse(NULL, NULL) < 0);
    CPPUNIT_ASSERT(http_json_parse("", NULL) < 0);
    CPPUNIT_ASSERT(http_json_get_value(NULL, NULL) == NULL);
    CPPUNIT_ASSERT(http_json_get_value(NULL, "") == NULL);
    CPPUNIT_ASSERT(http_get(NULL, 0, NULL, NULL, NULL) == NULL);
    CPPUNIT_ASSERT(http_post_data(NULL, NULL, 0, NULL, NULL, NULL) == NULL);
}

void Http_test::http_functional_test()
{
    /* json tests */
    {
        static const char *JSON_STRING =
        "{\"user\": \"johndoe\", \"admin\": \"false\", \"uid\": 1000,\n"
        "\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"],"
        "\"lists\": {\"user1\": \"name1\", \"user2\": \"name2\"}}";

        struct json_object *json_obj = NULL;
        CPPUNIT_ASSERT(http_json_parse(JSON_STRING, &json_obj) == 0);
        char *value = http_json_get_value(json_obj, "/user");
        CPPUNIT_ASSERT(value && strcmp(value, "johndoe") == 0);
        value = http_json_get_value(json_obj, "/admin");
        CPPUNIT_ASSERT(value && strcmp(value, "false") == 0);
        value = http_json_get_value(json_obj, "/uid");  /* not a string */
        CPPUNIT_ASSERT(value == NULL);
        value = http_json_get_value(json_obj, "/notgood");  /* not existing */
        CPPUNIT_ASSERT(value == NULL);
        value = http_json_get_value(json_obj, "/groups/0");
        CPPUNIT_ASSERT(value && strcmp(value, "users") == 0);
        value = http_json_get_value(json_obj, "/lists/user1");
        CPPUNIT_ASSERT(value && strcmp(value, "name1") == 0);
        json_object_put(json_obj);
    }

    /* http tests with NMC card */
    {
        const char *address = "10.130.35.76";
        const char *login = "admin";
        const char *password = "admin";

        int RETRY_MAX = 3;
        int resp_code;
        int status = 0;
        unsigned timeout_ms = 5000; /* 5 sec */
        char *content = NULL;
        char *url = NULL;
        struct json_object *json_obj = NULL;
        http_options_t http_opt;
        memset(&http_opt, 0, sizeof(http_options_t));

        http_opt.user_agent = "Test";
        /* TBD http_opt.no_proxy = address; */
        CPPUNIT_ASSERT(asprintf(&url, "http://%s/ups_cont.htm", address) != -1);
        int retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (content) free(content);
        } while (retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        CPPUNIT_ASSERT_MESSAGE("Get http with secure access and without password", status == 401);

        memset(&http_opt, 0, sizeof(http_options_t));
        http_opt.user_agent = "Test";
        http_opt.login = login;
        http_opt.password = password;
        retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (content) free(content);
        } while (retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        CPPUNIT_ASSERT_MESSAGE("Get http with secure access and with password", status == 200);
        if (url) free(url);
    }

    /* http tests with M2 card */
    {
        const char *address = "10.130.33.35";
        const char *login = "admin";
        const char *password = "noSoup4u!";

        int RETRY_MAX = 3;
        int resp_code;
        int status = 0;
        unsigned timeout_ms = 5000; /* 5 sec */
        char *content = NULL;
        char *url = NULL;
        struct json_object *json_obj = NULL;
        http_options_t http_opt;
        memset(&http_opt, 0, sizeof(http_options_t));

        /* get token with bad credentials */
        http_opt.user_agent = "Test";
        http_opt.https_no_cert_check = 1;
        http_opt.https_no_Expect = 1;
        CPPUNIT_ASSERT(asprintf(&url, "https://%s/rest/mbdetnrs/1.0/oauth2/token", address) != -1);
        int retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (content) free(content);
        } while(retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        CPPUNIT_ASSERT_MESSAGE("Get https with secure access and without password", status == 401);

        /* get token */
        memset(&http_opt, 0, sizeof(http_options_t));
        http_opt.user_agent = "Test";
        http_opt.https_no_cert_check = 1;
        http_opt.https_no_Expect = 1;
        // oauth data/ credentials (json)
        char *data = NULL;
        CPPUNIT_ASSERT(asprintf(&data,
                "{\"grant_type\":\"password\",\"scope\":\"webservices\",\"username\":\"%s\",\"password\":\"%s\"}",
                 login, password) != -1);
        retry = RETRY_MAX;
        do {
            content = http_post_data(url, data, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (status == 0 && content) free(content);
        } while(retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        if (url) free(url);
        CPPUNIT_ASSERT_MESSAGE("Get https with secure access and with password", status == 200);
        CPPUNIT_ASSERT(content);
        CPPUNIT_ASSERT(http_json_parse(content, &json_obj) == 0);
        if (content) free(content);
        char *bearer_token = http_json_get_value(json_obj, "/access_token");
        json_object_put(json_obj);
        CPPUNIT_ASSERT(bearer_token);
        http_opt.auth_bearer = bearer_token;
        CPPUNIT_ASSERT(asprintf(&url, "https://%s/rest/mbdetnrs/1.0/", address) != -1);
        retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (content) free(content);
        } while(retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        if (url) free(url);
        if (bearer_token) free(bearer_token);
        CPPUNIT_ASSERT_MESSAGE("Get https with bearer access", status == 200);
        if (data) free(data);

        /* get and parse rest comm */
        char *clientCertUrl = NULL;
        memset(&http_opt, 0, sizeof(http_options_t));
        http_opt.user_agent = "Test";
        http_opt.https_no_cert_check = 1;
        http_opt.https_no_Expect = 1;
        CPPUNIT_ASSERT(asprintf(&url, "https://%s/etn/v1/comm", address) != -1);
        retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (status == 0 && content) free(content);
        } while (retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        if (url) free(url);
        CPPUNIT_ASSERT_MESSAGE("Get https without secure access", content && (status == 200));

        int res = http_json_parse(content, &json_obj);
        if (content) free(content);
        CPPUNIT_ASSERT_MESSAGE("Get https without secure access", res == 0);
        char *value = http_json_get_value(json_obj, "/identification/uuid");
        CPPUNIT_ASSERT_MESSAGE("Read uuid", value != NULL);
        printf("uuid=%s\n", value);
        free(value);
        value = http_json_get_value(json_obj, "/certificates/members/0/clients/path");
        CPPUNIT_ASSERT_MESSAGE("Read certificat path", value != NULL);
        printf("client certificat path=%s\n", value);
        clientCertUrl = strdup(value);
        CPPUNIT_ASSERT_MESSAGE("client certificat path", clientCertUrl);
        free(value);

        /* get server certificat */
        char serverCertFileName[11] = "";
        char *serverCertificate = http_json_get_value(json_obj, "/certificates/members/0/server");
        printf("serverCertificate=%s\n", serverCertificate);
        if (json_obj) json_object_put(json_obj);
        {
            X509 *cert = NULL;
            BIO *bio = BIO_new_mem_buf((void*)serverCertificate, strlen(serverCertificate));
            cert = PEM_read_bio_X509(bio, &cert, NULL, NULL);
            if(cert)
            {
                char buf[512];
                X509_NAME_oneline(X509_get_subject_name(cert), buf, 512);
                printf("subject=%s\n", buf);

                X509_NAME_oneline(X509_get_issuer_name(cert), buf, 512);
                printf("issuer=%s\n", buf);

                unsigned long hash = X509_subject_name_hash(cert);
                printf("hash=%lu\n", buf);

                char hexstring[32];
                snprintf(hexstring, sizeof(hexstring), "%x", hash);
                printf("decimal: %d  hex: %s\n", hash, hexstring);
                hexstring[8] = '\0';
                snprintf(serverCertFileName, sizeof(serverCertFileName), "%s.0", hexstring);
                printf("serverCertFileName=%s", serverCertFileName);
                X509_free(cert);
            }
            BIO_free(bio);

            /* TBD: add certifcate into security wallet -> caution c++ only */
            /*fty::SocketSyncClient syncClient("mqtt-test.socket");
            mlm::MlmStreamClient streamClient("mqtt-server-test", SECW_NOTIFICATIONS, 1000, endpoint);
            ProducerAccessor producerAccessor(syncClient, streamClient);
            try
            {
              ExternalCertificatePtr doc = std::make_shared<ExternalCertificate>("Test insert external certificate", serverCertificate);
              doc->addUsage("discovery_monitoring");
              id = producerAccessor.insertNewDocument("default", std::dynamic_pointer_cast<Document>(doc));
            }
            catch(const std::exception& e)
            {
              printf("Error: %s\n", e.what());
            }*/
        }
        if (serverCertificate) free(serverCertificate);

        /* push client certificat */
        const char *certificat = "-----BEGIN CERTIFICATE-----\n"
        "MIICYDCCAgcCBF3c9vUwCgYIKoZIzj0EAwQwQTEfMB0GA1UEAwwWOTFFV25YSVRu\n"
        "VzNtVFpNMEJBMFVIdzEOMAwGA1UECgwFRWF0b24xDjAMBgNVBAsMBUVhdG9uMB4X\n"
        "DTE5MTEyNTA5NTcwOVoXDTM0MTEyMjA5NTcwOVowQTEfMB0GA1UEAwwWOTFFV25Y\n"
        "SVRuVzNtVFpNMEJBMFVIdzEOMAwGA1UECgwFRWF0b24xDjAMBgNVBAsMBUVhdG9u\n"
        "MIIBSzCCAQMGByqGSM49AgEwgfcCAQEwLAYHKoZIzj0BAQIhAP////8AAAABAAAA\n"
        "AAAAAAAAAAAA////////////////MFsEIP////8AAAABAAAAAAAAAAAAAAAA////\n"
        "///////////8BCBaxjXYqjqT57PrvVV2mIa8ZR0GsMxTsPY7zjw+J9JgSwMVAMSd\n"
        "NgiG5wSTamZ44ROdJreBn36QBEEEaxfR8uEsQkf4vOblY6RA8ncDfYEt6zOg9KE5\n"
        "RdiYwpZP40Li/hp/m47n60p8D54WK84zV2sxXs7LtkBoN79R9QIhAP////8AAAAA\n"
        "//////////+85vqtpxeehPO5ysL8YyVRAgEBA0IABGFnOkLQkKQi5a23X36YgpVT\n"
        "un+QVBDx2IIhpro/WWHu/Bj5gIlZwmw1bpHiQ24EuLoxY6s9MhBwwOzAAsJ1pbcw\n"
        "CgYIKoZIzj0EAwQDRwAwRAIgWKQH6QCxeTr/t9XNY/gczo2KjfIqriaNOt27+sbd\n"
        "+v4CIAz2yg1DT/JPEASivLAa7xQILx5RtzdolxsVLKw0Oz0N\n"
        "-----END CERTIFICATE-----\n";

        char clientCertFileName[11] = "";
        X509 *cert = NULL;
        BIO *bio = BIO_new_mem_buf((void*)certificat, strlen(certificat));
        cert = PEM_read_bio_X509(bio, &cert, NULL, NULL);
        if(cert)
        {
            char buf[512];
            X509_NAME_oneline(X509_get_subject_name(cert), buf, 512);
            printf("subject=%s\n", buf);

            X509_NAME_oneline(X509_get_issuer_name(cert), buf, 512);
            printf("issuer=%s\n", buf);

            unsigned long hash = X509_subject_name_hash(cert);
            printf("hash=%lu\n", buf);

            char hexstring[32];
            snprintf(hexstring, sizeof(hexstring), "%x", hash);
            printf("decimal: %d  hex: %s\n", hash, hexstring);
            hexstring[8] = '\0';
            snprintf(clientCertFileName, sizeof(clientCertFileName), "%s.0", hexstring);

            X509_free(cert);
        }
        BIO_free(bio);

        /* test if client certificate has been yet pairing */
        bool pairing = false;
        memset(&http_opt, 0, sizeof(http_options_t));
        http_opt.user_agent = "Test";
        http_opt.https_no_cert_check = 1;
        http_opt.https_no_Expect = 1;
        CPPUNIT_ASSERT(asprintf(&url, "https://%s%s/%s", address, clientCertUrl, clientCertFileName) != -1);
        retry = RETRY_MAX;
        do {
            content = http_get(url, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (status == 0 && content) free(content);
        } while(retry-- > 0 && status == 0);
        printf("retry=%d\n", retry);
        if (url) free(url);
        CPPUNIT_ASSERT_MESSAGE("Get client certificate", status == 200 || status == 404);
        if (status == 200) {
            CPPUNIT_ASSERT_MESSAGE("Get client certificate", content);
            CPPUNIT_ASSERT(http_json_parse(content, &json_obj) == 0);
            char *result = http_json_get_value(json_obj, "/status");
            printf("result=%s\n", result);
            pairing = strcmp(result, "accepted") == 0 ? true : false;
            json_object_put(json_obj);
        }
        if (content) free(content);

        /* push client certificate if not pairing */
        if (!pairing) {
            printf("Push certificate\n");
            memset(&http_opt, 0, sizeof(http_options_t));
            http_opt.user_agent = "Test";
            http_opt.https_no_cert_check = 1;
            http_opt.https_no_Expect = 1;
            http_opt.login = login;
            http_opt.password = password;
            http_opt.auth_basic = true;
            http_opt.content_type = "application/json";
            CPPUNIT_ASSERT(asprintf(&url, "https://%s%s", address, clientCertUrl) != -1);
            char *certif_request = NULL;
            CPPUNIT_ASSERT(asprintf(&certif_request, "{\"format\": \"PEM\", \"certificate\": \"%s\"}", certificat) != -1);
            content = http_post_data(url, certif_request, timeout_ms, &status, NULL, &http_opt);
            printf("status=%d\n", status);
            if (url) free(url);
            if (certif_request) free(certif_request);
            CPPUNIT_ASSERT_MESSAGE("Post certificat", content && (status == 200));
            if (content) free(content);
        }
        else {
            printf("Push certificate not necessary !!!!!!!\n");
        }
    }
}
