/*  map_sensor.c - Map to manage sensor
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
#include "map_sensor.h"
#include <search.h>

/**
 * @function map_is_index_type_sensor Test validity of the index of the sensor type
 * @param root The alarm map
 * @param index_type_sensor The index of the sensor type
 * @return {integer} 0 if success else < 0
 */
int map_is_index_type_sensor(int index_type_sensor) {
    return ((index_type_sensor >= 0) && (index_type_sensor < NB_TYPE_SENSOR)) ? 1 : 0;
}

/**
 * @function map_get_index_type_sensor Get index according the sensor type
 * @param root The alarm map
 * @param index_type_sensor The index type sensor to test
 * @return {integer} 0 if success else < 0
 */
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

/**
 * @function map_get_nb_devices Return the number of sensor device in the map
 * @param root The sensor device map
 * @return {integer} the number of sensor device in the map if success else < 0
 */
int map_get_nb_devices(device_map_t *root) {
    if (!root) return -1 ;

    return root->nb_device;
}

/**
 * @function map_get_nb_init_devices Return the init number of sensor device in the map
 * @param root The sensor device map
 * @return {integer} the init number of sensor device in the map if success else < 0
 */
int map_get_nb_init_devices(device_map_t *root) {
    if (!root) return -1 ;

    return root->nb_init_device;
}

/**
 * @function map_set_nb_init_devices Set the init number of sensor device in the map
 * @param root The sensor device map
 * @param nb_init_device The init number of sensor device in the map to set
 * @return {integer} the init number of sensor device in the map if success else < 0
 */
int map_set_nb_init_devices(device_map_t *root, int nb_init_device) {
    if (!(root && nb_init_device >= 0)) return -1;

    root->nb_init_device = nb_init_device;
    return 0;
}

/**
 * @function map_remove_all_devices Remove all sensor devices in the map
 * @param root The sensor device map
 */
void map_remove_all_devices(device_map_t *root) {
    if (!root) return;

    int iDevice = 0, iTypeSensor, iSubElmt;
    device_t *current_device = root->list_device;
    device_t *next_device = NULL;
    while (current_device) {
        printf("+ free device %d/%s\n", iDevice++, current_device->key_device);
        for (iTypeSensor = 0; iTypeSensor < NB_TYPE_SENSOR; iTypeSensor++) {
            if (current_device->list_type_sensor[iTypeSensor].nb_sensor > 0 && current_device->list_type_sensor[iTypeSensor].list_sensor) {
                for(iSubElmt = 0; iSubElmt < current_device->list_type_sensor[iTypeSensor].nb_sensor; iSubElmt++) {
                    printf("+ free sensor %d %d/%s %d\n", iTypeSensor, iSubElmt,
                            current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor ? current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor : "",
                            current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].enable);
                    if (current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor)
                        free(current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor);
                }
                current_device->list_type_sensor[iTypeSensor].nb_sensor = 0;
                free(current_device->list_type_sensor[iTypeSensor].list_sensor);
                current_device->list_type_sensor[iTypeSensor].list_sensor = NULL;
            }
        }
        if (current_device->key_device) free(current_device->key_device);
        next_device = current_device->next_device;
        free(current_device);
        current_device = next_device;
    }
    root->list_device = NULL;
    root->nb_device = 0;
}

/**
 * @function map_remove_device Remove a sensor device in the map
 * @param root The sensor device map
 * @param index_device The index of the sensor device to remove
 * @return {integer} 0 if success else < 0
 */
int map_remove_device(device_map_t *root, int index_device) {
    if (!(root && index_device >= 0)) return -1;

    int iDevice = 0, iTypeSensor, iSubElmt;
    device_t *current_device = root->list_device;
    device_t *previous_device = NULL;
    while (current_device && iDevice < index_device) {
       previous_device = current_device;
       current_device = current_device->next_device;
       iDevice++;
    }
    if (!current_device && iDevice != index_device) return -2;

    if (current_device) {
        printf("+ free device %d/%s\n", iDevice, current_device->key_device);
        for (iTypeSensor = 0; iTypeSensor < NB_TYPE_SENSOR; iTypeSensor++) {
            if (current_device->list_type_sensor[iTypeSensor].nb_sensor > 0 && current_device->list_type_sensor[iTypeSensor].list_sensor) {
                for(iSubElmt = 0; iSubElmt < current_device->list_type_sensor[iTypeSensor].nb_sensor; iSubElmt++) {
                    printf("+ free sensor %d %d/%s %d\n", iTypeSensor, iSubElmt,
                            current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor ? current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor : "",
                            current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].enable);
                    if (current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor)
                        free(current_device->list_type_sensor[iTypeSensor].list_sensor[iSubElmt].key_sensor);
                }
                current_device->list_type_sensor[iTypeSensor].nb_sensor = 0;
                free(current_device->list_type_sensor[iTypeSensor].list_sensor);
                current_device->list_type_sensor[iTypeSensor].list_sensor = NULL;
            }
        }
        if (current_device->key_device) free(current_device->key_device);
        if (previous_device) previous_device->next_device = current_device->next_device;
        else root->list_device = current_device->next_device;
        free(current_device);
        root->nb_device--;
        return 0;
    }
    return -3;
}

/**
 * @function map_add_device Add a sensor device in the map
 * @param root The sensor device map
 * @param index_device The index of the sensor device to add
 * @param key_device The key of the sensor device to add
 * @return {integer} 0 if success else < 0
 */
int map_add_device(device_map_t *root, int index_device, char *key_device)
{
    if (!(root && key_device)) return -1;

    printf("map_add_device add %d/%s\n", index_device, key_device);
    int iDevice = 0;
    device_t *current_device = root->list_device;
    device_t *previous_device = NULL;
    while (current_device && (index_device == -1 || iDevice < index_device)) {
       previous_device = current_device;
       current_device = current_device->next_device;
       iDevice++;
    }
    if (index_device != -1 && iDevice != index_device) return -2;

    device_t *new_device = (device_t *) malloc(sizeof(device_t));
    if (new_device) {
        memset(new_device, 0, sizeof(device_t));
        new_device->key_device = strdup(key_device);;
        int iTypeSensor;
        for (iTypeSensor = 0; iTypeSensor < NB_TYPE_SENSOR; iTypeSensor++) {
            new_device->list_type_sensor[iTypeSensor].list_sensor = NULL;
            new_device->list_type_sensor[iTypeSensor].nb_sensor = 0;
        }
        if (previous_device) previous_device->next_device = new_device;
        else root->list_device = new_device;
        new_device->next_device = current_device;
        root->nb_device++;
        return 0;
    }
    return -2;
}

/**
 * @function map_move_device Move a sensor device in the map
 * @param root The sensor device map
 * @param index_device The index of the sensor device to move
 * @param new_index_device The new index of the sensor device to move
 * @return {integer} 0 if success else < 0
 */
int map_move_device(device_map_t *root, int index_device, int new_index_device) {
    if (!(root && index_device >= 0)) return -1;

    printf("map_move_device move from %d to %d\n", index_device, new_index_device);
    int i_src_device = 0;
    device_t *src_device = root->list_device;
    device_t *previous_src_device = NULL;
    while (src_device && i_src_device < index_device) {
       previous_src_device = src_device;
       src_device =src_device->next_device;
       i_src_device++;
    }
    if (i_src_device != index_device) return -2;

    int i_dest_device = 0;
    device_t *dest_device = root->list_device;
    device_t *previous_dest_device = NULL;
    while (dest_device && (new_index_device == -1 || i_dest_device < new_index_device)) {
        previous_dest_device = dest_device;
        dest_device = dest_device->next_device;
        i_dest_device++;
    }
    if (new_index_device != -1 && i_dest_device != new_index_device) return -3;

    if (src_device->next_device != dest_device) {
        if (previous_src_device) previous_src_device->next_device = src_device->next_device;
        else root->list_device = src_device->next_device;
        if (dest_device) {
            src_device->next_device = dest_device;
            if (previous_dest_device) previous_dest_device->next_device = src_device;
            else root->list_device = src_device;
        }
        else {
            src_device->next_device = NULL;
            if (previous_dest_device) previous_dest_device->next_device = src_device;
        }
    }
    return 0;
}

/**
 * @function map_get_index_device Return the index of the sensor device in the map according its key
 * @param root The sensor device map
 * @param key_device The key of the sensor device to search
 * @return {integer} the index of the sensor device found if success else < 0
 */
int map_get_index_device(device_map_t *root, char *key_device)
{
    if (!(root && key_device)) return -1;

    int iDevice = 0;
    device_t *current_device = root->list_device;
    while (current_device) {
        if (strcmp(current_device->key_device, key_device) == 0) {
            printf("map_find_device %d/%s\n", iDevice, key_device);
            return iDevice;
        }
        current_device = current_device->next_device;
        iDevice++;
    }
    printf("map_find_device not found %s\n", key_device);
    return -2;
}

/**
 * @function map_find_device Return the sensor device in the map according its key
 * @param root The sensor device map
 * @param key_device The key of the sensor device to search
 * @return {object} the sensor device found if success else NULL
 */
device_t *map_find_device(device_map_t *root, char *key_device)
{
    if (!(root && key_device)) return NULL;

    int iDevice = 0;
    device_t *current_device = root->list_device;
    while (current_device) {
        if (strcmp(current_device->key_device, key_device) == 0) {
            printf("map_find_device %d/%s\n", iDevice, key_device);
            return current_device;
        }
        current_device = current_device->next_device;
        iDevice++;
    }
    printf("map_find_device not found %s\n", key_device);
    return NULL;
}

/**
 * @function map_find_device_by_index Return the sensor device in the map according its index
 * @param root The sensor device map
 * @param index_device The index of the sensor device to search
 * @return {object} the sensor device found if success else NULL
 */
device_t *map_find_device_by_index(device_map_t *root, int index_device)
{
    if (!(root && index_device >= 0)) return NULL;

    int iDevice = 0;
    device_t *current_device = root->list_device;
    while (current_device && iDevice < index_device) {
        current_device = current_device->next_device;
        iDevice++;
    }
    return current_device;
}

/**
 * @function map_add_sensors Add sub-sensors into a sensor device
 * @param root The sensor device map
 * @param key_device The key of the sensor device to add sub-sensors
 * @param type_sensor Type of the sub-sensors to add
 * @param nb_sensor The number of sub-sensors to add
 * @return {integer} 0 if success else < 0
 */
int map_add_sensors(device_map_t *root, char *key_device, int type_sensor, int nb_sensor)
{
    if (!(root && key_device && map_is_index_type_sensor(type_sensor) && nb_sensor > 0)) return -1;

    printf("map_add_sensors add %s %d\n", key_device, nb_sensor);

    device_t *device = map_find_device(root, key_device);
    if (!device) return -2;

    if (device->list_type_sensor[type_sensor].nb_sensor > 0 &&
        device->list_type_sensor[type_sensor].list_sensor) {
        int iSubElmt;
        for(iSubElmt = 0; iSubElmt < device->list_type_sensor[type_sensor].nb_sensor; iSubElmt++) {
            printf("+ sub free_element of %s\n",
            device->list_type_sensor[type_sensor].list_sensor[iSubElmt].key_sensor ?
            device->list_type_sensor[type_sensor].list_sensor[iSubElmt].key_sensor : "");
            if (device->list_type_sensor[type_sensor].list_sensor[iSubElmt].key_sensor)
                free(device->list_type_sensor[type_sensor].list_sensor[iSubElmt].key_sensor);
        }
        device->list_type_sensor[type_sensor].nb_sensor = 0;
        free(device->list_type_sensor[type_sensor].list_sensor);
        device->list_type_sensor[type_sensor].list_sensor = NULL;
    }
    device->list_type_sensor[type_sensor].list_sensor = (sensor_t *) malloc(sizeof(sensor_t) * nb_sensor);
    if (device->list_type_sensor[type_sensor].list_sensor) {
        memset(device->list_type_sensor[type_sensor].list_sensor, 0, sizeof(sizeof(sensor_t) * nb_sensor));
        int iSubElmt;
        for(iSubElmt = 0; iSubElmt < nb_sensor; iSubElmt++) {
            device->list_type_sensor[type_sensor].list_sensor[iSubElmt].enable = -1;
            device->list_type_sensor[type_sensor].list_sensor[iSubElmt].key_sensor = NULL;
        }
        device->list_type_sensor[type_sensor].nb_sensor = nb_sensor;
        return 0;
    }
    return -3;
}

/**
 * @function map_init_sensor Init the key of a sub-sensor into a sensor device
 * @param root The sensor device map
 * @param key_device The key of the sensor device to init sub-sensor
 * @param type_sensor Type of the sub-sensors to init
 * @param index_sensor The index of the sub-sensor to init
 * @param key_sensor The key of the sub-sensor to init
 * @return {integer} 0 if success else < 0
 */
int map_init_sensor(device_map_t *root, char *key_device, int type_sensor, int index_sensor, char *key_sensor)
{
    if (!(root && key_device && map_is_index_type_sensor(type_sensor) && index_sensor >= 0 && key_sensor)) return -1;

    printf("map_init_sensor init %s/%d/%s\n", key_device, index_sensor, key_sensor);

    device_t *device = map_find_device(root, key_device);
    if (!device) return -2;

    if (device->list_type_sensor[type_sensor].nb_sensor > 0 &&
        device->list_type_sensor[type_sensor].list_sensor &&
        index_sensor < device->list_type_sensor[type_sensor].nb_sensor) {
        device->list_type_sensor[type_sensor].list_sensor[index_sensor].key_sensor = strdup(key_sensor);
        return 0;
    }
    return -3;
}

/**
 * @function map_get_index_sensor Return the index of the sub-sensor in a sensor device according its key
 * @param root The sensor device map
 * @param index_device The index of the sensor device
 * @param type_sensor Type of the sub-sensor to search
 * @param key_sensor The key of the sub-sensor to search
 * @return {integer} the index of the sub-sensor found if success else < 0
 */
int map_get_index_sensor(device_map_t *root, int index_device, int type_sensor, char *key_sensor)
{
    if (!(root && index_device >= 0 && map_is_index_type_sensor(type_sensor) && index_device < root->nb_device && key_sensor)) return -1;

    device_t *device = map_find_device_by_index(root, index_device);
    if (!device) return -2;

    int iSensor;
    printf("map_find_sensor: %s\n", key_sensor);
    for(iSensor = 0; iSensor < device->list_type_sensor[type_sensor].nb_sensor; iSensor++) {
        printf("map_find_sensor: %s/%d: test %s\n", key_sensor, iSensor,
               device->list_type_sensor[type_sensor].list_sensor[iSensor].key_sensor ?
               device->list_type_sensor[type_sensor].list_sensor[iSensor].key_sensor : "");
        if (device->list_type_sensor[type_sensor].list_sensor &&
            device->list_type_sensor[type_sensor].list_sensor[iSensor].key_sensor &&
            strcmp(device->list_type_sensor[type_sensor].list_sensor[iSensor].key_sensor, key_sensor) == 0) {
            printf("map_find_sensor %s/%d\n", key_sensor, iSensor);
            return iSensor;
        }
    }
    printf("map_find_sensor not found %d/%s\n", index_device, key_sensor);
    return -3;
}

/**
 * @function map_set_sensor_state Set the state of a sub-sensor in a sensor device
 * @param root The sensor device map
 * @param index_device The index of the sensor device
 * @param type_sensor Type of the sub-sensor
 * @param index_sensor The index of the sub-sensor
 * @param enable The state of the sub-sensor
 * @return {integer} 0 if success else < 0
 */
int map_set_sensor_state(device_map_t *root, int index_device, int type_sensor, int index_sensor, int enable) {
    if (!(root && index_device >= 0 && map_is_index_type_sensor(type_sensor) && index_device < root->nb_device && index_sensor >= 0)) return -1;

    device_t *device = map_find_device_by_index(root, index_device);
    if (!device) return -2;

    if (device->list_type_sensor[type_sensor].list_sensor &&
        index_sensor < device->list_type_sensor[type_sensor].nb_sensor) {
        device->list_type_sensor[type_sensor].list_sensor[index_sensor].enable = enable;
        printf("map_set_sensor_state: %d/%d=%d\n", type_sensor, index_sensor, enable);
        return 0;
    }
    return -3;
}

/**
 * @function map_get_sensor_state Get the state of a sub-sensor in a sensor device
 * @param root The sensor device map
 * @param index_device The index of the sensor device
 * @param type_sensor Type of the sub-sensor
 * @param index_sensor The index of the sub-sensor
 * @return {integer} the state of the sub-sensor if success else < 0
 */
int map_get_sensor_state(device_map_t *root, int index_device, int type_sensor, int index_sensor) {
    if (!(root && index_device >= 0 && map_is_index_type_sensor(type_sensor) && index_device < root->nb_device && index_sensor >= 0)) return -1;

    device_t *device = map_find_device_by_index(root, index_device);
    if (!device) return -2;

    if (device->list_type_sensor[type_sensor].list_sensor &&
        index_sensor < device->list_type_sensor[type_sensor].nb_sensor) {
        printf("map_get_sensor_state: %d/%d=%d\n", type_sensor, index_sensor,
               device->list_type_sensor[type_sensor].list_sensor[index_sensor].enable);
        return device->list_type_sensor[type_sensor].list_sensor[index_sensor].enable;
    }
    return -3;
}