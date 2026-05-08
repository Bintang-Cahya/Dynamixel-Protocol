# Dynamixel Protocol 1.0 STM32 Library

Lightweight STM32 HAL library for controlling Dynamixel servos using **Dynamixel Protocol 1.0** over UART.

This library provides simple APIs for:

* Reading and writing Dynamixel registers
* Motion control
* Torque and LED control
* Dynamixel configuration
* instruction command support

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

DXL_HandleTypeDef dxl = {&huart1, DXL_DIR_GPIO_Port, DXL_DIR_Pin};

int main(void)
{
    HAL_Init();
    while(1)
    {

    }
}
```

---

# Example Usage

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

## REG_WRITE Example

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

| Instruction       | Support |
| ----------------- | ------- |
| AX_PING           | ☑      |
| AX_READ_DATA      | ☑      |
| AX_WRITE_DATA     | ☑      |
| AX_REG_WRITE      | ☑      |
| AX_ACTION         | ☑      |
| AX_RESET          | ☑      |
| AX_REBOOT         | ☑      |
| AX_SYNC_WRITE     | ☑      |
| AX_BULK_READ      | ☐      |

---

# Available API

## Instruction Commands

```c
uint8_t dxl_ping(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_factory_reset(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_reboot(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_sync_write(DXL_HandleTypeDef *dxl, uint8_t address, uint8_t *id, uint8_t *data, uint8_t id_count, uint8_t len);
void dxl_action(DXL_HandleTypeDef *dxl);
```

---

## Device Information

```c
uint8_t dxl_get_model_number(uint8_t id, uint16_t *data);
uint8_t dxl_get_firmware_version(uint8_t id, uint8_t *data);
uint8_t dxl_set_id(uint8_t id, uint8_t new_id);
```

---

## Communication Settings

```c
uint8_t dxl_get_baudrate(uint8_t id, uint8_t *data);
uint8_t dxl_set_baudrate(uint8_t id, uint8_t value);

uint8_t dxl_get_return_delay_time(uint8_t id, uint8_t *data);
uint8_t dxl_set_return_delay_time(uint8_t id, uint8_t delay);

uint8_t dxl_get_status_return_level(uint8_t id, uint8_t *data);
uint8_t dxl_set_status_return_level(uint8_t id, uint8_t level);
```

---

## Motion Limit

```c
uint8_t dxl_get_cw_angle_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_cw_angle_limit(uint8_t id, uint16_t value);

uint8_t dxl_get_ccw_angle_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_ccw_angle_limit(uint8_t id, uint16_t value);

uint8_t dxl_get_max_torque(uint8_t id, uint16_t *data);
uint8_t dxl_set_max_torque(uint8_t id, uint16_t max_torque);

uint8_t dxl_get_punch(uint8_t id, uint16_t *data);
uint8_t dxl_set_punch(uint8_t id, uint16_t current);
```

---

## Safety Settings

```c
uint8_t dxl_get_temperature_limit(uint8_t id, uint8_t *data);
uint8_t dxl_set_temperature_limit(uint8_t id, uint8_t temp);

uint8_t dxl_get_min_voltage_limit(uint8_t id, uint8_t *data);
uint8_t dxl_set_min_voltage_limit(uint8_t id, uint8_t voltage_value);

uint8_t dxl_get_max_voltage_limit(uint8_t id, uint8_t *data);
uint8_t dxl_set_max_voltage_limit(uint8_t id, uint8_t voltage_value);

uint8_t dxl_get_alarm_led(uint8_t id, uint8_t *data);
uint8_t dxl_set_alarm_led(uint8_t id, uint8_t error_code);

uint8_t dxl_get_shutdown(uint8_t id, uint8_t *data);
uint8_t dxl_set_shutdown(uint8_t id, uint8_t error_code);
```

---

## Torque and LED control

```c
uint8_t dxl_get_torque_enable(uint8_t id, uint8_t *data);
uint8_t dxl_set_torque_enable(uint8_t id, uint8_t state);
uint8_t dxl_reg_torque_enable(uint8_t id, uint8_t state);

uint8_t dxl_get_led(uint8_t id, uint8_t *data);
uint8_t dxl_set_led(uint8_t id, uint8_t state);
uint8_t dxl_reg_led(uint8_t id, uint8_t state);

uint8_t dxl_get_lock(uint8_t id, uint8_t *data);
uint8_t dxl_set_lock(uint8_t id, uint8_t state);
```

---

## Compliance Settings

```c
uint8_t dxl_get_cw_compliance_margin(uint8_t id, uint8_t *data);
uint8_t dxl_set_cw_compliance_margin(uint8_t id, uint8_t margin);

uint8_t dxl_get_ccw_compliance_slope(uint8_t id, uint8_t *data);
uint8_t dxl_set_ccw_compliance_slope(uint8_t id, uint8_t steps);
```

---

## Motion Control

```c
uint8_t dxl_get_goal_position(uint8_t id, uint16_t *data);
uint8_t dxl_set_goal_position(uint8_t id, uint16_t position);
uint8_t dxl_reg_goal_position(uint8_t id, uint16_t position);

uint8_t dxl_get_moving_speed(uint8_t id, uint16_t *data);
uint8_t dxl_set_moving_speed(uint8_t id, uint16_t speed);
uint8_t dxl_reg_moving_speed(uint8_t id, uint16_t speed);

uint8_t dxl_set_goal_position_and_speed(uint8_t id, uint16_t position, uint16_t speed);
uint8_t dxl_reg_goal_position_and_speed(uint8_t id, uint16_t position, uint16_t speed);

uint8_t dxl_get_torque_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_torque_limit(uint8_t id, uint16_t torque);
uint8_t dxl_reg_torque_limit(uint8_t id, uint16_t torque);
```

---

## Status and Feedback

```c
uint8_t dxl_get_present_position(uint8_t id, uint16_t *data);
uint8_t dxl_get_present_speed(uint8_t id, uint16_t *data);
uint8_t dxl_get_present_load(uint8_t id, uint16_t *data);

uint8_t dxl_get_present_voltage(uint8_t id, uint8_t *data);
uint8_t dxl_get_present_temperature(uint8_t id, uint8_t *data);

uint8_t dxl_get_registered(uint8_t id, uint8_t *data);
uint8_t dxl_get_moving(uint8_t id, uint8_t *data);
```

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
