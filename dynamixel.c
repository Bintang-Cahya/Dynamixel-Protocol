/*
 * dynamixel.c
 *
 * Author  : Bintang Cahya
 * Version : 1.0.3
 */

#include "dynamixel.h"

#define DYN_TX_MODE		HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_SET)
#define DYN_RX_MODE		HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_RESET)

UART_HandleTypeDef *huart;

GPIO_TypeDef *gpiox;
uint16_t gpio_pin;

static uint8_t dxl_checksum(const uint8_t *packet, uint8_t length)
{
	uint16_t sum = 0;

	for(uint8_t i = 2; i < (length - 1); i++)
	{
		sum += packet[i];
	}

	return (~sum) & DXL_HEADER;
}


static uint8_t dxl_validate_status_packet(const uint8_t *packet, uint8_t expected_id, uint8_t length)
{
	/* Header Check */
	if(packet[0] != DXL_HEADER || packet[1] != DXL_HEADER)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* ID Check */
	if(packet[2] != expected_id)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Checksum Check */
	if(dxl_checksum(packet, length) != packet[length - 1])
	{
		return DXL_ERR_CHECKSUM;
	}

	/* Return Dynamixel Error Byte */
	return packet[4];
}


static uint8_t dxl_receive_status_packet(uint8_t expected_id, uint8_t *packet, uint8_t length)
{
	/* Switch to RX Mode */
	DYN_RX_MODE;

	/* Receive Packet */
	if(HAL_UART_Receive(huart, packet, length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	return dxl_validate_status_packet(packet, expected_id, length);
}


static uint8_t dxl_build_packet(uint8_t id, uint8_t instruction, const uint8_t *params, uint8_t param_len, uint8_t *packet)
{
	packet[0] = DXL_HEADER;			// Header
	packet[1] = DXL_HEADER;
	packet[2] = id;				// Dynamixel ID
	packet[3] = param_len + 2;	// Parameter length + instruction and checksum
	packet[4] = instruction;	// Instruction

	if(params != NULL)
	{
		for(uint8_t i = 0; i < param_len; i++)
		{
			packet[5 + i] = params[i];
		}
	}

	uint8_t total_length = param_len + 6;

	packet[total_length - 1] = dxl_checksum(packet, total_length);

	return total_length;
}

uint8_t dxl_read_u8(uint8_t id, uint8_t address, uint8_t *data)
{
	uint8_t params[2];
	uint8_t tx_packet[8];
	uint8_t rx_packet[7];

	/* Null Pointer Check */
	if(data == NULL)
	{
		return DXL_ERR_NULL;
	}

	/* Parameters */
	params[0] = address;
	params[1] = 0x01;

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, AX_READ_DATA, params, sizeof(params), tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	if(HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	uint8_t error = dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));

	if(error != 0)
	{
		return error;
	}

	/* Extract Data */
	*data = rx_packet[5];

	return 0;
}


uint8_t dxl_read_u16(uint8_t id, uint8_t address, uint16_t *data)
{
	uint8_t params[2];
	uint8_t tx_packet[8];
	uint8_t rx_packet[8];

	/* Null Pointer Check */
	if(data == NULL)
	{
		return DXL_ERR_NULL;
	}

	/* Parameters */
	params[0] = address;
	params[1] = 0x02;

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, AX_READ_DATA, params, sizeof(params), tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	if(HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	uint8_t error = dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));

	if(error != 0)
	{
		return error;
	}

	/* Extract Data */
	*data = rx_packet[5] | (rx_packet[6] << 8);

	return 0;
}


uint8_t dxl_write_u8(uint8_t id, uint8_t address, uint8_t value)
{
	uint8_t params[2];
	uint8_t tx_packet[8];
	uint8_t rx_packet[6];

	/* Parameters */
	params[0] = address;
	params[1] = value;

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, AX_WRITE_DATA, params, sizeof(params), tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	if(HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));
}


uint8_t dxl_write_u16(uint8_t id, uint8_t address, uint16_t value)
{
	uint8_t params[3];
	uint8_t tx_packet[9];
	uint8_t rx_packet[6];

	/* Parameters */
	params[0] = address;
	params[1] = value & 0xFF;
	params[2] = value >> 8;

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, AX_WRITE_DATA, params, sizeof(params), tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	if(HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));
}


/*================================================================================================
 * Initialization
 *==============================================================================================*/

/*
 * @brief   Initialize the Dynamixel communication interface
 *
 * @param   huart       Pointer to the UART handle for communication
 * @param   GPIOx       GPIO port used for controlling Dynamixel direction pin
 * @param   GPIO_Pin    GPIO pin used for controlling Dynamixel direction
 */
void dxl_init(UART_HandleTypeDef *huart_handle, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	huart = huart;
	gpiox = GPIOx;
	gpio_pin = GPIO_Pin;
}


/*================================================================================================
 * Basic Commands
 *==============================================================================================*/

/*
 * @brief   Ping a Dynamixel servo to check its availability
 *
 * @param   id      	ID of the Dynamixel to ping
 * @return  uint8_t 	Status of the ping operation:
 *                  	- 0: Success, Dynamixel responded to ping
 *                  	- Error code: Indicates failure during ping operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_ping(uint8_t id)
{
	uint8_t tx_packet[6];
	uint8_t rx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, AX_PING, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	if(HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
	{
		return DXL_ERR_COMMUNICATION;
	}

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Execute command that was registered beforehand
 */
void dxl_action(void)
{
	uint8_t tx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(DXL_BROADCAST_ID, AX_ACTION, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	DYN_TX_MODE;

	/* Transmit Packet */
	HAL_UART_Transmit(huart, tx_packet, packet_length, 10);

	while(__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);
}


/*================================================================================================
 * EEPROM Area - Device Information
 *==============================================================================================*/

uint8_t dxl_get_model_number(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MODEL_NUMBER_L, data);
}

uint8_t dxl_get_firmware_version(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_FIRMWARE_VERSION, data);
}


/*
 * @brief   Change Dynamixel ID
 *
 * @param   id      	Current ID of the Dynamixel servo
 * @param   new_id   	New ID to be set for the Dynamixel servo (0 - 253)
 * @return  uint8_t 	Status of the set ID operation:
 *                  	- 0: Success, ID has been changed
 *                  	- Error code: Indicates failure during set ID operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_id(uint8_t id, uint8_t new_id)
{
	return dxl_write_u8(id, AX_ID, new_id);
}


/*================================================================================================
 * EEPROM Area - Communication Settings
 *==============================================================================================*/

uint8_t dxl_get_baudrate(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_BAUD_RATE, data);
}


uint8_t dxl_set_baudrate(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_BAUD_RATE, value);
}


uint8_t dxl_get_return_delay_time(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_RETURN_DELAY, data);
}


uint8_t dxl_set_return_delay_time(uint8_t id, uint8_t delay)
{
	return dxl_write_u8(id, AX_RETURN_DELAY, delay);
}


uint8_t dxl_get_status_return_level(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_STATUS_RETURN_LEVEL, data);
}


uint8_t dxl_set_status_return_level(uint8_t id, uint8_t level)
{
	return dxl_write_u8(id, AX_STATUS_RETURN_LEVEL, level);
}


/*================================================================================================
 * EEPROM Area - Motion Limits
 *==============================================================================================*/

uint8_t dxl_get_cw_angle_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_CW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set the clockwise angle limit of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel
 * @param   value   	Target value for the clockwise angle limit (0-1023)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, clockwise angle limit set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_cw_angle_limit(uint8_t id, uint16_t value)
{
	return dxl_write_u16(id, AX_CW_ANGLE_LIMIT_L, value);
}


uint8_t dxl_get_ccw_angle_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_CCW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set the counterclockwise angle limit of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel
 * @param   value   	Target value for the counterclockwise angle limit (0-1023)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, counterclockwise angle limit set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_ccw_angle_limit(uint8_t id, uint16_t value)
{
	return dxl_write_u16(id, AX_CCW_ANGLE_LIMIT_L, value);
}

uint8_t dxl_get_max_torque(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MAX_TORQUE_L, data);
}

uint8_t dxl_set_max_torque(uint8_t id, uint16_t max_torque)
{
	return dxl_write_u16(id, AX_MAX_TORQUE_L, max_torque);
}

uint8_t dxl_get_punch(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PUNCH_L, data);
}

uint8_t dxl_set_punch(uint8_t id, uint16_t current)
{
	return dxl_write_u16(id, AX_PUNCH_L, current);
}

/*================================================================================================
 * EEPROM Area - Safety Settings
 *==============================================================================================*/

uint8_t dxl_get_temperature_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LIMIT_TEMPERATURE, data);
}

uint8_t dxl_set_temperature_limit(uint8_t id, uint8_t temp)
{
	return dxl_write_u8(id, AX_LIMIT_TEMPERATURE, temp);
}

uint8_t dxl_get_min_voltage_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_MIN_VOLTAGE_LIMIT, data);
}

uint8_t dxl_set_min_voltage_limit(uint8_t id, uint8_t voltage_value)
{
	return dxl_write_u8(id, AX_MIN_VOLTAGE_LIMIT, voltage_value);
}

uint8_t dxl_get_max_voltage_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_MAX_VOLTAGE_LIMIT, data);
}
uint8_t dxl_set_max_voltage_limit(uint8_t id, uint8_t voltage_value)
{
	return dxl_write_u8(id, AX_MAX_VOLTAGE_LIMIT, voltage_value);
}

uint8_t dxl_get_alarm_led(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_ALARM_LED, data);
}
uint8_t dxl_set_alarm_led(uint8_t id, uint8_t error_code)
{
	return dxl_write_u8(id, AX_ALARM_LED, error_code);
}

uint8_t dxl_get_shutdown(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_ALARM_SHUTDOWN, data);
}
uint8_t dxl_set_shutdown(uint8_t id, uint8_t error_code)
{
	return dxl_write_u8(id, AX_ALARM_SHUTDOWN, error_code);
}

/*================================================================================================
 * RAM Area - Torque & LED Control
 *==============================================================================================*/

uint8_t dxl_get_torque_enable(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_TORQUE_ENABLE, data);
}
uint8_t dxl_set_torque_enable(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_TORQUE_ENABLE, state);
}

uint8_t dxl_get_led(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LED, data);
}
uint8_t dxl_set_led(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_LED, state);
}

uint8_t dxl_get_lock(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LOCK, data);
}
uint8_t dxl_set_lock(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_LOCK, state);
}

/*================================================================================================
 * RAM Area - Compliance Settings
 *==============================================================================================*/

uint8_t dxl_get_cw_compliance_margin(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_CW_COMPLIANCE_MARGIN, data);
}
uint8_t dxl_set_cw_compliance_margin(uint8_t id, uint8_t margin)
{
	return dxl_write_u8(id, AX_CW_COMPLIANCE_MARGIN, margin);
}

uint8_t dxl_get_ccw_compliance_slope(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_CCW_COMPLIANCE_SLOPE, data);
}
uint8_t dxl_set_ccw_compliance_slope(uint8_t id, uint8_t steps)
{
	return dxl_write_u8(id, AX_CCW_COMPLIANCE_SLOPE, steps);
}

/*================================================================================================
 * Motion Control
 *==============================================================================================*/

//uint8_t dxl_move(uint8_t id, uint16_t position);
//uint8_t dxl_move_speed(uint8_t id, uint16_t position, uint16_t speed);

uint8_t dxl_get_goal_position(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_GOAL_POSITION_L, data);
}


/*
 * @brief   Set the goal position of a Dynamixel to be executed upon receiving the action command
 *
 * @param   id      	ID of the Dynamixel
 * @param   value   	Target position value (0-1023)
 * @return  uint8_t 	Status of the set goal operation:
 *                  	- 0: Success, goal position set successfully
 *                  	- Error code: Indicates failure during goal position setting.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_goal_position(uint8_t id, uint16_t position)
{
	return dxl_write_u16(id, AX_GOAL_POSITION_L, position);
}

uint8_t dxl_get_moving_speed(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MOVING_SPEED_L, data);
}
uint8_t dxl_set_moving_speed(uint8_t id, uint16_t speed)
{
	return dxl_write_u16(id, AX_MOVING_SPEED_L, speed);
}

uint8_t dxl_get_torque_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_TORQUE_LIMIT_L, data);
}

uint8_t dxl_set_torque_limit(uint8_t id, uint16_t torque)
{
	return dxl_write_u16(id, AX_TORQUE_LIMIT_L, torque);
}

/*================================================================================================
 * Status & Feedback
 *==============================================================================================*/

/*
 * @brief   Get the current position of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the position data will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, position data retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_present_position(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PRESENT_POSITION_L, data);
}


/*
 * @brief   Get the current speed of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the speed data will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, speed data retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                              	  See error code definitions in Header File
 */
uint8_t dxl_get_present_speed(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PRESENT_SPEED_L, data);
}


/*
 * @brief   Get the current load of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the load data will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, load data retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                              	  See error code definitions in Header File
 */
uint8_t dxl_get_present_load(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PRESENT_LOAD_L, data);
}

uint8_t dxl_get_present_voltage(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_PRESENT_VOLTAGE, data);
}

uint8_t dxl_get_present_temperature(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_PRESENT_TEMPERATURE, data);
}

uint8_t dxl_get_registered(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_REGISTERED_INSTRUCTION, data);
}


/*
 * @brief   Check if a Dynamixel servo is moving
 *
 * @param   id      	ID of the Dynamixel servo to query
 * @param   data   		Pointer to a variable where the moving status will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, moving status retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_moving(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_MOVING, data);
}
