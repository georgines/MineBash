add_library(PortaSerial STATIC
    PortaSerial.cpp
)

target_include_directories(PortaSerial PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(PortaSerial PUBLIC
    pico_stdlib
    hardware_uart
    hardware_gpio
)
