/* Map_sensor_test - CppUnit map sensor unit test

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

class Map_sensor_test : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Map_sensor_test);
		CPPUNIT_TEST(map_sensor_safety_test);
		CPPUNIT_TEST(map_sensor_functional_test);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void map_sensor_safety_test();
	void map_sensor_functional_test();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(Map_sensor_test);

#include "../drivers/map_sensor.h"

void Map_sensor_test::setUp()
{
}

void Map_sensor_test::tearDown()
{
}

void Map_sensor_test::map_sensor_safety_test()
{
    device_map_t list_root = { 0, 0, NULL};

    CPPUNIT_ASSERT(map_get_index_type_sensor(NULL) < 0);
    CPPUNIT_ASSERT(map_get_nb_devices(NULL) < 0);
    CPPUNIT_ASSERT(map_get_nb_init_devices(NULL) < 0);
    CPPUNIT_ASSERT(map_set_nb_init_devices(NULL, -1) < 0);
    CPPUNIT_ASSERT(map_set_nb_init_devices(NULL, 0) < 0);
    CPPUNIT_ASSERT(map_set_nb_init_devices(&list_root, -1) < 0);
    CPPUNIT_ASSERT(map_add_device(NULL, -1, NULL) < 0);
    CPPUNIT_ASSERT(map_add_device(NULL, -1, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_add_device(NULL, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_add_device(&list_root, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_move_device(NULL, -1, -1) < 0);
    CPPUNIT_ASSERT(map_move_device(NULL, 0, -1) < 0);
    CPPUNIT_ASSERT(map_move_device(NULL, -1, 0) < 0);
    CPPUNIT_ASSERT(map_move_device(&list_root, -1, 0) < 0);
    CPPUNIT_ASSERT(map_remove_device(NULL, -1) < 0);
    CPPUNIT_ASSERT(map_remove_device(NULL, 0) < 0);
    CPPUNIT_ASSERT(map_remove_device(&list_root, -1) < 0);
    map_remove_all_devices(NULL);
    map_remove_all_devices(&list_root);
    CPPUNIT_ASSERT(map_get_index_device(NULL, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_device(NULL, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_get_index_device(&list_root, NULL) < 0);
    CPPUNIT_ASSERT(map_find_device(NULL, NULL) == NULL);
    CPPUNIT_ASSERT(map_find_device(NULL, (char *) "key") == NULL);
    CPPUNIT_ASSERT(map_find_device(&list_root, NULL) == NULL);
    CPPUNIT_ASSERT(map_add_sensors(NULL, NULL, 0, 0) < 0);
    CPPUNIT_ASSERT(map_add_sensors(NULL, NULL, 0, 1) < 0);
    CPPUNIT_ASSERT(map_add_sensors(NULL, (char *) "key", 0, 0) < 0);
    CPPUNIT_ASSERT(map_add_sensors(NULL, (char *) "key", 0, -1) < 0);
    CPPUNIT_ASSERT(map_add_sensors(&list_root, NULL, 0, 0) < 0);
    CPPUNIT_ASSERT(map_add_sensors(&list_root, NULL, 0, 1) < 0);
    CPPUNIT_ASSERT(map_add_sensors(&list_root, (char *) "key", 0, 0) < 0);
    CPPUNIT_ASSERT(map_add_sensors(&list_root, (char *) "key", 0, -1) < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, NULL, 0, -1, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, NULL, 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, NULL, 0, 0, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, (char *) "key", 0, -1, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, (char *) "key", 0, -1, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, (char *) "key", 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(NULL, (char *) "key", 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, NULL, 0, -1, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, NULL, 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, NULL, 0, 0, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, (char *) "key", 0, -1, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, (char *) "key", 0, -1, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, (char *) "key", 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_init_sensor(&list_root, (char *) "key", 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(NULL, -1, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(NULL, 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(NULL, -1, 0, (char *) "key") < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(&list_root, -1, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(&list_root, 0, 0, NULL) < 0);
    CPPUNIT_ASSERT(map_get_index_sensor(&list_root, -1, 0, (char *) "key") < 0);

    CPPUNIT_ASSERT(map_init_sensor(&list_root, (char *) "key", 0, 0, (char *) "key") < 0);
}

void Map_sensor_test::map_sensor_functional_test()
{
    device_map_t list_root = { 0, 0, NULL};

    CPPUNIT_ASSERT_MESSAGE("is_index_type_sensor < 0", map_is_index_type_sensor(-1) == 0);
    CPPUNIT_ASSERT_MESSAGE("is_index_type_sensor 0", map_is_index_type_sensor(0) == 1);
    CPPUNIT_ASSERT_MESSAGE("is_index_type_sensor 1", map_is_index_type_sensor(1) == 1);
    CPPUNIT_ASSERT_MESSAGE("is_index_type_sensor 2", map_is_index_type_sensor(2) == 1);
    CPPUNIT_ASSERT_MESSAGE("is_index_type_sensor 3", map_is_index_type_sensor(3) == 0);

    int nb_device = 3, nb_init_device = 3, nb_sensor = 3;
    char device_name[10];
    char sensor_name[10];
    int iDevice, iType, iSensor;

    map_set_nb_init_devices(&list_root, nb_init_device);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("test nb device", nb_init_device, map_get_nb_init_devices(&list_root));

    /* Add 3 devices and remove by the end */
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("add device " << iDevice << "/\"" << device_name << "\"", map_add_device(&list_root, iDevice, device_name) == 0);
        CPPUNIT_ASSERT_STREAM("found device \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == iDevice);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 3, list_root.nb_device);
    for (iDevice = nb_device - 1; iDevice >= 0; iDevice--) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("remove device " << iDevice << " \"" << device_name << "\"", map_remove_device(&list_root, iDevice) == 0);
        CPPUNIT_ASSERT_STREAM("not found device " << "\"" << device_name << "\"", map_get_index_device(&list_root, device_name) < 0);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 0, list_root.nb_device);

    /* Add 3 devices and remove by the beginning */
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("add " << iDevice << "\"" << device_name << "\"", map_add_device(&list_root, iDevice, device_name) == 0);
        CPPUNIT_ASSERT_STREAM("found device " << "\"" << device_name << "\"", map_get_index_device(&list_root, device_name) == iDevice);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 3, list_root.nb_device);
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("remove device 0 " << "\"" << device_name << "\"", map_remove_device(&list_root, 0) == 0);
        CPPUNIT_ASSERT_STREAM("not found device " << "\"" << device_name << "\"", map_get_index_device(&list_root, device_name) < 0);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 0, list_root.nb_device);

    /* Add 3 devices */
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("add 0 " << "\"" << device_name << "\"", map_add_device(&list_root, iDevice, device_name) == 0);
        CPPUNIT_ASSERT_STREAM("found device " << "\"" << device_name << "\"", map_get_index_device(&list_root, device_name) == iDevice);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 3, list_root.nb_device);
    /* Add "DD" at beginning ("DD" -> "A" -> "B" -> "C") */
    printf("**** Add \"DD\" at beginning\n");
    strncpy(device_name, "DD", sizeof(device_name));
    CPPUNIT_ASSERT_STREAM("add device " << "\"" << device_name << "\"", map_add_device(&list_root, 0, device_name) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 4, list_root.nb_device);
    CPPUNIT_ASSERT_STREAM("found index device 0: \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 1", map_get_index_device(&list_root, (char *) "A") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 2", map_get_index_device(&list_root, (char *) "B") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 3", map_get_index_device(&list_root, (char *) "C") == 3);
    /* Add "EE" between "A" and "B" ("DD" -> "A" -> "EE" -> "B" -> "C") */
    printf("**** Add \"EE1\" between \"A\" and \"B\"\n");
    strncpy(device_name, "EE", sizeof(device_name));
    CPPUNIT_ASSERT_STREAM("add device " << "\"" << device_name << "\"", map_add_device(&list_root, 2, device_name) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 5, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD1\": 0", map_get_index_device(&list_root, (char *) "DD") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 1", map_get_index_device(&list_root, (char *) "A") == 1);
    CPPUNIT_ASSERT_STREAM("found index device 2: \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 3", map_get_index_device(&list_root, (char *) "B") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 4", map_get_index_device(&list_root, (char *) "C") == 4);
    /* Add "FF" at the end ("DD" -> "A" -> "EE" -> "B" -> "C" -> "FF") */
    printf("**** Add \"FF\" at the end\n");
    strncpy(device_name, "FF", sizeof(device_name));
    CPPUNIT_ASSERT_STREAM("add device at the end \"" << device_name << "\"", map_add_device(&list_root, -1, device_name) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 0", map_get_index_device(&list_root, (char *) "DD") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 1", map_get_index_device(&list_root, (char *) "A") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 2", map_get_index_device(&list_root, (char *) "EE") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 3", map_get_index_device(&list_root, (char *) "B") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 4", map_get_index_device(&list_root, (char *) "C") == 4);
    CPPUNIT_ASSERT_STREAM("found index device 5: \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == 5);

    /* Move "FF" at the beginning ("FF" -> "DD" -> "A" -> "EE" -> "B" -> "C") */
    printf("**** Move \"FF\" at the beginning\n");
    CPPUNIT_ASSERT_MESSAGE("move \"FF\" device at the beginning", map_move_device(&list_root, 5, 0) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"FF\": 0", map_get_index_device(&list_root, (char *) "FF") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 1", map_get_index_device(&list_root, (char *) "DD") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 2", map_get_index_device(&list_root, (char *) "A") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 3", map_get_index_device(&list_root, (char *) "EE") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 4", map_get_index_device(&list_root, (char *) "B") == 4);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 5", map_get_index_device(&list_root, (char *) "C") == 5);

    /* Move "FF" from 0 to 1 ("FF" -> "DD" -> "A" -> "EE" -> "B" -> "C") */
    printf("**** Move \"FF\" from 0 to 1: No changement\n");
    CPPUNIT_ASSERT_MESSAGE("move \"2_1\" device from 0 to 1", map_move_device(&list_root, 0, 1) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"FF\": 0", map_get_index_device(&list_root, (char *) "FF") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 1", map_get_index_device(&list_root, (char *) "DD") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 2", map_get_index_device(&list_root, (char *) "A") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 3", map_get_index_device(&list_root, (char *) "EE") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 4", map_get_index_device(&list_root, (char *) "B") == 4);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 5", map_get_index_device(&list_root, (char *) "C") == 5);

    /* Move "FF" from 1 to 0 ("DD" -> "FF" -> "A" -> "EE" -> "B" -> "C") */
    printf("**** Move \"FF\" from 1 to 0\n");
    CPPUNIT_ASSERT_MESSAGE("move \"FF\" device from 1 to 0", map_move_device(&list_root, 1, 0) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 0", map_get_index_device(&list_root, (char *) "DD") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"FF\": 1", map_get_index_device(&list_root, (char *) "FF") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 2", map_get_index_device(&list_root, (char *) "A") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 3", map_get_index_device(&list_root, (char *) "EE") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 4", map_get_index_device(&list_root, (char *) "B") == 4);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 5", map_get_index_device(&list_root, (char *) "C") == 5);

    /* Move "FF" from 1 to 4 ("DD" -> "A" -> "EE" -> "FF" -> "B" -> "C") */
    printf("**** Move \"FF\" from 1 to 4\n");
    CPPUNIT_ASSERT_MESSAGE("move \"FF\" device from 1 to 4", map_move_device(&list_root, 1, 4) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 0", map_get_index_device(&list_root, (char *) "DD") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 1", map_get_index_device(&list_root, (char *) "A") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 2", map_get_index_device(&list_root, (char *) "EE") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"FF\": 3", map_get_index_device(&list_root, (char *) "FF") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 4", map_get_index_device(&list_root, (char *) "B") == 4);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 5", map_get_index_device(&list_root, (char *) "C") == 5);

    /* Move "FF" at the end ("DD" -> "A" -> "EE" -> "B" -> "C" -> "FF") */
    printf("**** Move \"FF\" at the end\n");
    CPPUNIT_ASSERT_MESSAGE("move \"FF\" device at the end", map_move_device(&list_root, 3, -1) == 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("found index device \"DD\": 0", map_get_index_device(&list_root, (char *) "DD") == 0);
    CPPUNIT_ASSERT_MESSAGE("found index device \"A\": 1", map_get_index_device(&list_root, (char *) "A") == 1);
    CPPUNIT_ASSERT_MESSAGE("found index device \"EE\": 2", map_get_index_device(&list_root, (char *) "EE") == 2);
    CPPUNIT_ASSERT_MESSAGE("found index device \"B\": 3", map_get_index_device(&list_root, (char *) "B") == 3);
    CPPUNIT_ASSERT_MESSAGE("found index device \"C\": 4", map_get_index_device(&list_root, (char *) "C") == 4);
    CPPUNIT_ASSERT_MESSAGE("found index device \"FF\": 5", map_get_index_device(&list_root, (char *) "FF") == 5);

    printf("**** Remove device previously added\n");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 6, list_root.nb_device);
    CPPUNIT_ASSERT_MESSAGE("remove device 0 \"DD\"", map_remove_device(&list_root, 0) == 0);
    CPPUNIT_ASSERT_MESSAGE("not found device \"DD\"", map_get_index_device(&list_root, (char *) "DD") < 0);
    CPPUNIT_ASSERT_MESSAGE("remove device 0 \"EE\"", map_remove_device(&list_root, 1) == 0);
    CPPUNIT_ASSERT_MESSAGE("not found device \"EE\"", map_get_index_device(&list_root, (char *) "EE") < 0);
    CPPUNIT_ASSERT_MESSAGE("remove device 0 \"FF\"", map_remove_device(&list_root, 3) == 0);
    CPPUNIT_ASSERT_MESSAGE("not found device \"FF\"", map_get_index_device(&list_root, (char *) "FF") < 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 3, list_root.nb_device);
    printf("**** Test initial devices\n");
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("found device \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == iDevice);
    }
    printf("**** Remove all nodes\n");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 3, list_root.nb_device);
    map_remove_all_devices (&list_root);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Nb count device", 0, list_root.nb_device);

    printf("**** Add devices and sensors\n");
    CPPUNIT_ASSERT_STREAM("set init device " << nb_device, map_set_nb_init_devices(&list_root, nb_device) == 0);
    for (iDevice = 0; iDevice < nb_device; iDevice++) {
        snprintf(device_name, sizeof(device_name), "%c", iDevice + 'A');
        CPPUNIT_ASSERT_STREAM("add device \"" << device_name << "\"", map_add_device(&list_root, iDevice, device_name) == 0);
        CPPUNIT_ASSERT_STREAM("found device \"" << device_name << "\"", map_get_index_device(&list_root, device_name) == iDevice);
        for (iType = 0; iType < NB_TYPE_SENSOR; iType++) {
            CPPUNIT_ASSERT_STREAM("add sensor in device: " << nb_sensor << "\"" << device_name << "/" << iType << "\"", map_add_sensors(&list_root, device_name, iType, nb_sensor) == 0);
            for (iSensor = 0; iSensor < nb_sensor; iSensor++) {
                snprintf(sensor_name, sizeof(sensor_name), "%s_%d_%d", device_name, iType, iSensor);
                CPPUNIT_ASSERT_STREAM("init sensor \"" << sensor_name << "\"", map_init_sensor(&list_root, device_name, iType, iSensor, sensor_name) == 0);
                CPPUNIT_ASSERT_STREAM("found sensor " << sensor_name, map_get_index_sensor(&list_root, iDevice, iType, sensor_name) == iSensor);
                CPPUNIT_ASSERT_STREAM("set sensor state " << sensor_name, map_set_sensor_state(&list_root, iDevice, iType, iSensor, 1) == 0);
                CPPUNIT_ASSERT_STREAM("get sensor state " << sensor_name, map_get_sensor_state(&list_root, iDevice, iType, iSensor) == 1);
            }
        }
    }
    map_remove_all_devices(&list_root);
}
