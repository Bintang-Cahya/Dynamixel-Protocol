/*
 * dynamixel.h
 *
 * Author  : Bintang Cahya
 * Version : 1.0.3
 */

#ifndef DYNAMIXEL_H_
#define DYNAMIXEL_H_

#include "main.h"

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

// Instruction Set
#define AX_PING					   0x01
#define AX_READ_DATA        	   0x02
#define AX_WRITE_DATA       	   0x03
#define AX_REG_WRITE        	   0x04
#define AX_ACTION           	   0x05
#define AX_RESET            	   0x06
#define AX_REBOOT				   0x08
#define AX_SYNC_WRITE       	   0x83
#define AX_SYNC_REG_WRITE   	   0x84
#define AX_BULK_READ        	   0x92

// EEPROM Area Address
#define AX_MODEL_NUMBER_L          0x00
#define AX_MODEL_NUMBER_H          0x01
#define AX_FIRMWARE_VERSION        0x02
#define AX_ID                      0x03
#define AX_BAUD_RATE               0x04
#define AX_RETURN_DELAY            0x05
#define AX_CW_ANGLE_LIMIT_L        0x06
#define AX_CW_ANGLE_LIMIT_H        0x07
#define AX_CCW_ANGLE_LIMIT_L       0x08
#define AX_CCW_ANGLE_LIMIT_H       0x09
#define AX_LIMIT_TEMPERATURE       0x0B
#define AX_MIN_VOLTAGE_LIMIT       0x0C
#define AX_MAX_VOLTAGE_LIMIT       0x0D
#define AX_MAX_TORQUE_L            0x0E
#define AX_MAX_TORQUE_H            0x0F
#define AX_STATUS_RETURN_LEVEL     0x10
#define AX_ALARM_LED               0x11
#define AX_ALARM_SHUTDOWN          0x12

// RAM Area Address
#define AX_TORQUE_ENABLE           0x18
#define AX_LED                     0x19
#define AX_CW_COMPLIANCE_MARGIN    0x1A
#define AX_CCW_COMPLIANCE_MARGIN   0x1B
#define AX_CW_COMPLIANCE_SLOPE     0x1C
#define AX_CCW_COMPLIANCE_SLOPE    0x1D
#define AX_GOAL_POSITION_L         0x1E
#define AX_GOAL_POSITION_H         0x1F
#define AX_MOVING_SPEED_L          0x20
#define AX_MOVING_SPEED_H          0x21
#define AX_TORQUE_LIMIT_L          0x22
#define AX_TORQUE_LIMIT_H          0x23
#define AX_PRESENT_POSITION_L      0x24
#define AX_PRESENT_POSITION_H      0x25
#define AX_PRESENT_SPEED_L         0x26
#define AX_PRESENT_SPEED_H         0x27
#define AX_PRESENT_LOAD_L          0x28
#define AX_PRESENT_LOAD_H          0x29
#define AX_PRESENT_VOLTAGE         0x2A
#define AX_PRESENT_TEMPERATURE     0x2B
#define AX_REGISTERED_INSTRUCTION  0x2C
#define AX_PAUSE_TIME              0x2D
#define AX_MOVING                  0x2E
#define AX_LOCK                    0x2F
#define AX_PUNCH_L                 0x30
#define AX_PUNCH_H                 0x31

/*==============================================================================
 * Initialization
 *============================================================================*/

void dxl_init(UART_HandleTypeDef *huart_handle, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);


/*==============================================================================
 * Basic Commands
 *============================================================================*/

uint8_t dxl_ping(uint8_t id);
void dxl_action(void);


/*==============================================================================
 * EEPROM Area - Device Information
 *============================================================================*/

uint8_t dxl_get_model_number(uint8_t id, uint16_t *data);
uint8_t dxl_get_firmware_version(uint8_t id, uint8_t *data);
uint8_t dxl_set_id(uint8_t id, uint8_t new_id);


/*==============================================================================
 * EEPROM Area - Communication Settings
 *============================================================================*/

uint8_t dxl_get_baudrate(uint8_t id, uint8_t *data);
uint8_t dxl_set_baudrate(uint8_t id, uint8_t value);

uint8_t dxl_get_return_delay_time(uint8_t id, uint8_t *data);
uint8_t dxl_set_return_delay_time(uint8_t id, uint8_t delay);

uint8_t dxl_get_status_return_level(uint8_t id, uint8_t *data);
uint8_t dxl_set_status_return_level(uint8_t id, uint8_t level);


/*==============================================================================
 * EEPROM Area - Motion Limits
 *============================================================================*/

uint8_t dxl_get_cw_angle_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_cw_angle_limit(uint8_t id, uint16_t value);

uint8_t dxl_get_ccw_angle_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_ccw_angle_limit(uint8_t id, uint16_t value);

uint8_t dxl_get_max_torque(uint8_t id, uint16_t *data);
uint8_t dxl_set_max_torque(uint8_t id, uint16_t max_torque);

uint8_t dxl_get_punch(uint8_t id, uint16_t *data);
uint8_t dxl_set_punch(uint8_t id, uint16_t current);


/*==============================================================================
 * EEPROM Area - Safety Settings
 *============================================================================*/

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


/*==============================================================================
 * RAM Area - Torque & LED Control
 *============================================================================*/

uint8_t dxl_get_torque_enable(uint8_t id, uint8_t *data);
uint8_t dxl_set_torque_enable(uint8_t id, uint8_t state);

uint8_t dxl_get_led(uint8_t id, uint8_t *data);
uint8_t dxl_set_led(uint8_t id, uint8_t state);

uint8_t dxl_get_lock(uint8_t id, uint8_t *data);
uint8_t dxl_set_lock(uint8_t id, uint8_t state);


/*==============================================================================
 * RAM Area - Compliance Settings
 *============================================================================*/

uint8_t dxl_get_cw_compliance_margin(uint8_t id, uint8_t *data);
uint8_t dxl_set_cw_compliance_margin(uint8_t id, uint8_t margin);

uint8_t dxl_get_ccw_compliance_slope(uint8_t id, uint8_t *data);
uint8_t dxl_set_ccw_compliance_slope(uint8_t id, uint8_t steps);


/*==============================================================================
 * Motion Control
 *============================================================================*/

//uint8_t dxl_move(uint8_t id, uint16_t position);
//uint8_t dxl_move_speed(uint8_t id, uint16_t position, uint16_t speed);

uint8_t dxl_get_goal_position(uint8_t id, uint16_t *data);
uint8_t dxl_set_goal_position(uint8_t id, uint16_t position);

uint8_t dxl_get_moving_speed(uint8_t id, uint16_t *data);
uint8_t dxl_set_moving_speed(uint8_t id, uint16_t speed);

uint8_t dxl_get_torque_limit(uint8_t id, uint16_t *data);
uint8_t dxl_set_torque_limit(uint8_t id, uint16_t torque);


/*==============================================================================
 * Status & Feedback
 *============================================================================*/

uint8_t dxl_get_present_position(uint8_t id, uint16_t *data);
uint8_t dxl_get_present_speed(uint8_t id, uint16_t *data);
uint8_t dxl_get_present_load(uint8_t id, uint16_t *data);

uint8_t dxl_get_present_voltage(uint8_t id, uint8_t *data);
uint8_t dxl_get_present_temperature(uint8_t id, uint8_t *data);

uint8_t dxl_get_registered(uint8_t id, uint8_t *data);
uint8_t dxl_get_moving(uint8_t id, uint8_t *data);


#endif /* DYNAMIXEL_H_ */
