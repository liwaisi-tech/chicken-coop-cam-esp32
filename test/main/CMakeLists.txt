idf_component_register(SRCS "test_main.c" "test_sensor_e18.c" "test_cam_reader.c"
                       INCLUDE_DIRS "."
                       REQUIRES unity sensorE18 cam_reader)