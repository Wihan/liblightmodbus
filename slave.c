#include "slave.h"

uint8_t MODBUSAddress; //Address of device
uint16_t *MODBUSRegisters; //Pointer to slave-side modbus registers
uint16_t MODBUSRegisterCount; //Count of slave-side modbus registers

void MODBUSParseRequest( uint8_t *Frame, uint8_t FrameLength )
{
	//Parse and interpret given modbus frame on slave-side

	//Init parser union
	union MODBUSParser Parser;
	memcpy( Parser.Frame, Frame, FrameLength );

	//User needs to free memory alocated for frame himself!

	//If frame is not broadcasted and address doesn't match skip parsing
	if ( Parser.Base.Address != MODBUSAddress && Parser.Base.Address != 0 ) return;

	switch( Parser.Base.Function )
	{
		case 3: //Read multiple holding register

			//Check frame CRC
			if ( MODBUSCRC16( Parser.Frame, 6 ) != Parser.Request03.CRC ) return; //EXCEPTION (in future)

			//Swap endianness of longer members (but not CRC)
			Parser.Request03.FirstRegister = MODBUSSwapEndian( Parser.Request03.FirstRegister );
			Parser.Request03.RegisterCount = MODBUSSwapEndian( Parser.Request03.RegisterCount );

			break;

		case 6: //Write single holding register
			break;

		case 16: //Write multiple holding registers
			break;
	}
}
