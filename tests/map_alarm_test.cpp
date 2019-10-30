/* Map_alarm_test - CppUnit map alarm unit test

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

#define CPPUNIT_ASSERT_STREAM(MSG, CONDITION)\
    do {\
        std::ostringstream oss;\
        CPPUNIT_ASSERT_MESSAGE(\
            static_cast<std::ostringstream &>(oss << MSG).str(),\
            CONDITION);\
    } while (0);

class Map_alarm_test : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Map_alarm_test);
		CPPUNIT_TEST(map_alarm_safety_test);
		CPPUNIT_TEST(map_alarm_functional_test);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void map_alarm_safety_test();
	void map_alarm_functional_test();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(Map_alarm_test);

#include "../drivers/map_alarm.h"

void Map_alarm_test::setUp()
{
}

void Map_alarm_test::tearDown()
{
}

void Map_alarm_test::map_alarm_safety_test()
{
    alarm_map_t alarm_map = { 0, 0, NULL};
    alarm_t one_alarm;
	CPPUNIT_ASSERT(map_alarm_code_find(NULL, NULL) == 0);
	CPPUNIT_ASSERT(map_alarm_code_find(NULL, "code") == 0);
    CPPUNIT_ASSERT(map_alarm_code_find("code list", NULL) == 0);
    CPPUNIT_ASSERT(map_get_nb_alarms(NULL) < 0);
    CPPUNIT_ASSERT(map_add_alarm(NULL, NULL) < 0);
    CPPUNIT_ASSERT(map_add_alarm(NULL, &one_alarm) < 0);
    CPPUNIT_ASSERT(map_add_alarm(&alarm_map, NULL) < 0);
    CPPUNIT_ASSERT(map_remove_alarm(NULL, NULL) < 0);
    CPPUNIT_ASSERT(map_remove_alarm(NULL, "id") < 0);
    CPPUNIT_ASSERT(map_remove_alarm(&alarm_map, "") < 0);
    map_remove_all_alarms(NULL);
    map_remove_all_alarms(&alarm_map);
    CPPUNIT_ASSERT(map_find_alarm(NULL, NULL) == NULL);
    CPPUNIT_ASSERT(map_find_alarm(NULL, "id") == NULL);
    CPPUNIT_ASSERT(map_find_alarm(&alarm_map, NULL) == NULL);
}

void Map_alarm_test::map_alarm_functional_test()
{
    alarm_map_t alarm_map = { 0, 0, NULL};
	CPPUNIT_ASSERT_MESSAGE("code 111 found", map_alarm_code_find("111", "111") == 1);
    CPPUNIT_ASSERT_MESSAGE("code 111 found", map_alarm_code_find("111,222", "111") == 1);
    CPPUNIT_ASSERT_MESSAGE("code 222 found", map_alarm_code_find("111,222", "222") == 1);
    CPPUNIT_ASSERT_MESSAGE("code 22 not found", map_alarm_code_find("111,222", "22") == 0);
    CPPUNIT_ASSERT_MESSAGE("code 333 not found", map_alarm_code_find("111,222", "333") == 0);

    int nb_alarm = 3;
    int iAlarm;

    for (iAlarm = 0; iAlarm < nb_alarm; iAlarm ++) {
        alarm_t *alarm = (alarm_t *) malloc(sizeof(alarm_t));
        memset(alarm, 0, sizeof(alarm_t));
        if (asprintf(&alarm->id, "%d", iAlarm)) {};
        CPPUNIT_ASSERT_STREAM("add alarm " << alarm->id, map_add_alarm(&alarm_map, alarm) == 0);
        CPPUNIT_ASSERT_STREAM("found alarm " << alarm->id, map_find_alarm(&alarm_map, alarm->id) != NULL);
        free(alarm->id);
        free(alarm);
    }
    map_remove_all_alarms(&alarm_map);
    CPPUNIT_ASSERT_MESSAGE("remove all alarms", map_get_nb_alarms(&alarm_map) == 0);
}
