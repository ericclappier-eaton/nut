/*  map_sensor.h - Map to manage sensor
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

#ifndef MAP_SENSOR_H
#define MAP_SENSOR_H

#define NB_TYPE_SENSOR 3

enum TYPE_SENSOR { TYPE_TEMPERATURE,  TYPE_HUMIDITY,  TYPE_INPUTS };

typedef struct
{
    int enable;
    char *key_sensor;
} sensor_t;

typedef struct
{
    int nb_sensor;
    //char **list_key_sensor;
    sensor_t *list_sensor;
} sensor_map_t;

typedef struct device
{
    char *key_device;
    sensor_map_t list_type_sensor[NB_TYPE_SENSOR];
    struct device *next_device;
} device_t;

typedef struct
{
    int nb_device;
    int nb_init_device;
    device_t *list_device;
} device_map_t;

int map_is_index_type_sensor(int index_type_sensor);

int map_get_index_type_sensor(char *type_sensor);

int map_get_nb_devices(device_map_t *root);

int map_get_nb_init_devices(device_map_t *root);

int map_set_nb_init_devices(device_map_t *root, int nb_device);

int map_add_device(device_map_t *root, int index_device, char *key_device);

int map_move_device(device_map_t *root, int index_device, int new_index_device);

int map_remove_device(device_map_t *root, int index_device);

void map_remove_all_devices(device_map_t *root);

int map_get_index_device(device_map_t *root, char *key_device);

device_t *map_find_device(device_map_t *root, char *key_device);

int map_add_sensors(device_map_t *root, char *key_device, int type_sensor, int nb_sensor);

int map_init_sensor(device_map_t *root, char *key_device, int type_sensor, int index_sensor, char *key_sensor);

int map_get_index_sensor(device_map_t *root, int index_device, int type_sensor, char *key_sensor);

int map_set_sensor_state(device_map_t *root, int index_device, int type_sensor, int index_sensor, int enable);

int map_get_sensor_state(device_map_t *root, int index_device, int type_sensor, int index_sensor);

// Unitary tests
void map_test();

#endif /* MAP_SENSOR_H */
