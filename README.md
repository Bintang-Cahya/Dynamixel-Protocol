# Dynamixel Protocol 1.0 STM32 Library

Lightweight STM32 HAL library for controlling Dynamixel servos using **Dynamixel Protocol 1.0** over UART.

This library provides simple APIs for:

* Reading and writing Dynamixel registers
* Motion control
* Torque and LED control
* EEPROM and RAM configuration
* Packet handling and checksum validation
* REG_WRITE and ACTION instruction support

Designed for STM32 projects using the HAL driver.

---

# Features

* Supports Dynamixel Protocol 1.0
* STM32 HAL compatible
* UART half-duplex communication
* Simple high-level API
* Read/write 8-bit and 16-bit registers
* Error packet validation
* Support for:

  * AX series
  * MX series (Protocol 1.0 mode)
  * Other compatible Dynamixel devices

---

# File Structure

```text
.
├── dynamixel.c
├── dynamixel.h
└── README.md
```

---

# Requirements

* STM32 HAL Library
* UART configured in Half-Duplex mode
* Direction control pin (TX/RX enable)

---

# Hardware Connection

Typical half-duplex UART connection:

```text
STM32 TX/RX  <---->  Dynamixel DATA
STM32 GND    <---->  Dynamixel GND
STM32 VCC    <---->  Dynamixel VCC
```

Direction pin is used to switch between transmit and receive mode.

---

# Initialization

## Include Header

```c
#include "dynamixel.h"
```

## Initialize Library

```c
extern UART_HandleTypeDef huart1;

// Example direction pin
#define DXL_DIR_GPIO_Port GPIOA
#define DXL_DIR_Pin       GPIO_PIN_8

int main(void)
{
    HAL_Init();

    dxl_init(&huart1, DXL_DIR_GPIO_Port, DXL_DIR_Pin);

    while(1)
    {

    }
}
```

---

# Basic Usage

## Ping Servo

```c
uint8_t err;

err = dxl_ping(1);

if(err == 0)
{
    // Servo detected
}
```

---

## Enable Torque

```c
// Enable torque
 dxl_set_torque_enable(1, 1);

// Disable torque
 dxl_set_torque_enable(1, 0);
```

---

## Control LED

```c
// LED ON
 dxl_set_led(1, 1);

// LED OFF
 dxl_set_led(1, 0);
```

---

## Set Goal Position

```c
// Position range: 0 - 1023
 dxl_set_goal_position(1, 512);
```

---

## Set Moving Speed

```c
// Speed range: 0 - 1023
 dxl_set_moving_speed(1, 200);
```

---

## Read Present Position

```c
uint16_t position;

if(dxl_get_present_position(1, &position) == 0)
{
    // Position successfully read
}
```

---

# REG_WRITE Example

Queue multiple commands before executing them simultaneously.

```c
// Queue movement
 dxl_reg_goal_position(1, 300);
 dxl_reg_goal_position(2, 700);

// Execute together
 dxl_action();
```

---

# Error Handling

Functions return a Dynamixel error code.

```c
uint8_t err = dxl_ping(1);

if(err == 0)
{
    // Success
}
```

## Error Codes

| Error                 | Description              |
| --------------------- | ------------------------ |
| DXL_ERR_INPUT_VOLTAGE | Input voltage error      |
| DXL_ERR_ANGLE_LIMIT   | Angle limit error        |
| DXL_ERR_OVERHEATING   | Overheating error        |
| DXL_ERR_RANGE         | Range error              |
| DXL_ERR_CHECKSUM      | Checksum error           |
| DXL_ERR_OVERLOAD      | Overload error           |
| DXL_ERR_INSTRUCTION   | Invalid instruction      |
| DXL_ERR_COMMUNICATION | UART communication error |
| DXL_ERR_NULL          | Null pointer error       |

---

# Supported Instructions

| Instruction       | Value |
| ----------------- | ----- |
| AX_PING           | 0x01  |
| AX_READ_DATA      | 0x02  |
| AX_WRITE_DATA     | 0x03  |
| AX_REG_WRITE      | 0x04  |
| AX_ACTION         | 0x05  |
| AX_RESET          | 0x06  |
| AX_REBOOT         | 0x08  |
| AX_SYNC_WRITE     | 0x83  |
| AX_SYNC_REG_WRITE | 0x84  |
| AX_BULK_READ      | 0x92  |

---

# Common Register Addresses

## EEPROM Area

| Register             | Address |
| -------------------- | ------- |
| AX_ID                | 0x03    |
| AX_BAUD_RATE         | 0x04    |
| AX_RETURN_DELAY      | 0x05    |
| AX_CW_ANGLE_LIMIT_L  | 0x06    |
| AX_CCW_ANGLE_LIMIT_L | 0x08    |
| AX_MAX_TORQUE_L      | 0x0E    |

## RAM Area

| Register               | Address |
| ---------------------- | ------- |
| AX_TORQUE_ENABLE       | 0x18    |
| AX_LED                 | 0x19    |
| AX_GOAL_POSITION_L     | 0x1E    |
| AX_MOVING_SPEED_L      | 0x20    |
| AX_PRESENT_POSITION_L  | 0x24    |
| AX_PRESENT_SPEED_L     | 0x26    |
| AX_PRESENT_LOAD_L      | 0x28    |
| AX_PRESENT_VOLTAGE     | 0x2A    |
| AX_PRESENT_TEMPERATURE | 0x2B    |
| AX_MOVING              | 0x2E    |

---

# Available API

## Initialization

```c
void dxl_init(UART_HandleTypeDef *huart_handle,
              GPIO_TypeDef *GPIOx,
              uint16_t GPIO_Pin);
```

---

## Basic Commands

```c
uint8_t dxl_ping(uint8_t id);
void dxl_action(void);
```

---

## Motion Control

```c
uint8_t dxl_set_goal_position(uint8_t id, uint16_t position);
uint8_t dxl_get_goal_position(uint8_t id, uint16_t *data);

uint8_t dxl_set_moving_speed(uint8_t id, uint16_t speed);
uint8_t dxl_get_moving_speed(uint8_t id, uint16_t *data);

uint8_t dxl_set_goal_position_and_speed(uint8_t id,
                                        uint16_t position,
                                        uint16_t speed);
```

---

## Torque & LED

```c
uint8_t dxl_set_torque_enable(uint8_t id, uint8_t state);
uint8_t dxl_set_led(uint8_t id, uint8_t state);
```

---

## EEPROM Settings

```c
uint8_t dxl_set_id(uint8_t id, uint8_t new_id);
uint8_t dxl_set_baudrate(uint8_t id, uint8_t value);
uint8_t dxl_set_return_delay_time(uint8_t id, uint8_t delay);
```

---

# UART Configuration Notes

Recommended UART settings:

| Setting   | Value       |
| --------- | ----------- |
| Baudrate  | 1000000     |
| Data Bits | 8           |
| Stop Bits | 1           |
| Parity    | None        |
| Mode      | Half Duplex |

---

# Notes

* Dynamixel Protocol 1.0 uses little-endian format for 16-bit data.
* Make sure servo voltage matches your Dynamixel model.
* Some EEPROM registers require torque to be disabled before writing.
* Broadcast ID: `0xFE`

---

# Version

Current version:

```text
v1.0.3
```

---

# Author

Bintang Cahya

---

# License

This project is open-source and free to use for educational and personal projects.
