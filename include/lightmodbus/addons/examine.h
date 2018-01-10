/*
	liblightmodbus - a lightweight, multiplatform Modbus library
	Copyright (C) 2017 Jacek Wieczorek <mrjjot@gmail.com>

	This file is part of liblightmodbus.

	Liblightmodbus is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Liblightmodbus is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIGHTMODBUS_EXAMINE
#define LIGHTMODBUS_EXAMINE

#include <inttypes.h>
#include "../core.h"
#include "../libconf.h"

//Create macro aliases so they can be more easily connected to the modbusExamine call
#define MODBUS_EXAMINE_COIL MODBUS_COIL
#define MODBUS_EXAMINE_DISCRETE_INPUT MODBUS_DISCRETE_INPUT
#define MODBUS_EXAMINE_HOLDING_REGISTER MODBUS_HOLDING_REGISTER
#define MODBUS_EXAMINE_INPUT_REGISTER MODBUS_INPUT_REGISTER

#define MODBUS_EXAMINE_REQUEST  1
#define MODBUS_EXAMINE_RESPONSE 2

#define MODBUS_EXAMINE_READ 1
#define MODBUS_EXAMINE_WRITE 2

#define MODBUS_EXAMINE_UNDEFINED (-1)

typedef struct modbusFrameInfo
{
	//Struct members are signed types because each of them can take MODBUS_EXAMINE_UNDEFINED value
	uint8_t direction; //Just a friendly reminder (MODBUS_EXAMINE_REQUEST/MODBUS_EXAMINE_RESPONSE)
	uint8_t address; //Slave address
	int16_t function; //Function
	int16_t exception; //Exception number
	int8_t type; //Data type (MODBUS_EXAMINE_COIL/MODBUS_EXAMINE_HOLDING_REGISTER and so on)
	int32_t index; //Register index
	int32_t count; //Data unit count
	uint8_t access; //Access type (MODBUS_EXAMINE_READ/MODBUS_EXAMINE_WRITE)
	uint16_t crc; //CRC

	//Request 22 exclusive
	int32_t andmask, ormask;

	//Binary data - pointer and length in bytes
	//Important: Endianness of this data remains unchanged since this pointer points to the frame itself
	void *data;
	uint8_t length;
} ModbusFrameInfo;

extern uint8_t modbusExamine( ModbusFrameInfo *info, uint8_t dir, const uint8_t *frame, uint8_t length );

#endif
