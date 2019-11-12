/* nutdrv_mqtt_map_outlet_test - CppUnit map outlet unit test

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

class Map_outlet_test : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Map_outlet_test);
		CPPUNIT_TEST(map_outlet_safety_test);
		CPPUNIT_TEST(map_outlet_functional_test);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void map_outlet_safety_test();
	void map_outlet_functional_test();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(Map_outlet_test);

#include "../drivers/nutdrv_mqtt_map_outlet.h"

void Map_outlet_test::setUp()
{
}

void Map_outlet_test::tearDown()
{
}

void Map_outlet_test::map_outlet_safety_test()
{
    outlet_map_t outlet_map;
    outlet_map.nb_outlet = 0;
    outlet_map.array_outlet = NULL;

    CPPUNIT_ASSERT(map_get_nb_outlet(NULL) == -1);
    CPPUNIT_ASSERT(map_init_outlet(NULL, 0) == -1);
    CPPUNIT_ASSERT(map_find_outlet(NULL, NULL) == NULL);
    CPPUNIT_ASSERT(map_find_outlet(NULL, "topic") == NULL);
    CPPUNIT_ASSERT(map_find_outlet(&outlet_map, NULL) == NULL);
    map_destroy_outlet(NULL);
    map_destroy_outlet(&outlet_map);
}

void Map_outlet_test::map_outlet_functional_test()
{
    int init_nb_outlet = 3;

    outlet_map_t outlet_map;
    outlet_map.nb_outlet = 0;
    outlet_map.array_outlet = NULL;

    map_init_outlet(&outlet_map, init_nb_outlet);
    CPPUNIT_ASSERT_MESSAGE("init outlets", map_get_nb_outlet(&outlet_map) == init_nb_outlet);
    int iOutlet;
    for (iOutlet = 0; iOutlet < init_nb_outlet; iOutlet ++) {
        if (asprintf(&(outlet_map.array_outlet[iOutlet].topic), "%d", iOutlet)) {};
    }
    char *topic = NULL;
    for (iOutlet = 0; iOutlet < init_nb_outlet; iOutlet ++) {
        if (asprintf(&topic, "%d", iOutlet)) {};
        CPPUNIT_ASSERT_STREAM("found outlet " << topic, map_find_outlet(&outlet_map, topic) != NULL);
        free(topic);
    }
    map_destroy_outlet(&outlet_map);
    CPPUNIT_ASSERT_MESSAGE("remove all outlets", map_get_nb_outlet(&outlet_map) == 0);
}
