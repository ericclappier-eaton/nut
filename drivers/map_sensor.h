/*  map_sensor.h - Map to manage sensor
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

#ifndef MAP_SENSOR_H
#define MAP_SENSOR_H

#define NB_TYPE_SENSOR 3

enum TYPE_SENSOR { TYPE_TEMPERATURE,  TYPE_HUMIDITY,  TYPE_INPUTS };

typedef struct
{
    int nb_sensor;
    char **list_key_sensor;
} sensor_map_t;

typedef struct
{
    char *key_device;
    sensor_map_t list_sensor[NB_TYPE_SENSOR];
} device_t;

typedef struct
{
    int nb_device;
    device_t *list_device;
} device_map_t;

int map_get_index_type_sensor(char *type_sensor);

int map_add_devices(device_map_t *root, int nb_device);

void map_remove_devices(device_map_t *root);

int map_init_device(device_map_t *root, int index_device, char *key_device);

int map_find_device(device_map_t *root, char *key_device);

int map_add_sensors(device_map_t *root, char *key_device, int type_sensor, int nb_sensor);

int map_remove_sensors(device_map_t *root, char *key_device);

int map_init_sensor(device_map_t *root, char *key_device, int type_sensor, int index_sensor, char *key_sensor);

int map_find_sensor(device_map_t *root, int index_device, int type_sensor, char *key_sensor);

// Unitary tests
void map_test();

#endif /* MAP_SENSOR_H */

