/*  nutdrv_mqtt_map_alarm.h - Map to manage alarms
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

#ifndef MAP_ALARM_H
#define MAP_ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char *id;
    int level;
    char *code;
    //int state;
    //int active;
    char *topic;
} alarm_t;

typedef struct alarm_elm
{
    alarm_t alarm;
    struct alarm_elm *next_alarm;
} alarm_elm_t;

typedef struct
{
    int nb_alarm;
    int nb_init_alarm;
    alarm_elm_t *list_alarm;
} alarm_map_t;

__attribute__ ((visibility ("default"))) int map_alarm_code_find(const char *list_code, const char *code_to_search);

int map_get_nb_alarms(alarm_map_t *root);

int map_add_alarm(alarm_map_t *root, alarm_t *alarm);

alarm_elm_t *map_find_alarm(alarm_map_t *root, const char *id);

int map_remove_alarm(alarm_map_t *root, const char *id);

void map_remove_all_alarms(alarm_map_t *root);

#ifdef __cplusplus
}
#endif

#endif /* MAP_ALARM_H */
