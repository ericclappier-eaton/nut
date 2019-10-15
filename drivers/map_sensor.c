/*  map_sensor.c - Map to manage sensor
 *
 *  Copyright (C)
 *
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
#include "map_sensor.h"
#include <search.h>

static int is_index_type_sensor(int index_type_sensor) {
    return ((index_type_sensor >= 0) && (index_type_sensor < NB_TYPE_SENSOR)) ? 1 : 0;
}

int map_get_index_type_sensor(char *type_sensor) {
    if (type_sensor) {
        if (strcmp(type_sensor, "temperatures") == 0) {
            return TYPE_TEMPERATURE;
        }
        else if (strcmp(type_sensor, "humidities") == 0) {
            return TYPE_HUMIDITY;
        }
        else if (strcmp(type_sensor, "digitalInputs") == 0) {
            return TYPE_INPUTS;
        }
    }
    return -1;
}

void map_remove_devices(device_map_t *root) {
    if (!root) return;

    int iDevice, iTypeSensor, iSubElmt;
    for (iDevice = 0; iDevice < root->nb_device; iDevice++) {
        printf("+ free device %d/%s\n", iDevice, root->list_device[iDevice].key_device);
        for (iTypeSensor = 0; iTypeSensor < NB_TYPE_SENSOR; iTypeSensor++) {
            if (root->list_device[iDevice].list_sensor[iTypeSensor].nb_sensor > 0 && root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor) {
                for(iSubElmt = 0; iSubElmt < root->list_device[iDevice].list_sensor[iTypeSensor].nb_sensor; iSubElmt++) {
                    printf("+ free sensor %d %d/%s\n", iTypeSensor, iSubElmt, root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor[iSubElmt]);
                    free(root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor[iSubElmt]);
                }
                root->list_device[iDevice].list_sensor[iTypeSensor].nb_sensor = 0;
                free(root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor);
                root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor = NULL;
            }
        }
        if (root->list_device[iDevice].key_device) free(root->list_device[iDevice].key_device);
    }
    if (root->list_device) {
        printf("+ free device list\n");
        free(root->list_device);
    }
    root->list_device = NULL;
    root->nb_device = 0;
}

int map_add_devices(device_map_t *root, int nb_device)
{
    if (!(root && nb_device > 0)) return -1;

    printf("map_add_devices add %d\n", nb_device);
    map_remove_devices(root);
    root->list_device = (device_t *) malloc(sizeof(device_t) * nb_device);
    if (root->list_device) {
        root->nb_device = nb_device;
        memset(root->list_device, 0, sizeof(sizeof(device_t) * nb_device));
        int iDevice;
        for (iDevice = 0; iDevice < nb_device; iDevice++) {
            root->list_device[iDevice].key_device = NULL;
            int iTypeSensor;
            for (iTypeSensor = 0; iTypeSensor < NB_TYPE_SENSOR; iTypeSensor++) {
                root->list_device[iDevice].list_sensor[iTypeSensor].list_key_sensor = NULL;
                root->list_device[iDevice].list_sensor[iTypeSensor].nb_sensor = 0;
            }
        }
        return 0;
    }
    return -2;
}

int map_init_device(device_map_t *root, int index_device, char *key_device)
{
    if (!(root && index_device >= 0 &&
          index_device < root->nb_device && key_device)) return -1;
    if (!(root->list_device)) return -2;

    printf("map_init_device init %d/%s\n", index_device, key_device);
    root->list_device[index_device].key_device = strdup(key_device);
    return 0;
}

int map_find_device(device_map_t *root, char *key_device)
{
    if (!(root && key_device)) return -1;
    if (!(root->list_device)) return -2;

    int iDevice;
    for(iDevice = 0; iDevice < root->nb_device; iDevice++) {
        if (strcmp(root->list_device[iDevice].key_device, key_device) == 0) {
            printf("map_find_device %d/%s\n", iDevice, key_device);
            return iDevice;
        }
    }
    printf("map_find_device not found /%s\n", key_device);
    return -3;
}

int map_add_sensors(device_map_t *root, char *key_device, int type_sensor, int nb_sensor)
{
    if (!(root && key_device && is_index_type_sensor(type_sensor) && nb_sensor > 0)) return -1;

    printf("map_add_sensors add %s %d\n", key_device, nb_sensor);

    int index_device = map_find_device(root, key_device);
    if (index_device >= 0) {
        if (root->list_device[index_device].list_sensor[type_sensor].nb_sensor > 0 && root->list_device[index_device].list_sensor[type_sensor].list_key_sensor) {
            int iSubElmt;
            for(iSubElmt = 0; iSubElmt < root->list_device[index_device].list_sensor[type_sensor].nb_sensor; iSubElmt++) {
                printf("+ sub free_element of %s\n", root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[iSubElmt]);
                free(root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[iSubElmt]);
            }
            root->list_device[index_device].list_sensor[type_sensor].nb_sensor = 0;
            free(root->list_device[index_device].list_sensor[type_sensor].list_key_sensor);
            root->list_device[index_device].list_sensor[type_sensor].list_key_sensor = NULL;
        }
        root->list_device[index_device].list_sensor[type_sensor].list_key_sensor = (char **) malloc(sizeof(char *) * nb_sensor);
        if (root->list_device[index_device].list_sensor[type_sensor].list_key_sensor) {
            memset(root->list_device[index_device].list_sensor[type_sensor].list_key_sensor, 0, sizeof(sizeof(char *) * nb_sensor));
            int iSubElmt;
            for(iSubElmt = 0; iSubElmt < nb_sensor; iSubElmt++) {
                root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[iSubElmt] = NULL;
            }
            root->list_device[index_device].list_sensor[type_sensor].nb_sensor = nb_sensor;
            return 0;
        }
        return -2;
    }
    return -3;
}

int map_init_sensor(device_map_t *root, char *key_device, int type_sensor, int index_sensor, char *key_sensor)
{
    if (!(root && key_device && is_index_type_sensor(type_sensor) && index_sensor >= 0 && key_sensor)) return -1;

    printf("map_init_sensor init %s/%d/%s\n", key_device, index_sensor, key_sensor);

    int index_device = map_find_device(root, key_device);
    if (index_device >= 0) {
        if (root->list_device[index_device].list_sensor[type_sensor].nb_sensor > 0 &&
            root->list_device[index_device].list_sensor[type_sensor].list_key_sensor &&
            index_sensor < root->list_device[index_device].list_sensor[type_sensor].nb_sensor) {
            root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[index_sensor] = strdup(key_sensor);
            return 0;
        }
        return -2;
    }
    return -3;
}

int map_find_sensor(device_map_t *root, int index_device, int type_sensor, char *key_sensor)
{
    if (!(root && index_device >= 0 && is_index_type_sensor(type_sensor) && index_device < root->nb_device && key_sensor)) return -1;

    int iSensor;
    printf("map_find_sensor: %d/%s\n", index_device, key_sensor);
    for(iSensor = 0; iSensor < root->list_device[index_device].list_sensor[type_sensor].nb_sensor; iSensor++) {
        printf("map_find_sensor: %d/%s/%d: test %s\n", index_device, key_sensor, iSensor, root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[iSensor]);
        if (root->list_device[index_device].list_sensor[type_sensor].list_key_sensor &&
            strcmp(root->list_device[index_device].list_sensor[type_sensor].list_key_sensor[iSensor], key_sensor) == 0) {
            printf("map_find_sensor %d/%s/%d\n", index_device, key_sensor, iSensor);
            return iSensor;
        }
    }
    printf("map_find_sensor not found %d/%s\n", index_device, key_sensor);
    return -2;  // not found
}

/*-------------------------------------------------------------*/
/* Unitary tests function                                      */
/*-------------------------------------------------------------*/

int nb_error = 0;
int nb_test = 0;

device_map_t g_list_root = { 0, NULL};

void RESULT_TEST_MSG(int result, const char *msg, ...)
{
    nb_test ++;
    char *buffer_msg = NULL;
    if (msg) {
        va_list args;
        va_start(args, msg);
        vasprintf(&buffer_msg, msg, args);
        va_end(args);
    }
    else {
        asprintf(&buffer_msg, "Test %d", nb_test);
    }
    if (result) {
        printf("-> %s OK\n", buffer_msg);
    }
    else {
        nb_error ++;
        printf("-> ERROR: %s NOK !!!\n", buffer_msg);
    }
    if (buffer_msg) free(buffer_msg);
}

void RESULT_TEST(int result)
{
    RESULT_TEST_MSG(result, NULL);
}

void map_test()
{

    printf("\nBEGIN TEST\n");

    //  Safety tests
    {
        RESULT_TEST(map_get_index_type_sensor(NULL) < 0);
        RESULT_TEST(map_add_devices(NULL, 0) < 0);
        RESULT_TEST(map_add_devices(&g_list_root, 0) < 0);
        map_remove_devices(NULL);
        map_remove_devices(&g_list_root);
        RESULT_TEST(map_init_device(NULL, -1, NULL) < 0);
        RESULT_TEST(map_init_device(NULL, 0, NULL) < 0);
        RESULT_TEST(map_init_device(NULL, 0, "key") < 0);
        RESULT_TEST(map_init_device(&g_list_root, -1, NULL) < 0);
        RESULT_TEST(map_init_device(&g_list_root, 0, NULL) < 0);
        RESULT_TEST(map_init_device(&g_list_root, 0, "key") < 0);
        RESULT_TEST(map_find_device(NULL, NULL) == -1);
        RESULT_TEST(map_find_device(&g_list_root, NULL) == -1);
        RESULT_TEST(map_add_sensors(NULL, NULL, 0, 0) < 0);
        RESULT_TEST(map_add_sensors(NULL, NULL, 0, 1) < 0);
        RESULT_TEST(map_add_sensors(NULL, "key", 0, 0) < 0);
        RESULT_TEST(map_add_sensors(NULL, "key", 0, -1) < 0);
        RESULT_TEST(map_add_sensors(&g_list_root, NULL, 0, 0) < 0);
        RESULT_TEST(map_add_sensors(&g_list_root, NULL, 0, 1) < 0);
        RESULT_TEST(map_add_sensors(&g_list_root, "key", 0, 0) < 0);
        RESULT_TEST(map_add_sensors(&g_list_root, "key", 0, -1) < 0);
        RESULT_TEST(map_init_sensor(NULL, NULL, 0, -1, NULL) < 0);
        RESULT_TEST(map_init_sensor(NULL, NULL, 0, 0, NULL) < 0);
        RESULT_TEST(map_init_sensor(NULL, NULL, 0, 0, "key") < 0);
        RESULT_TEST(map_init_sensor(NULL, "key", 0, -1, "key") < 0);
        RESULT_TEST(map_init_sensor(NULL, "key", 0, -1, NULL) < 0);
        RESULT_TEST(map_init_sensor(NULL, "key", 0, 0, NULL) < 0);
        RESULT_TEST(map_init_sensor(NULL, "key", 0, 0, NULL) < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, NULL, 0, -1, NULL) < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, NULL, 0, 0, NULL) < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, NULL, 0, 0, "key") < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, "key", 0, -1, "key") < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, "key", 0, -1, NULL) < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, "key", 0, 0, NULL) < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, "key", 0, 0, NULL) < 0);
        RESULT_TEST(map_find_sensor(NULL, -1, 0, NULL) < 0);
        RESULT_TEST(map_find_sensor(NULL, 0, 0, NULL) < 0);
        RESULT_TEST(map_find_sensor(NULL, -1, 0, "key") < 0);
        RESULT_TEST(map_find_sensor(&g_list_root, -1, 0, NULL) < 0);
        RESULT_TEST(map_find_sensor(&g_list_root, 0, 0, NULL) < 0);
        RESULT_TEST(map_find_sensor(&g_list_root, -1, 0, "key") < 0);

        RESULT_TEST(map_init_device(&g_list_root, 0, "key") < 0);
        RESULT_TEST(map_init_sensor(&g_list_root, "key", 0, 0, "key") < 0);
    }

    //  Functional tests
    {
        RESULT_TEST_MSG(is_index_type_sensor(-1) == 0, "is_index_type_sensor < 0");
        RESULT_TEST_MSG(is_index_type_sensor(0) == 1, "is_index_type_sensor 0");
        RESULT_TEST_MSG(is_index_type_sensor(1) == 1, "is_index_type_sensor 1");
        RESULT_TEST_MSG(is_index_type_sensor(2) == 1, "is_index_type_sensor 2");
        RESULT_TEST_MSG(is_index_type_sensor(3) == 0, "is_index_type_sensor 3");

        int nb_device = 3;
        int nb_sensor = 3;
        char device_name[10];
        char sensor_name[10];
        int iDevice, iType, iSensor;
        RESULT_TEST_MSG(map_add_devices(&g_list_root, nb_device) == 0, "add %d device", nb_device);
        for (iDevice = 0; iDevice < nb_device; iDevice++) {
            snprintf(device_name, sizeof(device_name), "%d", iDevice);
            RESULT_TEST_MSG(map_init_device(&g_list_root, iDevice, device_name) == 0, "init device %d \"%s\"", iDevice, device_name);
            RESULT_TEST_MSG(map_find_device(&g_list_root, device_name) == iDevice, "found device \"%s\"", device_name);
            for (iType = 0; iType < NB_TYPE_SENSOR; iType++) {
                RESULT_TEST_MSG(map_add_sensors(&g_list_root, device_name, iType, nb_sensor) == 0, "add %d sensor in device \"%s\"", nb_sensor, device_name);
                for (iSensor = 0; iSensor < nb_sensor; iSensor++) {
                    snprintf(sensor_name, sizeof(sensor_name), "%s_%d", device_name, iSensor);
                    RESULT_TEST_MSG(map_init_sensor(&g_list_root, device_name, iType, iSensor, sensor_name) == 0, "init sensor \"%s\"", sensor_name);
                    RESULT_TEST_MSG(map_find_sensor(&g_list_root, iDevice, iType, sensor_name) == iSensor, "found sensor %s", sensor_name);
                }
            }
        }
        map_remove_devices(&g_list_root);
    }

    if (nb_error > 0) {
        printf("\n--------------------------------------------\n");
        printf(" TEST ENDED: ERROR %d / %d test passed !!!\n", nb_test - nb_error, nb_test);
        printf("--------------------------------------------\n");
    }
    else {
        printf("\nTEST ENDED: All %d tests passed\n", nb_test);
    }
}


