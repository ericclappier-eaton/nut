/*  nutdrv_mqtt_map_outlet.c - Map to manage UPS outlets
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
#include "nutdrv_mqtt_map_outlet.h"

/**
 * @function map_get_nb_outlets Return the number of outlets
 * @param outlet_map The outlets array
 * @return {integer} the number of outlet
 */
int map_get_nb_outlet(outlet_map_t *outlet_map)
{
    if (!outlet_map) return -1;

    return outlet_map->nb_outlet;
}

/**
 * @function map_init_outlet Init outlets
 * @param outlet_map The outlets array
 * @param init_nb_outlet The number of outlet
 * @return {integer} 0 if success else < 0 
 */
int map_init_outlet(outlet_map_t *outlet_map, int init_nb_outlet)
{
    if (!(outlet_map && init_nb_outlet > 0)) return -1;

    map_destroy_outlet(outlet_map);
    outlet_map->array_outlet = calloc(init_nb_outlet, sizeof(outlet_t));
    if (outlet_map->array_outlet) {
        outlet_map->nb_outlet = init_nb_outlet;
        int iOutlet = 0;
        while (iOutlet < init_nb_outlet) {
            outlet_map->array_outlet[iOutlet].topic = NULL;
            iOutlet ++;
        }
        return 0;
    }
    return -2;
}

/**
 * @function map_find_outlet Find an outlet in the array
 * @param outlet_map The outlets array
 * @param id The outlet topic to find
 * @return {object} the outlet found if success else NULL
 */
outlet_t *map_find_outlet(outlet_map_t *outlet_map, const char *topic)
{
    if (!(outlet_map && outlet_map->array_outlet && topic)) return NULL;

    int iOutlet = 0;
    //printf("map_find_outlet: %d\n", outlet_map->nb_outlet);
    while (iOutlet < outlet_map->nb_outlet) {
        //printf("map_find_outlet: %s\n", outlet_map->array_outlet[iOutlet].topic);
        if (outlet_map->array_outlet[iOutlet].topic &&
            strcmp(outlet_map->array_outlet[iOutlet].topic, topic) == 0) {
            return &(outlet_map->array_outlet[iOutlet]);
        }
        iOutlet ++;
    }
    //printf("map_find_outlet not found %s\n", topic);
    return NULL;
}

/**
 * @function map_destroy_outlets Remove all outlets
 * @param outlet_map The outlets array
 */
void map_destroy_outlet(outlet_map_t *outlet_map) {
    if (!(outlet_map && outlet_map->array_outlet)) return;

    int iOutlet = 0;
    while (iOutlet < outlet_map->nb_outlet) {
        if (outlet_map->array_outlet[iOutlet].topic) {
            free(outlet_map->array_outlet[iOutlet].topic);
        }
        iOutlet ++;
    }
    free(outlet_map->array_outlet);
    outlet_map->array_outlet = NULL;
    outlet_map->nb_outlet = 0;
}

