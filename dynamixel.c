/*
 * dynamixel.c
 *
 * Author  : Bintang Cahya
 * Version : 1.0.3
 */

#include "dynamixel.h"

/*================================================================================================
 * Communication Direction Control
 *==============================================================================================*/


static void dxl_tx_mode(DXL_HandleTypeDef *dxl)
{
    HAL_GPIO_WritePin(dxl->gpiox, dxl->gpio_pin, GPIO_PIN_SET);
}

static void dxl_rx_mode(DXL_HandleTypeDef *dxl)
{
    HAL_GPIO_WritePin(dxl->gpiox, dxl->gpio_pin, GPIO_PIN_RESET);
}


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
		return DXL_ERR_COMMUNICATION;

	/* ID Check */
	if(packet[2] != expected_id)
		return DXL_ERR_COMMUNICATION;

	/* Length Check */
	if(packet[3] != (length - 4))
	    return DXL_ERR_COMMUNICATION;

	/* Checksum Check */
	if(dxl_checksum(packet, length) != packet[length - 1])
		return DXL_ERR_CHECKSUM;

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
static uint8_t dxl_receive_status_packet(DXL_HandleTypeDef *dxl, uint8_t expected_id, uint8_t *packet, uint8_t length)
{
	if(expected_id == DXL_BROADCAST_ID)
		return 0;

	dxl_rx_mode(dxl);

	if(HAL_UART_Receive(dxl->huart, packet, length, DXL_RECEIVE_TIMEOUT) != HAL_OK)
		return DXL_ERR_COMMUNICATION;

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
 * @brief   Read one or more bytes from a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   address     Register start address
 * @param   out         Pointer to receive buffer
 * @param   len         Number of bytes to read
 * @return  uint8_t     Dynamixel error code
 */
static uint8_t dxl_read_len(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t address, uint8_t *out, uint8_t len)
{
    if (out == NULL || dxl == NULL || id == DXL_BROADCAST_ID)
        return DXL_ERR_NULL;

    uint8_t params[2];
    uint8_t tx_packet[8];
    uint8_t rx_packet[DXL_PACKET_LENGTH + len];

    /* Parameters: address + length */
    params[0] = address;
    params[1] = len;

    uint8_t packet_length = dxl_build_packet(id, DXL_READ_DATA, params, sizeof(params), tx_packet);

    dxl_tx_mode(dxl);

    if (HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
        return DXL_ERR_COMMUNICATION;

    while (__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

    uint8_t err = dxl_receive_status_packet(dxl, id, rx_packet, sizeof(rx_packet));
    if (err) return err;

    /* Copy N bytes from status packet */
    memcpy(out, &rx_packet[5], len);

    return 0;
}


/*
 * @brief   Read an 8-bit value from a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   address     Register address
 * @param   data        Pointer to received 8-bit value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_read_u8(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t address, uint8_t *data)
{
	return dxl_read_len(dxl, id, address, data, 1);
}


/*
 * @brief   Read a 16-bit value from a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   address     Register address
 * @param   data        Pointer to received 16-bit value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_read_u16(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t address, uint16_t *data)
{
    uint8_t buf[2];
    uint8_t err = dxl_read_len(dxl, id, address, buf, 2);
    if (err) return err;

    *data = buf[0] | (buf[1] << 8);
    return 0;
}


/*
 * @brief   Write one or more bytes to a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   instruction Dynamixel instruction type
 * @param   address     Register start address
 * @param   data        Pointer to transmit data buffer
 * @param   len         Number of bytes to write
 * @return  uint8_t     Dynamixel error code
 */
static uint8_t dxl_write_len(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t instruction, uint8_t address, const uint8_t *data, uint8_t len)
{
	if (dxl == NULL)
		return DXL_ERR_NULL;

    uint8_t tx_packet[DXL_PACKET_LENGTH + len + 1];
    uint8_t rx_packet[6];
    uint8_t params[1 + len];   // address + data length

    params[0] = address;
    memcpy(&params[1], data, len);

    uint8_t packet_length = dxl_build_packet(id, instruction, params, 1 + len, tx_packet);

    dxl_tx_mode(dxl);

    if (HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
        return DXL_ERR_COMMUNICATION;

    while (__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

    return dxl_receive_status_packet(dxl, id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Write an 8-bit value to a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   instruction Dynamixel instruction type
 * @param   address     Register address
 * @param   value       8-bit value to write
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_write_u8(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t instruction, uint8_t address, uint8_t value)
{
	return dxl_write_len(dxl, id, instruction, address, &value, 1);
}


/*
 * @brief   Write a 16-bit value to a Dynamixel register
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   instruction Dynamixel instruction type
 * @param   address     Register address
 * @param   value       16-bit value to write
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_write_u16(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t instruction, uint8_t address, uint16_t value)
{
    uint8_t b[2] = { value & 0xFF, value >> 8 };
    return dxl_write_len(dxl, id, instruction, address, b, 2);
}


/*================================================================================================
 * Instruction Commands
 *==============================================================================================*/

/*
 * @brief   Ping a Dynamixel device to check communication availability
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @return  uint8_t     Dynamixel error code
 *                      - 0 : Device responded successfully
 *                      - Non-zero : Communication or protocol error
 */
uint8_t dxl_ping(DXL_HandleTypeDef *dxl, uint8_t id)
{
	if (dxl == NULL)
		return DXL_ERR_NULL;

	uint8_t tx_packet[6];
	uint8_t rx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, DXL_PING, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	dxl_tx_mode(dxl);

	/* Transmit Packet */
	if(HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
		return DXL_ERR_COMMUNICATION;

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(dxl, id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Reset all Dynamixel configuration to its factory setting
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @return  uint8_t     Dynamixel error code
 *                      - 0 : Device responded successfully
 *                      - Non-zero : Communication or protocol error
 */
uint8_t dxl_factory_reset(DXL_HandleTypeDef *dxl, uint8_t id)
{
	if (dxl == NULL)
		return DXL_ERR_NULL;

	uint8_t tx_packet[6];
	uint8_t rx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, DXL_FACTORY_RESET, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	dxl_tx_mode(dxl);

	/* Transmit Packet */
	if(HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
		return DXL_ERR_COMMUNICATION;

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(dxl, id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Restart Dynamixel
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @return  uint8_t     Dynamixel error code
 *                      - 0 : Device responded successfully
 *                      - Non-zero : Communication or protocol error
 */
uint8_t dxl_reboot(DXL_HandleTypeDef *dxl, uint8_t id)
{
	if (dxl == NULL)
		return DXL_ERR_NULL;

	uint8_t tx_packet[6];
	uint8_t rx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(id, DXL_REBOOT, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	dxl_tx_mode(dxl);

	/* Transmit Packet */
	if(HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
		return DXL_ERR_COMMUNICATION;

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

	/* Receive Status Packet */
	return dxl_receive_status_packet(dxl, id, rx_packet, sizeof(rx_packet));
}


/*
 * @brief   Perform Dynamixel Sync Write instruction
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   address     Starting control table address to write
 * @param   id          Pointer to array of Dynamixel IDs
 * @param   data        Pointer to data array
 *                      Data must be arranged sequentially per device:
 *                      [ID1_DATA0 ... ID1_DATAn]
 *                      [ID2_DATA0 ... ID2_DATAn]
 * @param   id_count    Number of Dynamixel devices
 * @param   len         Number of bytes written to each device
 *
 * @return  uint8_t     Dynamixel error code
 *                      - 0 : Sync Write transmitted successfully
 *                      - Non-zero : Communication or protocol error
 *
 * @note    Sync Write currently uses broadcast ID and does not return status packets.
 */
uint8_t dxl_sync_write(DXL_HandleTypeDef *dxl, uint8_t address, uint8_t *id, uint8_t *data, uint8_t id_count, uint8_t len)
{
    if (dxl == NULL)
        return DXL_ERR_NULL;

    /*
        Params format:
        [address]
        [data length]
        [ID1][DATA...]
        [ID2][DATA...]
    */

    // the data from user is assumed little-endian

    uint16_t params_len = 2 + (id_count * (1 + len));

    uint8_t params[params_len];

    params[0] = address;
    params[1] = len;

    uint16_t idx = 2;

    for (uint8_t i = 0; i < id_count; i++)
    {
        params[idx++] = id[i];

        for (uint8_t j = 0; j < len; j++)
        {
            params[idx++] = data[i * len + j];
        }
    }

    uint8_t tx_packet[6 + params_len];

    uint8_t packet_length = dxl_build_packet(DXL_BROADCAST_ID, DXL_SYNC_WRITE, params, params_len, tx_packet);

	/* Switch to TX Mode */
	dxl_tx_mode(dxl);

	/* Transmit Packet */
	if(HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT) != HAL_OK)
		return DXL_ERR_COMMUNICATION;

	/* Wait Until Transmission Complete */
	while(__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);

    return 0;
}





/*
 * @brief   Execute all previously registered REG_WRITE commands
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @note    This instruction is broadcast to all connected Dynamixel devices
 *          using DXL_BROADCAST_ID
 */
void dxl_action(DXL_HandleTypeDef *dxl)
{
	if (dxl == NULL)
		return;

	uint8_t tx_packet[6];

	/* Build Packet */
	uint8_t packet_length = dxl_build_packet(DXL_BROADCAST_ID, DXL_ACTION, NULL, 0, tx_packet);

	/* Switch to TX Mode */
	dxl_tx_mode(dxl);

	/* Transmit Packet */
	HAL_UART_Transmit(dxl->huart, tx_packet, packet_length, DXL_TRANSMIT_TIMEOUT);

	while(__HAL_UART_GET_FLAG(dxl->huart, UART_FLAG_TC) == RESET);
}


/*================================================================================================
 * EEPROM Area - Device Information
 *==============================================================================================*/

/*
 * @brief   Read the Dynamixel model number
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive the model number
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_model_number(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_MODEL_NUMBER_L, data);
}


/*
 * @brief   Read the Dynamixel firmware version
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive the firmware version
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_firmware_version(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_FIRMWARE_VERSION, data);
}


/*
 * @brief   Set the Dynamixel device ID
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Current Dynamixel ID
 * @param   new_id      New ID value (0 - 253)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_id(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t new_id)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_ID, new_id);
}


/*================================================================================================
 * EEPROM Area - Communication Settings
 *==============================================================================================*/

/*
 * @brief   Get Dynamixel baudrate setting
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive baudrate value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_baudrate(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_BAUD_RATE, data);
}


/*
 * @brief   Set Dynamixel baudrate
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   value       Baudrate configuration value:
 *                      1   = 1,000,000 bps (default)
 *                      3   = 500,000 bps
 *                      4   = 400,000 bps
 *                      7   = 250,000 bps
 *                      9   = 200,000 bps
 *                      16  = 115,200 bps
 *                      34  = 57,600 bps
 *                      103 = 19,200 bps
 *                      207 = 9,600 bps
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_baudrate(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_BAUD_RATE, value);
}


/*
 * @brief   Get Dynamixel return delay time
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive delay time value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_return_delay_time(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_RETURN_DELAY, data);
}


/*
 * @brief   Set Dynamixel return delay time
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   delay       Return delay time (0–254, unit: 2 µs per step)
 *                      Default: 250 (500 µs)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_return_delay_time(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t delay)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_RETURN_DELAY, delay);
}


/*
 * @brief   Get Dynamixel status return level
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive return level value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_status_return_level(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_STATUS_RETURN_LEVEL, data);
}


/*
 * @brief   Set Dynamixel status return level
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   level       Status return level:
 *                      0 : Reply only to PING
 *                      1 : Reply to PING and READ
 *                      2 : Reply to all instructions (default)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_status_return_level(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t level)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_STATUS_RETURN_LEVEL, level);
}


/*================================================================================================
 * EEPROM Area - Motion Limits
 *==============================================================================================*/

/*
 * @brief   Read CW angle limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive CW angle limit value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_cw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_CW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set CW angle limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   value       CW angle limit (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_cw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t value)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_CW_ANGLE_LIMIT_L, value);
}


/*
 * @brief   Read CCW angle limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive CCW angle limit value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_ccw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_CCW_ANGLE_LIMIT_L, data);
}


/*
 * @brief   Set CCW angle limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   value       CCW angle limit (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_ccw_angle_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t value)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_CCW_ANGLE_LIMIT_L, value);
}


/*
 * @brief   Read maximum torque limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive max torque value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_max_torque(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_MAX_TORQUE_L, data);
}


/*
 * @brief   Set maximum torque limit
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   max_torque  Maximum torque value (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_max_torque(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t max_torque)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_MAX_TORQUE_L, max_torque);
}


/*
 * @brief   Read punch value
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   data        Pointer to receive punch value
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_get_punch(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_PUNCH_L, data);
}


/*
 * @brief   Set punch value
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   current     Punch value (0x20–0x3FF)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_punch(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t current)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_PUNCH_L, current);
}

/*================================================================================================
 * EEPROM Area - Safety Settings
 *==============================================================================================*/

/*
 * @brief   Read Dynamixel temperature limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive temperature limit value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_temperature_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_LIMIT_TEMPERATURE, data);
}


/*
 * @brief   Set Dynamixel temperature limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   value   Temperature limit (0–99)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_temperature_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_LIMIT_TEMPERATURE, value);
}


/*
 * @brief   Read minimum voltage limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive minimum voltage value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_min_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_MIN_VOLTAGE_LIMIT, data);
}


/*
 * @brief   Set minimum voltage limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   value   Minimum voltage limit (50–160, unit: 0.1 V)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_min_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_MIN_VOLTAGE_LIMIT, value);
}


/*
 * @brief   Read maximum voltage limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive maximum voltage value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_max_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_MAX_VOLTAGE_LIMIT, data);
}


/*
 * @brief   Set maximum voltage limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   value   Maximum voltage limit (50–160, unit: 0.1 V)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_max_voltage_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_MAX_VOLTAGE_LIMIT, value);
}


/*
 * @brief   Read alarm LED configuration
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive alarm LED mask
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_alarm_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_ALARM_LED, data);
}


/*
 * @brief   Set alarm LED configuration
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   value   Alarm LED mask
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_alarm_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_ALARM_LED, value);
}


/*
 * @brief   Read shutdown configuration
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive shutdown mask
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_shutdown(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_ALARM_SHUTDOWN, data);
}


/*
 * @brief   Set shutdown configuration
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   value   Shutdown error mask (bit-field of fault conditions)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_shutdown(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t value)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_ALARM_SHUTDOWN, value);
}

/*================================================================================================
 * RAM Area - Torque & LED Control
 *==============================================================================================*/

/*
 * @brief   Get torque enable state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive torque state (0 = disabled, 1 = enabled)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_TORQUE_ENABLE, data);
}


/*
 * @brief   Set torque enable state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   state   Torque state (0 = disable, 1 = enable)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_TORQUE_ENABLE, state);
}


/*
 * @brief   Register torque enable state (REG_WRITE)
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   state   Torque state (0 = disable, 1 = enable)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_reg_torque_enable(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state)
{
	return dxl_write_u8(dxl, id, DXL_REG_WRITE, DXL_TORQUE_ENABLE, state);
}


/*
 * @brief   Get LED state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive LED state (0 = off, 1 = on)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_LED, data);
}


/*
 * @brief   Set LED state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   state   LED state (0 = off, 1 = on)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_LED, state);
}


/*
 * @brief   Register LED state (REG_WRITE)
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   state   LED state (0 = off, 1 = on)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_reg_led(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state)
{
	return dxl_write_u8(dxl, id, DXL_REG_WRITE, DXL_LED, state);
}


/*
 * @brief   Get EEPROM lock state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive lock state (0 = unlocked, 1 = locked)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_lock(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_LOCK, data);
}


/*
 * @brief   Set EEPROM lock state
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   state   Lock state (0 = unlock, 1 = lock)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_lock(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t state)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_LOCK, state);
}

/*================================================================================================
 * RAM Area - Compliance Settings
 *==============================================================================================*/

/*
 * @brief   Get CW compliance margin
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive CW compliance margin value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_cw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_CW_COMPLIANCE_MARGIN, data);
}


/*
 * @brief   Set CW compliance margin
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   margin  CW compliance margin value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_cw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t margin)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_CW_COMPLIANCE_MARGIN, margin);
}


/*
 * @brief   Get CCW compliance margin
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive CCW compliance margin value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_ccw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_CCW_COMPLIANCE_MARGIN, data);
}


/*
 * @brief   Set CCW compliance margin
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   margin  CCW compliance margin value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_ccw_compliance_margin(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t margin)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_CCW_COMPLIANCE_MARGIN, margin);
}


/*
 * @brief   Get CW compliance slope
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive CW compliance slope value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_cw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_CW_COMPLIANCE_SLOPE, data);
}


/*
 * @brief   Set CW compliance slope
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   slope   CW compliance slope value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_cw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t slope)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_CW_COMPLIANCE_SLOPE, slope);
}


/*
 * @brief   Get CCW compliance slope
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive CCW compliance slope value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_ccw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_CCW_COMPLIANCE_SLOPE, data);
}


/*
 * @brief   Set CCW compliance slope
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   slope   CCW compliance slope value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_ccw_compliance_slope(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t slope)
{
	return dxl_write_u8(dxl, id, DXL_WRITE_DATA, DXL_CCW_COMPLIANCE_SLOPE, slope);
}

/*================================================================================================
 * Motion Control
 *==============================================================================================*/

/*
 * @brief   Get goal position
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive goal position value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_GOAL_POSITION_L, data);
}


/*
 * @brief   Set goal position
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   position    Goal position (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_GOAL_POSITION_L, position);
}


/*
 * @brief   Register goal position (executed on ACTION command)
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   position    Goal position (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_reg_goal_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position)
{
	return dxl_write_u16(dxl, id, DXL_REG_WRITE, DXL_GOAL_POSITION_L, position);
}


/*
 * @brief   Get moving speed
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive moving speed value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_MOVING_SPEED_L, data);
}


/*
 * @brief   Set moving speed
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   speed   Moving speed (0–1023)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t speed)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_MOVING_SPEED_L, speed);
}


/*
 * @brief   Register moving speed (executed on ACTION command)
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   speed   Moving speed (0–1023)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_reg_moving_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t speed)
{
	return dxl_write_u16(dxl, id, DXL_REG_WRITE, DXL_MOVING_SPEED_L, speed);
}


/*
 * @brief   Set goal position and moving speed
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   position    Goal position (0–1023)
 * @param   speed       Moving speed (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_set_goal_position_and_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t params[4] =
    {
        position & 0xFF,
        position >> 8,
        speed & 0xFF,
        speed >> 8
    };

    return dxl_write_len(dxl, id, DXL_WRITE_DATA, DXL_GOAL_POSITION_L, params, 4);
}


/*
 * @brief   Register goal position and moving speed (executed on ACTION command)
 *
 * @param   dxl         Pointer to Dynamixel interface handle
 * @param   id          Target Dynamixel ID
 * @param   position    Goal position (0–1023)
 * @param   speed       Moving speed (0–1023)
 * @return  uint8_t     Dynamixel error code
 */
uint8_t dxl_reg_goal_position_and_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t params[4] =
    {
        position & 0xFF,
        position >> 8,
        speed & 0xFF,
        speed >> 8
    };

    return dxl_write_len(dxl, id, DXL_REG_WRITE, DXL_GOAL_POSITION_L, params, 4);
}


/*
 * @brief   Get torque limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive torque limit value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_TORQUE_LIMIT_L, data);
}


/*
 * @brief   Set torque limit
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   torque  Torque limit value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_set_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t torque)
{
	return dxl_write_u16(dxl, id, DXL_WRITE_DATA, DXL_TORQUE_LIMIT_L, torque);
}


/*
 * @brief   Register torque limit (executed on ACTION command)
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   torque  Torque limit value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_reg_torque_limit(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t torque)
{
	return dxl_write_u16(dxl, id, DXL_REG_WRITE, DXL_TORQUE_LIMIT_L, torque);
}

/*================================================================================================
 * Status & Feedback
 *==============================================================================================*/

/*
 * @brief   Get present position
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive position value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_present_position(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_PRESENT_POSITION_L, data);
}


/*
 * @brief   Get present speed
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive speed value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_present_speed(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_PRESENT_SPEED_L, data);
}


/*
 * @brief   Get present load
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive load value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_present_load(DXL_HandleTypeDef *dxl, uint8_t id, uint16_t *data)
{
	return dxl_read_u16(dxl, id, DXL_PRESENT_LOAD_L, data);
}


/*
 * @brief   Get present input voltage
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive voltage value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_present_voltage(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_PRESENT_VOLTAGE, data);
}


/*
 * @brief   Get present temperature
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive temperature value
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_present_temperature(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_PRESENT_TEMPERATURE, data);
}


/*
 * @brief   Get registered instruction status
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive registered flag (0 or 1)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_registered(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_REGISTERED_INSTRUCTION, data);
}


/*
 * @brief   Get moving status
 *
 * @param   dxl     Pointer to Dynamixel interface handle
 * @param   id      Target Dynamixel ID
 * @param   data    Pointer to receive moving flag (0 or 1)
 * @return  uint8_t Dynamixel error code
 */
uint8_t dxl_get_moving(DXL_HandleTypeDef *dxl, uint8_t id, uint8_t *data)
{
	return dxl_read_u8(dxl, id, DXL_MOVING, data);
}
