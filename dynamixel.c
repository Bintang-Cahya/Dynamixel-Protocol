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


/*================================================================================================
 * Packet utility
 *==============================================================================================*/


/*
 * @brief   Calculate packet checksum
 *
 * @param   packet      Pointer to packet array
 * @param   length    	Packet array length
 * @return  uint8_t		Checksum calculation result
 */
static uint8_t dxl_checksum(const uint8_t *packet, uint8_t length)
{
	uint16_t sum = 0;

	for(uint8_t i = 2; i < (length - 1); i++)
	{
		sum += packet[i];
	}

	return (~sum) & 0xFF;
}


/*
 * @brief   Validate status packet
 *
 * @param   packet      Pointer to packet array
 * @param   expected_id	Responder expected ID
 * @param   length    	Packet array length
 * @return  uint8_t		Dynamixel error code
 */
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


/*
 * @brief   Receive status packet from dynamixel
 *
 * @param   expected_id	Responder expected ID
 * @param   packet      Pointer to packet array
 * @param   length    	Packet array length
 * @return  uint8_t		Dynamixel error code
 */
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


/*
 * @brief   Building packet structure before send to dynamixel
 *
 * @param   id			Dynamixel target ID
 * @param   instruction	Instruction type (READ, WRITE, REG_WRITE, etc.)
 * @param   params      Packet parameter
 * @param   param_len   Packet parameter length
 * @return  uint8_t		Packet total length
 */
static uint8_t dxl_build_packet(uint8_t id, uint8_t instruction, const uint8_t *params, uint8_t param_len, uint8_t *packet)
{
	packet[0] = DXL_HEADER;		// Header
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


/*================================================================================================
 * Dynamixel read and write
 *==============================================================================================*/


/*
 * @brief   Read data from an address
 *
 * @param   id			Dynamixel target ID
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   data		Pointer to variabel that hold received data
 * @return  uint8_t		Dynamixel error code
 */
static uint8_t dxl_read_len(uint8_t id, uint8_t address, uint8_t *out, uint8_t len)
{
    uint8_t params[2];
    uint8_t tx_packet[8];
    uint8_t rx_packet[10];

    if (out == NULL)
        return DXL_ERR_NULL;

    /* Parameters: address + length */
    params[0] = address;
    params[1] = len;

    uint8_t packet_length = dxl_build_packet(id, AX_READ_DATA, params, sizeof(params), tx_packet);

    DYN_TX_MODE;

    if (HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
        return DXL_ERR_COMMUNICATION;

    while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

    uint8_t err = dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));
    if (err) return err;

    /* Copy N bytes from status packet */
    memcpy(out, &rx_packet[5], len);

    return 0;
}


/*
 * @brief   Read 1 byte data from an address
 *
 * @param   id			Dynamixel target ID
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   data		Pointer to variabel that hold received data
 * @return  uint8_t		Dynamixel error code
 */
uint8_t dxl_read_u8(uint8_t id, uint8_t address, uint8_t *data)
{
	return dxl_read_len(id, address, data, 1);
}


/*
 * @brief   Read 2 byte data from an address
 *
 * @param   id			Dynamixel target ID
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   data		Pointer to variabel that hold received data
 * @return  uint8_t		Dynamixel error code
 */
uint8_t dxl_read_u16(uint8_t id, uint8_t address, uint16_t *data)
{
    uint8_t buf[2];
    uint8_t err = dxl_read_len(id, address, buf, 2);
    if (err) return err;

    *data = buf[0] | (buf[1] << 8);
    return 0;
}


/*
 * @brief   Write data to an address
 *
 * @param   id			Dynamixel target ID
 * @param	instruction	Instruction type
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   value		value to be set
 * @return  uint8_t		Dynamixel error code
 */
static uint8_t dxl_write_len(uint8_t id, uint8_t instruction, uint8_t address, const uint8_t *data, uint8_t len)
{
    uint8_t tx_packet[10];
    uint8_t rx_packet[6];
    uint8_t params[1 + 4];   // address + up to 4 bytes

    params[0] = address;
    memcpy(&params[1], data, len);

    uint8_t packet_length = dxl_build_packet(id, instruction, params, 1 + len, tx_packet);

    DYN_TX_MODE;

    if (HAL_UART_Transmit(huart, tx_packet, packet_length, 10) != HAL_OK)
        return DXL_ERR_COMMUNICATION;

    while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET);

    return dxl_receive_status_packet(id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Write 1 byte data to an address
 *
 * @param   id			Dynamixel target ID
 * @param	instruction	Instruction type
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   value		value to be set
 * @return  uint8_t		Dynamixel error code
 */
uint8_t dxl_write_u8(uint8_t id, uint8_t instruction, uint8_t address, uint8_t value)
{
	return dxl_write_len(id, instruction, address, &value, 1);
}


/*
 * @brief   Write 2 byte data to an address
 *
 * @param   id			Dynamixel target ID
 * @param	instruction	Instruction type
 * @param   address		Register address ex. AX_TORQUE_ENABLE, AX_LED
 * @param   value		value to be set
 * @return  uint8_t		Dynamixel error code
 */
uint8_t dxl_write_u16(uint8_t id, uint8_t instruction, uint8_t address, uint16_t value)
{
    uint8_t b[2] = { value & 0xFF, value >> 8 };
    return dxl_write_len(id, instruction, address, b, 2);
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
 * @brief   Ping a dynamixel to check its availability
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
 * @brief   Execute all registered command or value. note that this command send to
 * 			all connected dynamixel by using BROADCAST_ID
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

/*
 * @brief   Get dynamixel model number
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold model number data
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during get model number operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_model_number(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MODEL_NUMBER_L, data);
}


/*
 * @brief   Get dynamixel firmware version
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold firmware version data
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during get firmware operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_firmware_version(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_FIRMWARE_VERSION, data);
}


/*
 * @brief   Change dynamixel ID
 *
 * @param   id      	Current ID of the dynamixel
 * @param   new_id   	New ID to be set for the dynamixel (0 - 253)
 * @return  uint8_t 	Status of the set ID operation:
 *                  	- 0: Success, ID has been changed
 *                  	- Error code: Indicates failure during set ID operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_id(uint8_t id, uint8_t new_id)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_ID, new_id);
}


/*================================================================================================
 * EEPROM Area - Communication Settings
 *==============================================================================================*/

/*
 * @brief   Get dynamixel baudrate
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold baudrate
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during get operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_baudrate(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_BAUD_RATE, data);
}


/*
 * @brief   Set dynamixel baudrate
 *
 * @param   id      	Dynamixel target ID
 * @param	value		Dynamixel baudrate value:
 * 						1 (default)		= 1.000.000 baudrate
 * 						3				= 500.000 baudrate
 * 						4				= 400.000 baudrate
 * 						7				= 250.000 baudrate
 * 						9				= 200.000 baudrate
 * 						16				= 115.200 baudrate
 * 						34				= 57.600 baudrate
 * 						103				= 19.200 baudrate
 * 						207				= 9.600 baudrate
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_set_baudrate(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_BAUD_RATE, value);
}


/*
 * @brief   Get dynamixel return delay time
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold delay time value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_return_delay_time(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_RETURN_DELAY, data);
}


/*
 * @brief   Set dynamixel return delay time
 *
 * @param   id      	Dynamixel target ID
 * @param	delay		Return delay time value (0-254) with 2uS each point
 * 						factory default value is 250 (500uS)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_set_return_delay_time(uint8_t id, uint8_t delay)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_RETURN_DELAY, delay);
}


/*
 * @brief   Get dynamixel status return level
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold status return level value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_status_return_level(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_STATUS_RETURN_LEVEL, data);
}


/*
 * @brief   Set dynamixel status return level that decides how to return Status Packet
 * 			when DYNAMIXEL receives an Instruction Packet
 *
 * @param   id      	Dynamixel target ID
 * @param	level		Desired value for status return level (0-2):
 * 						0				-> Returns the Status Packet for PING Instruction only
 * 						1				-> Returns the Status Packet for PING and READ Instruction
 * 						2 (default)		-> Returns the Status Packet for all Instructions
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_set_status_return_level(uint8_t id, uint8_t level)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_STATUS_RETURN_LEVEL, level);
}


/*================================================================================================
 * EEPROM Area - Motion Limits
 *==============================================================================================*/

/*
 * @brief   Get dynamixel CW angle limit
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold limit value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_cw_angle_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_CW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set dynamixel CW angle limit
 *
 * @param   id      	Dynamixel target ID
 * @param   value   	Desired value for clockwise angle limit (0-1023)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, CW angle limit set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_cw_angle_limit(uint8_t id, uint16_t value)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_CW_ANGLE_LIMIT_L, value);
}


/*
 * @brief   Get dynamixel CCW angle limit
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold limit value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_ccw_angle_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_CCW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set dynamixel CCW angle limit
 *
 * @param   id      	Dynamixel target ID
 * @param   value   	Desired value for CCW angle limit (0-1023)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, CCW angle limit set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_ccw_angle_limit(uint8_t id, uint16_t value)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_CCW_ANGLE_LIMIT_L, value);
}


/*
 * @brief   Get dynamixel max torque value
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold limit value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_max_torque(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MAX_TORQUE_L, data);
}


/*
 * @brief   Set dynamixel max torque
 *
 * @param   id      	Dynamixel target ID
 * @param   max_torque  Desired value for max torque (0-1023)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, max torque set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_max_torque(uint8_t id, uint16_t max_torque)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_MAX_TORQUE_L, max_torque);
}


/*
 * @brief   Get dynamixel punch value
 *
 * @param   id      	Dynamixel target ID
 * @param	data		Pointer to a variabel that will hold punch value
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, no errors occurs
 *                  	- Error code: Indicates failure during operation.
 *                  				  See error code definitions in Header File
 */
uint8_t dxl_get_punch(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PUNCH_L, data);
}


/*
 * @brief   Set dynamixel punch
 *
 * @param   id      	Dynamixel target ID
 * @param   current   	Desired value for punch (0X20-0X3FF)
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, dynamixel punch set successfully
 *                  	- Error code: Indicates failure during operation.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_set_punch(uint8_t id, uint16_t current)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_PUNCH_L, current);
}

/*================================================================================================
 * EEPROM Area - Safety Settings
 *==============================================================================================*/

/*
 * @brief   Get Dynamixel temperature limit value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_temperature_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LIMIT_TEMPERATURE, data);
}


/*
 * @brief   Set Dynamixel temperature limit
 *
 * @param   id      Dynamixel target ID
 * @param   value   Desired value for temperature limit (0–99)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, temperature limit set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_temperature_limit(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_LIMIT_TEMPERATURE, value);
}


/*
 * @brief   Get Dynamixel minimum voltage limit value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_min_voltage_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_MIN_VOLTAGE_LIMIT, data);
}


/*
 * @brief   Set Dynamixel minimum voltage limit
 *
 * @param   id      Dynamixel target ID
 * @param   value   Desired value for minimum voltage limit (50–160) with 0.1V per unit
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, minimum voltage limit set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_min_voltage_limit(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_MIN_VOLTAGE_LIMIT, value);
}


/*
 * @brief   Get Dynamixel maximum voltage limit value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_max_voltage_limit(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_MAX_VOLTAGE_LIMIT, data);
}


/*
 * @brief   Set Dynamixel maximum voltage limit
 *
 * @param   id      Dynamixel target ID
 * @param   value   Desired value for maximum voltage limit (50–160) with 0.1V per unit
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, maximum voltage limit set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_max_voltage_limit(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_MAX_VOLTAGE_LIMIT, value);
}


/*
 * @brief   Get Dynamixel alarm LED configuration
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold alarm LED mask value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_alarm_led(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_ALARM_LED, data);
}


/*
 * @brief   Set Dynamixel alarm LED configuration
 *
 * @param   id      Dynamixel target ID
 * @param   value   Desired LED mask value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, alarm LED configuration set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_alarm_led(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_ALARM_LED, value);
}


/*
 * @brief   Get Dynamixel shutdown alarm configuration
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold shutdown mask value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_shutdown(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_ALARM_SHUTDOWN, data);
}


/*
 * @brief   Set Dynamixel shutdown alarm configuration
 *
 * @param   id      Dynamixel target ID
 * @param   value   Bit mask representing error conditions that trigger shutdown
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, shutdown configuration set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_shutdown(uint8_t id, uint8_t value)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_ALARM_SHUTDOWN, value);
}

/*================================================================================================
 * RAM Area - Torque & LED Control
 *==============================================================================================*/

/*
 * @brief   Get torque enable state
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold torque state (0 or 1)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_torque_enable(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_TORQUE_ENABLE, data);
}


/*
 * @brief   Enable or disable Dynamixel torque
 *
 * @param   id      Dynamixel target ID
 * @param   state   Torque state (0: Disable, 1: Enable)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, torque state set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_torque_enable(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_TORQUE_ENABLE, state);
}


/*
 * @brief   Register the Dynamixel torque state
 *
 * @param   id      Dynamixel target ID
 * @param   state   Torque state (0: Disable, 1: Enable)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, torque state set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_reg_torque_enable(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_REG_WRITE, AX_TORQUE_ENABLE, state);
}


/*
 * @brief   Get LED state
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold LED state (0 or 1)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_led(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LED, data);
}


/*
 * @brief   Set Dynamixel LED state
 *
 * @param   id      Dynamixel target ID
 * @param   state   LED state (0: Off, 1: On)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, LED state set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_led(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_LED, state);
}


/*
 * @brief   Register the Dynamixel LED state
 *
 * @param   id      Dynamixel target ID
 * @param   state   LED state (0: Off, 1: On)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, LED state set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_reg_led(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_REG_WRITE, AX_LED, state);
}


/*
 * @brief   Get EEPROM lock state
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold lock state (0 or 1)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_lock(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_LOCK, data);
}


/*
 * @brief   Set EEPROM lock state
 *
 * @param   id      Dynamixel target ID
 * @param   state   Lock state (0: Unlock, 1: Lock)
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, lock state set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_lock(uint8_t id, uint8_t state)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_LOCK, state);
}

/*================================================================================================
 * RAM Area - Compliance Settings
 *==============================================================================================*/

/*
 * @brief   Get clockwise compliance margin value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold compliance margin value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_cw_compliance_margin(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_CW_COMPLIANCE_MARGIN, data);
}


/*
 * @brief   Set clockwise compliance margin value
 *
 * @param   id      Dynamixel target ID
 * @param   margin  Desired compliance margin value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, compliance margin set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_cw_compliance_margin(uint8_t id, uint8_t margin)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_CW_COMPLIANCE_MARGIN, margin);
}


/*
 * @brief   Get counterclockwise compliance slope value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold compliance slope value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_ccw_compliance_slope(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_CCW_COMPLIANCE_SLOPE, data);
}


/*
 * @brief   Set counterclockwise compliance slope value
 *
 * @param   id      Dynamixel target ID
 * @param   steps   Desired compliance slope value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, compliance slope set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_ccw_compliance_slope(uint8_t id, uint8_t steps)
{
	return dxl_write_u8(id, AX_WRITE_DATA, AX_CCW_COMPLIANCE_SLOPE, steps);
}

/*================================================================================================
 * Motion Control
 *==============================================================================================*/

/*
 * @brief   Get goal position value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold goal position value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_goal_position(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_GOAL_POSITION_L, data);
}


/*
 * @brief   Set the goal position of a Dynamixel
 *
 * @param   id       Dynamixel target ID
 * @param   position Target position value (0-1023)
 * @return  uint8_t  Status of the set goal operation:
 *                   - 0: Success, goal position set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_set_goal_position(uint8_t id, uint16_t position)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_GOAL_POSITION_L, position);
}


/*
 * @brief   Register the goal position of a Dynamixel to be executed upon
 * 			receiving the ACTION command
 *
 * @param   id       Dynamixel target ID
 * @param   position Target position value (0-1023)
 * @return  uint8_t  Status of the set goal operation:
 *                   - 0: Success, goal position set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_reg_goal_position(uint8_t id, uint16_t position)
{
	return dxl_write_u16(id, AX_REG_WRITE, AX_GOAL_POSITION_L, position);
}


/*
 * @brief   Get moving speed value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold moving speed value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_moving_speed(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_MOVING_SPEED_L, data);
}


/*
 * @brief   Set the moving speed of a Dynamixel
 *
 * @param   id       Dynamixel target ID
 * @param   speed    Desired speed value (0-1023)
 * @return  uint8_t  Status of the set speed operation:
 *                   - 0: Success, moving speed set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_set_moving_speed(uint8_t id, uint16_t speed)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_MOVING_SPEED_L, speed);
}


/*
 * @brief   Register the moving speed of a Dynamixel to be executed upon
 * 			receiving the ACTION command
 *
 * @param   id       Dynamixel target ID
 * @param   speed    Desired speed value (0-1023)
 * @return  uint8_t  Status of the set speed operation:
 *                   - 0: Success, moving speed set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_reg_moving_speed(uint8_t id, uint16_t speed)
{
	return dxl_write_u16(id, AX_REG_WRITE, AX_MOVING_SPEED_L, speed);
}


/*
 * @brief   Set the goal position and moving speed of a Dynamixel
 *
 * @param   id       Dynamixel target ID
 * @param   speed    Desired speed value (0-1023)
 * @return  uint8_t  Status of the set speed operation:
 *                   - 0: Success, moving speed set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_set_goal_position_and_speed(uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t params[4] =
    {
        position & 0xFF,
        position >> 8,
        speed & 0xFF,
        speed >> 8
    };

    return dxl_write_len(id, AX_WRITE_DATA, AX_GOAL_POSITION_L, params, 4);
}


/*
 * @brief   Register the goal position and moving speed of a Dynamixel to be executed upon
 * 			receiving the ACTION command
 *
 * @param   id       Dynamixel target ID
 * @param   speed    Desired speed value (0-1023)
 * @return  uint8_t  Status of the set speed operation:
 *                   - 0: Success, moving speed set successfully
 *                   - Error code: Indicates failure during operation.
 *                                 See error code definitions in Header File
 */
uint8_t dxl_reg_goal_position_and_speed(uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t params[4] =
    {
        position & 0xFF,
        position >> 8,
        speed & 0xFF,
        speed >> 8
    };

    return dxl_write_len(id, AX_REG_WRITE, AX_GOAL_POSITION_L, params, 4);
}


/*
 * @brief   Get torque limit value
 *
 * @param   id      Dynamixel target ID
 * @param   data    Pointer to a variable that will hold torque limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, no errors occur
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_get_torque_limit(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_TORQUE_LIMIT_L, data);
}


/*
 * @brief   Set torque limit value
 *
 * @param   id      Dynamixel target ID
 * @param   torque  Desired torque limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, torque limit set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_set_torque_limit(uint8_t id, uint16_t torque)
{
	return dxl_write_u16(id, AX_WRITE_DATA, AX_TORQUE_LIMIT_L, torque);
}


/*
 * @brief   Register Dynamixel torque limit value
 *
 * @param   id      Dynamixel target ID
 * @param   torque  Desired torque limit value
 * @return  uint8_t Status of the operation:
 *                  - 0: Success, torque limit set successfully
 *                  - Error code: Indicates failure during operation.
 *                                See error code definitions in Header File
 */
uint8_t dxl_reg_torque_limit(uint8_t id, uint16_t torque)
{
	return dxl_write_u16(id, AX_REG_WRITE, AX_TORQUE_LIMIT_L, torque);
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
 *                                	  See error code definitions in Header File
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
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_present_load(uint8_t id, uint16_t *data)
{
	return dxl_read_u16(id, AX_PRESENT_LOAD_L, data);
}


/*
 * @brief   Get the current input voltage of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the voltage data will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, voltage data retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_present_voltage(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_PRESENT_VOLTAGE, data);
}


/*
 * @brief   Get the current temperature of a Dynamixel
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the temperature data will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, temperature data retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_present_temperature(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_PRESENT_TEMPERATURE, data);
}


/*
 * @brief   Check if a Dynamixel has a registered instruction waiting for ACTION command
 *
 * @param   id      	ID of the Dynamixel to query
 * @param   data   		Pointer to a variable where the registered status will be stored
 * @return  uint8_t 	Status of the operation:
 *                  	- 0: Success, registered status retrieved successfully
 *                  	- Error code: Indicates failure during data retrieval.
 *                                	  See error code definitions in Header File
 */
uint8_t dxl_get_registered(uint8_t id, uint8_t *data)
{
	return dxl_read_u8(id, AX_REGISTERED_INSTRUCTION, data);
}


/*
 * @brief   Check if a Dynamixel servo is currently moving
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
