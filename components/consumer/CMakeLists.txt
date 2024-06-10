idf_component_register(
        SRCS "consumer.c"
        INCLUDE_DIRS "include"
        REQUIRES shared json mqtt_service mqtt
)
