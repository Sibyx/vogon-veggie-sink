idf_component_register(
        SRCS "publisher.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_wifi mqtt json shared
)
