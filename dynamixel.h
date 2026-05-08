/*
 * dynamixel.h
 *
 * Author  : Bintang Cahya
 * Version : 1.0.3
 */

#ifndef DYNAMIXEL_H_
#define DYNAMIXEL_H_

#include "main.h"
#include <string.h>

/*
 * Dynamixel Protocol 1.0 Packet Structure:
 *
 * [0] 0xFF
 * [1] 0xFF
 * [2] ID
 * [3] LENGTH
 * [4] INSTRUCTION
 * [5:N] PARAMETERS
 * [N+1] CHECKSUM
 */


#define DXL_ERR_INPUT_VOLTAGE   (1 << 0)
#define DXL_ERR_ANGLE_LIMIT     (1 << 1)
#define DXL_ERR_OVERHEATING     (1 << 2)
#define DXL_ERR_RANGE           (1 << 3)
#define DXL_ERR_CHECKSUM        (1 << 4)
#define DXL_ERR_OVERLOAD        (1 << 5)
#define DXL_ERR_INSTRUCTION     (1 << 6)
#define DXL_ERR_COMMUNICATION   (1 << 7)

#define DXL_ERR_NULL               0xFE
#define DXL_BROADCAST_ID		   0xFE
#define DXL_HEADER				   0xFF
#define DXL_PACKET_LENGTH		   0x06

#define DXL_TRANSMIT_TIMEOUT	   10
#define DXL_RECEIVE_TIMEOUT		   10

// Instruction Set
#define DXL_PING					   0x01
#define DXL_READ_DATA        	   0x02
#define DXL_WRITE_DATA       	   0x03
#define DXL_REG_WRITE        	   0x04
#define DXL_ACTION           	   0x05
#define DXL_FACTORY_RESET           0x06
#define DXL_REBOOT				   0x08
#define DXL_SYNC_WRITE       	   0x83
#define DXL_SYNC_REG_WRITE   	   0x84
#define DXL_BULK_READ        	   0x92

// EEPROM Area Address
#define DXL_MODEL_NUMBER_L          0x00
#define DXL_MODEL_NUMBER_H          0x01
#define DXL_FIRMWARE_VERSION        0x02
#define DXL_ID                      0x03
#define DXL_BAUD_RATE               0x04
#define DXL_RETURN_DELAY            0x05
#define DXL_CW_ANGLE_LIMIT_L        0x06
#define DXL_CW_ANGLE_LIMIT_H        0x07
#define DXL_CCW_ANGLE_LIMIT_L       0x08
#define DXL_CCW_ANGLE_LIMIT_H       0x09
#define DXL_LIMIT_TEMPERATURE       0x0B
#define DXL_MIN_VOLTAGE_LIMIT       0x0C
#define DXL_MAX_VOLTAGE_LIMIT       0x0D
#define DXL_MAX_TORQUE_L            0x0E
#define DXL_MAX_TORQUE_H            0x0F
#define DXL_STATUS_RETURN_LEVEL     0x10
#define DXL_ALARM_LED               0x11
#define DXL_ALARM_SHUTDOWN          0x12

// RAM Area Address
#define DXL_TORQUE_ENABLE           0x18
#define DXL_LED                     0x19
#define DXL_CW_COMPLIANCE_MARGIN    0x1A
#define DXL_CCW_COMPLIANCE_MARGIN   0x1B
#define DXL_CW_COMPLIANCE_SLOPE     0x1C
#define DXL_CCW_COMPLIANCE_SLOPE    0x1D
#define DXL_GOAL_POSITION_L         0x1E
#define DXL_GOAL_POSITION_H         0x1F
#define DXL_MOVING_SPEED_L          0x20
#define DXL_MOVING_SPEED_H          0x21
#define DXL_TORQUE_LIMIT_L          0x22
#define DXL_TORQUE_LIMIT_H          0x23
#define DXL_PRESENT_POSITION_L      0x24
#define DXL_PRESENT_POSITION_H      0x25
#define DXL_PRESENT_SPEED_L         0x26
#define DXL_PRESENT_SPEED_H         0x27
#define DXL_PRESENT_LOAD_L          0x28
#define DXL_PRESENT_LOAD_H          0x29
#define DXL_PRESENT_VOLTAGE         0x2A
#define DXL_PRESENT_TEMPERATURE     0x2B
#define DXL_REGISTERED_INSTRUCTION  0x2C
#define DXL_PAUSE_TIME              0x2D
#define DXL_MOVING                  0x2E
#define DXL_LOCK                    0x2F
#define DXL_PUNCH_L                 0x30
#define DXL_PUNCH_H                 0x31


typedef struct
{
	UART_HandleTypeDef *huart;
    GPIO_TypeDef *gpiox;
    uint16_t gpio_pin;
} DXL_HandleTypeDef;


/*==============================================================================
 * Instruction Commands
 *============================================================================*/

uint8_t dxl_ping(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_factory_reset(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_reboot(DXL_HandleTypeDef *dxl, uint8_t id);
uint8_t dxl_sync_write(DXL_HandleTypeDef *dxl, uint8_t address, uint8_t *id, uint8_t *data, uint8_t id_count, uint8_t len);
void dxl_action(DXL_HandleTypeDef *dxl);


/*==============================================================================
 * EEPROM Area - Device Information
 *============================================================================*/

uint8_t dxl_get_model_number(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_get_firmware_version(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_id(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t new_id);


/*==============================================================================
 * EEPROM Area - Communication Settings
 *============================================================================*/

uint8_t dxl_get_baudrate(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_baudrate(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value);

uint8_t dxl_get_return_delay_time(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_return_delay_time(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t delay);

uint8_t dxl_get_status_return_level(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_status_return_level(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t level);


/*==============================================================================
 * EEPROM Area - Motion Limits
 *============================================================================*/

uint8_t dxl_get_cw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_cw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t value);

uint8_t dxl_get_ccw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_ccw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t value);

uint8_t dxl_get_max_torque(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_max_torque(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t max_torque);

uint8_t dxl_get_punch(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_punch(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t current);


/*==============================================================================
 * EEPROM Area - Safety Settings
 *============================================================================*/

uint8_t dxl_get_temperature_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_temperature_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t temp);

uint8_t dxl_get_min_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_min_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t voltage_value);

uint8_t dxl_get_max_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_max_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t voltage_value);

uint8_t dxl_get_alarm_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_alarm_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t error_code);

uint8_t dxl_get_shutdown(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_shutdown(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t error_code);


/*==============================================================================
 * RAM Area - Torque & LED Control
 *============================================================================*/

uint8_t dxl_get_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state);
uint8_t dxl_reg_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state);

uint8_t dxl_get_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state);
uint8_t dxl_reg_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state);

uint8_t dxl_get_lock(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_lock(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state);


/*==============================================================================
 * RAM Area - Compliance Settings
 *============================================================================*/

uint8_t dxl_get_cw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_cw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t margin);
uint8_t dxl_get_ccw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_ccw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t margin);

uint8_t dxl_get_cw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_cw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t slope);
uint8_t dxl_get_ccw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_set_ccw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t slope);


/*==============================================================================
 * Motion Control
 *============================================================================*/

uint8_t dxl_get_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position);
uint8_t dxl_reg_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position);

uint8_t dxl_get_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t speed);
uint8_t dxl_reg_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t speed);

uint8_t dxl_set_goal_position_and_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position, uint16_t speed);
uint8_t dxl_reg_goal_position_and_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position, uint16_t speed);

uint8_t dxl_get_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_set_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t torque);
uint8_t dxl_reg_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t torque);


/*==============================================================================
 * Status & Feedback
 *============================================================================*/

uint8_t dxl_get_present_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_get_present_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);
uint8_t dxl_get_present_load(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data);

uint8_t dxl_get_present_voltage(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_get_present_temperature(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);

uint8_t dxl_get_registered(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);
uint8_t dxl_get_moving(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data);


#endif /* DYNAMIXEL_H_ */
