/*  nutdrv_mqtt_map_outlet.h - Map to manage UPS outlets
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

#ifndef MQTT_MAP_OUTLET_H
#define MQTT_MAP_OUTLET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{        
    int switchable;
    int id;    
    char *topic;
} outlet_t;

typedef struct
{
    int nb_outlet;    
    outlet_t *array_outlet;
} outlet_map_t;

int map_get_nb_outlet(outlet_map_t *outlet_map); 

int map_init_outlet(outlet_map_t *outlet_map, int init_nb_outlets);

outlet_t *map_find_outlet(outlet_map_t *outlet_map, const char *topic);

void map_destroy_outlet(outlet_map_t *outlet_map);


#ifdef __cplusplus
}
#endif

#endif /* MQTT_MAP_OUTLET_H */

