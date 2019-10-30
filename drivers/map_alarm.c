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
#include "map_alarm.h"

/**
 * @function map_alarm_code_find Find a code into a list of error codes
 * @param list_code The error codes list
 * @param code_to_search The code to search
 * @return {integer} 1 if code found else 0
 */
int map_alarm_code_find(const char *list_code, const char *code_to_search)
{
    if (!(list_code && code_to_search)) return 0;

    int res = 0;
    char *list_code_cpy = strdup(list_code);
    char *p0 = list_code_cpy, *p1 = NULL, *code = NULL;

    do {
        code = p0;
        p1 = strstr(p0, ",");
        if (p1) {
            *p1 = 0;
            p0 = p1 + 1;
        }
        if (strcmp(code, code_to_search) == 0) {
            res = 1;
            break;
        }
    } while (p1);

    free(list_code_cpy);
    return res;
}

/**
 * @function map_get_nb_alarms Return the number of alarm in the map
 * @param root The alarm map
 * @return {integer} the number of alarm if success else < 0
 */
int map_get_nb_alarms(alarm_map_t *root) {
    if (!root) return -1 ;

    return root->nb_alarm;
}

/**
 * @function map_add_alarm Add an new alarm in the map
 * @param root The alarm map
 * @param id The alarm to add
 * @return {integer} 0 if success else < 0
 */
int map_add_alarm(alarm_map_t *root, alarm_t *alarm)
{
    if (!(root && alarm)) return -1;

    printf("map_add_alarm add %s\n", alarm->id);

    alarm_elm_t *current_alarm = root->list_alarm;
    if (current_alarm) {
        while (current_alarm->next_alarm) {
            current_alarm = current_alarm->next_alarm;
        }
    }
    alarm_elm_t *new_alarm = (alarm_elm_t *) malloc(sizeof(alarm_elm_t));
    if (new_alarm) {
        memset(new_alarm, 0, sizeof(alarm_elm_t));
        new_alarm->alarm.id = alarm->id ? strdup(alarm->id) : NULL;
        new_alarm->alarm.level = alarm->level;
        new_alarm->alarm.code = alarm->code ? strdup(alarm->code) : NULL;
        new_alarm->alarm.topic = alarm->topic ? strdup(alarm->topic) : NULL;
        if (current_alarm) current_alarm->next_alarm = new_alarm;
        else root->list_alarm = new_alarm;
        root->nb_alarm++;
        return 0;
    }
    return -2;
}

/**
 * @function map_find_alarm Find an alarm in the map
 * @param root The alarm map
 * @param id The alarm id to find
 * @return {object} the alarm found if success else NULL
 */
alarm_elm_t *map_find_alarm(alarm_map_t *root, const char *id)
{
    if (!(root && id)) return NULL;

    alarm_elm_t *current_alarm = root->list_alarm;
    while (current_alarm) {
        printf("map_find_alarm: \"%s\" - \"%s\"\n", current_alarm->alarm.id, id);
        if (strcmp(current_alarm->alarm.id, id) == 0) {
            printf("map_find_alarm %s\n", id);
            return current_alarm;
        }
        current_alarm = current_alarm->next_alarm;
    }
    printf("map_find_alarm not found %s\n", id);
    return NULL;
}

/**
 * @function map_remove_alarm Return an alarm in the map
 * @param root The alarm map
 * @param id The alarm id to remove
 * @return {integer} 0 if success else < 0
 */
int map_remove_alarm(alarm_map_t *root, const char *id) {
    if (!(root && id)) return -1;

    alarm_elm_t *current_alarm = root->list_alarm;
    alarm_elm_t *previous_alarm = NULL;
    while (current_alarm && strcmp(current_alarm->alarm.id, id) != 0) {
       previous_alarm = current_alarm;
       current_alarm = current_alarm->next_alarm;
    }

    if (current_alarm) {
        printf("+ free alarm %s\n", current_alarm->alarm.id);
        if (previous_alarm) previous_alarm->next_alarm = current_alarm->next_alarm;
        else root->list_alarm = current_alarm->next_alarm;
        if (current_alarm->alarm.id) free(current_alarm->alarm.id);
        if (current_alarm->alarm.code) free(current_alarm->alarm.code);
        if (current_alarm->alarm.topic) free(current_alarm->alarm.topic);
        free(current_alarm);
        root->nb_alarm--;
        return 0;
    }
    return -2;
}

/**
 * @function map_remove_all_alarms Remove all alarms in the map
 * @param root The alarm map
 */
void map_remove_all_alarms(alarm_map_t *root) {
    if (!root) return;

    alarm_elm_t *current_alarm = root->list_alarm;
    alarm_elm_t *next_alarm = NULL;

    while (current_alarm) {
        printf("+ free alarm %s\n", current_alarm->alarm.id);
        next_alarm = current_alarm->next_alarm;
        if (current_alarm->alarm.id) free(current_alarm->alarm.id);
        if (current_alarm->alarm.code) free(current_alarm->alarm.code);
        if (current_alarm->alarm.topic) free(current_alarm->alarm.topic);
        free(current_alarm);
        current_alarm = next_alarm;
    }
    root->list_alarm = NULL;
    root->nb_alarm = 0;
}
