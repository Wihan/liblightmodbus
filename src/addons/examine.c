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

#include <lightmodbus/core.h>
#include <lightmodbus/parser.h>
#include <lightmodbus/addons/examine.h>
#include <string.h>
#include <stddef.h>

//Examines Modbus frame and returns information in ModbusFrameInfo
//This function doesn't perform ANY data checking apart from CRC check
//Please keep in mind, that if return value is different from MODBUS_ERROR_OK or MODBUS_ERROR_EXCEPTION, the data returned in the structure is worthless
uint8_t modbusExamine( ModbusFrameInfo *info, uint8_t dir, const uint8_t *frame, uint8_t length )
{
	union modbusParser *parser;

	//Null pointer check
	if ( info == NULL ) return MODBUS_ERROR_OTHER;

	//Struct setup - assume everything is undefined
	info->direction = dir;
	info->address = MODBUS_EXAMINE_UNDEFINED;
	info->function = MODBUS_EXAMINE_UNDEFINED;
	info->exception = MODBUS_EXAMINE_UNDEFINED;
	info->type = MODBUS_EXAMINE_UNDEFINED;
	info->index = MODBUS_EXAMINE_UNDEFINED;
	info->count = MODBUS_EXAMINE_UNDEFINED;
	info->access = MODBUS_EXAMINE_UNDEFINED;
	info->crc = MODBUS_EXAMINE_UNDEFINED;
	info->andmask = MODBUS_EXAMINE_UNDEFINED;
	info->ormask = MODBUS_EXAMINE_UNDEFINED;
	info->length = 0;
	info->data = NULL; //This is for weird patforms that don't consider 0 to be NULL

	//Check for bad frame
	if ( length == 0 || frame == NULL ) return MODBUS_ERROR_OTHER;

	parser = (union modbusParser*) frame;

	//CRC check (should satisfy both little and big endian platforms - see modbusCRC)
	info->crc = modbusCRC( frame, length - 2 );
	if ( info->crc != ( frame[length - 2] | ( frame[length - 1] << 8 ) ) ) return MODBUS_ERROR_CRC;

	//Copy basic information
	info->address = parser->base.address;
	info->function = parser->base.function;

	//Determine data and access type - filter out exception bit
	switch ( parser->base.function & 127 )
	{
		//Coils - read
		case 01:
			info->type = MODBUS_EXAMINE_COIL;
			info->access = MODBUS_EXAMINE_READ;
			break;
		
		//Discrete inputs - read
		case 02:
			info->type = MODBUS_EXAMINE_DISCRETE_INPUT;
			info->access = MODBUS_EXAMINE_READ;
			break;

		//Holding registers - read
		case 03:
			info->type = MODBUS_EXAMINE_HOLDING_REGISTER;
			info->access = MODBUS_EXAMINE_READ;
			break;
			
		//Input registers - read
		case 04:
			info->type = MODBUS_EXAMINE_INPUT_REGISTER;
			info->access = MODBUS_EXAMINE_READ;
			break;

		//Coils - write
		case 05:
		case 15:
			info->type = MODBUS_EXAMINE_COIL;
			info->access = MODBUS_EXAMINE_WRITE;
			break;

		//Holding registers - write
		case 06:
		case 16:
		case 22:
			info->type = MODBUS_EXAMINE_HOLDING_REGISTER;
			info->access = MODBUS_EXAMINE_WRITE;
			break;

		//Unknown function
		default:
			return MODBUS_ERROR_OTHER;
			break;
	}

	//Interpret exception - we don't care about data direction
	if ( length == 5 && ( parser->base.function & 128 ) )
	{
		info->exception = parser->exception.code;
		return MODBUS_ERROR_EXCEPTION;
	}

	if ( dir == MODBUS_EXAMINE_REQUEST )
	{
		switch ( parser->base.function )
		{
			//Reading multiple coils/discrete inputs
			case 01:
			case 02:
				info->index = modbusSwapEndian( parser->request0102.index );
				info->count = modbusSwapEndian( parser->request0102.count );
				break;

			//Reading multiple holding/input registers
			case 03:
			case 04:
				info->index = modbusSwapEndian( parser->request0304.index );
				info->count = modbusSwapEndian( parser->request0304.count );
				break;

			//Write single coil
			case 05:
				info->index = modbusSwapEndian( parser->request05.index );
				info->data = &parser->request05.value;
				info->length = 2;
				info->count = 1;
				break;

			//Write single holding register
			case 06:
				info->index = modbusSwapEndian( parser->request06.index );
				info->data = &parser->request06.value;
				info->length = 2;
				info->count = 1;
				break;

			//Write multiple coils
			case 15:
				info->index = modbusSwapEndian( parser->request15.index );
				info->count = modbusSwapEndian( parser->request15.count );
				info->data = parser->request15.values;
				info->length = parser->request15.length;
				break;

			//Write multiple registers
			case 16:
				info->index = modbusSwapEndian( parser->request16.index );
				info->count = modbusSwapEndian( parser->request16.count );
				info->data = parser->request16.values;
				info->length = parser->request16.length;
				break;

			//Mask write
			case 22:
				info->index = modbusSwapEndian( parser->request22.index );
				info->count = 1;
				info->andmask = modbusSwapEndian( parser->request22.andmask );
				info->ormask = modbusSwapEndian( parser->request22.ormask );
				break;

			//Unknown function
			default:
				return MODBUS_ERROR_OTHER;
				break;
		}
	}
	else if ( MODBUS_EXAMINE_RESPONSE ) //Response parsing
	{
		switch ( info->function )
		{
			//Reading multiple coils/discrete inputs
			case 01:
			case 02:
				info->data = parser->response0102.values;
				info->length = parser->response0102.length;
				break;

			//Reading multiple holding/input registers
			case 03:
			case 04:
				info->data = parser->response0304.values;
				info->length = parser->response0304.length;
				break;

			//Write single coil
			case 05:
				info->index = modbusSwapEndian( parser->response05.index );
				info->data = &parser->response05.value;
				info->length = 2;
				info->count = 1;
				break;

			//Write single holding register
			case 06:
				info->index = modbusSwapEndian( parser->response06.index );
				info->data = &parser->response06.value;
				info->length = 2;
				info->count = 1;
				break;

			//Write multiple coils
			case 15:
				info->index = modbusSwapEndian( parser->response15.index );
				info->count = modbusSwapEndian( parser->response15.count );
				break;

			//Write multiple registers
			case 16:
				info->index = modbusSwapEndian( parser->response16.index );
				info->count = modbusSwapEndian( parser->response16.count );
				break;

			//Mask write
			case 22:
				info->index = modbusSwapEndian( parser->response22.index );
				info->count = 1;
				info->andmask = modbusSwapEndian( parser->response22.andmask );
				info->ormask = modbusSwapEndian( parser->response22.ormask );
				break;

			//Unknown function
			default:
				return MODBUS_ERROR_OTHER;
				break;
		}
	}
	else return MODBUS_ERROR_OTHER;

	return MODBUS_ERROR_OK;
}
