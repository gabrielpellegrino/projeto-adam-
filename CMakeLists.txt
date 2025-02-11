# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(proj_fin_gabriel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(proj_fin_gabriel 
    proj_fin_gabriel.c
    ssd1306_i2c.c
)

pico_set_program_name(proj_fin_gabriel "proj_fin_gabriel")
pico_set_program_version(proj_fin_gabriel "0.1")

# Enable standard output over UART/USB
pico_enable_stdio_uart(proj_fin_gabriel 0)
pico_enable_stdio_usb(proj_fin_gabriel 1)

# Add standard Pico SDK libraries
target_link_libraries(proj_fin_gabriel 
    pico_stdlib          # Funções padrão (UART, USB, etc.)
    hardware_pio         # Controlador PIO (necessário para LEDs WS2812B)
    hardware_adc         # ADC (leitura do microfone)
    hardware_gpio        # GPIO (LEDs, botões)
    hardware_sync        # Controle de sincronização de hardware
    hardware_i2c         # I2C (controlador OLED)
)

# Add include directories
target_include_directories(proj_fin_gabriel PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}  # Inclui diretório atual
)

# Generate extra outputs (UF2, HEX, BIN)
pico_add_extra_outputs(proj_fin_gabriel)
