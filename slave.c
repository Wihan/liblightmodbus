#include "slave.h"
#include <stdio.h> //*DEBUG*

MODBUSSlaveStatus MODBUSSlave; //Slave configuration

void MODBUSException( uint8_t Function, uint8_t ExceptionCode )
{
	//Generates modbus exception frame in allocated memory frame
	//Returns generated frame length

	//Allocate memory for union
	union MODBUSException *Exception = malloc( sizeof( MODBUSException ) );

	//Reallocate frame memory
	MODBUSSlave.Response.Frame = realloc( MODBUSSlave.Response.Frame, 5 );
	memset( MODBUSSlave.Response.Frame, 0, 5 );

	//Setup exception frame
	( *Exception ).Exception.Address = MODBUSSlave.Address;
	( *Exception ).Exception.Function = ( 1 << 7 ) | Function;
	( *Exception ).Exception.ExceptionCode = ExceptionCode;
	( *Exception ).Exception.CRC = MODBUSCRC16( ( *Exception ).Frame, 3 );

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Exception ).Frame, 5 );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = 5;

	//Free memory used for union
	free( Exception );
}

void MODBUSRequest03( union MODBUSParser *Parser )
{
	//Read multiple holding registers
	//Using data from union pointer

	//Update frame length
	uint8_t FrameLength = 8;

	//Check frame CRC
	if ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) != ( *Parser ).Request03.CRC ) return;

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request03.FirstRegister = MODBUSSwapEndian( ( *Parser ).Request03.FirstRegister );
	( *Parser ).Request03.RegisterCount = MODBUSSwapEndian( ( *Parser ).Request03.RegisterCount );

	//Check if register is in valid range
	if ( ( *Parser ).Request03.FirstRegister >= MODBUSSlave.RegisterCount || ( *Parser ).Request03.FirstRegister + ( *Parser ).Request03.RegisterCount > MODBUSSlave.RegisterCount )
	{
		//Illegal data address exception
		MODBUSException( 0x03, 0x02 );
		return;
	}

	//---- Response ----
	//Frame length is (with CRC): 5 + ( ( *Parser ).Request03.RegisterCount << 1 )
	//5 bytes of data and each register * 2b ( 1 + 1 + 1 + 2x + 2 )

	uint8_t i = 0;
	FrameLength = 5 + ( ( *Parser ).Request03.RegisterCount << 1 );
	union MODBUSParser *Builder = malloc( FrameLength ); //Allocate memory for builder union
	MODBUSSlave.Response.Frame = realloc( MODBUSSlave.Response.Frame, FrameLength ); //Reallocate response frame memory to needed memory
	memset( MODBUSSlave.Response.Frame, 0, FrameLength ); //Empty response frame

	//Set up basic response data
	( *Builder ).Response03.Address = MODBUSSlave.Address;
	( *Builder ).Response03.Function = ( *Parser ).Request03.Function;
	( *Builder ).Response03.BytesCount = ( *Parser ).Request03.RegisterCount << 1;

	//Copy registers to response frame
	for ( i = 0; i < ( *Parser ).Request03.RegisterCount; i++ )
		( *Builder ).Response03.Values[i] = MODBUSSwapEndian( MODBUSSlave.Registers[( *Parser ).Request03.FirstRegister + i] );

	//Calculate CRC
	( *Builder ).Response03.Values[( *Parser ).Request03.RegisterCount] = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 );

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Builder ).Frame, FrameLength );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = FrameLength;

	//Free union memory
	free( Builder );
}

void MODBUSRequest06( union MODBUSParser *Parser )
{
	//Write single holding register
	//Using data from union pointer

	//Update frame length
	uint8_t FrameLength = 8;

	//Check frame CRC
	if ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) != ( *Parser ).Request06.CRC ) return; //EXCEPTION (in future)

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request06.Register = MODBUSSwapEndian( ( *Parser ).Request06.Register );
	( *Parser ).Request06.Value = MODBUSSwapEndian( ( *Parser ).Request06.Value );

	//Check if register is in valid range
	if ( ( *Parser ).Request06.Register >= MODBUSSlave.RegisterCount )
	{
		//Illegal data address exception
		MODBUSException( 0x06, 0x02 );
		return;
	}

	//Write register
	MODBUSSlave.Registers[( *Parser ).Request06.Register] = ( *Parser ).Request06.Value;

	//---- Response ----
	//Frame length is (with CRC): 8
	//8 bytes of data ( 1 + 1 + 2 + 2 + 2 )

	uint8_t i = 0;
	FrameLength = 8;
	union MODBUSParser *Builder = malloc( FrameLength ); //Allocate memory for builder union
	MODBUSSlave.Response.Frame = realloc( MODBUSSlave.Response.Frame, FrameLength ); //Reallocate response frame memory to needed memory
	memset( MODBUSSlave.Response.Frame, 0, FrameLength ); //Empty response frame

	//Set up basic response data
	( *Builder ).Response06.Address = MODBUSSlave.Address;
	( *Builder ).Response06.Function = ( *Parser ).Request06.Function;
	( *Builder ).Response06.Register = ( *Parser ).Request06.Register;
	( *Builder ).Response06.Value = ( *Parser ).Request06.Value;

	//Calculate CRC
	( *Builder ).Response06.CRC = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 );

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Builder ).Frame, FrameLength );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = FrameLength;

	//Free union memory
	free( Builder );
}

void MODBUSRequest16( union MODBUSParser *Parser )
{
	//Write multiple holding registers
	//Using data from union pointer

	//Update frame length
	uint8_t i = 0;
	uint8_t FrameLength = 9 + ( *Parser ).Request16.BytesCount;

	//Check frame CRC
	//Shifting is used instead of dividing for optimisation on smaller devices (AVR)
	if ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) != ( *Parser ).Request16.Values[( *Parser ).Request16.BytesCount >> 1] ) return; //EXCEPTION (in future)

	//Check if bytes or registers count isn't 0
	if ( ( *Parser ).Request16.BytesCount == 0 || ( *Parser ).Request16.RegisterCount == 0 )
	{
		//Illegal data value error
		MODBUSException( 0x10, 0x03 );
		return;
	}

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request16.FirstRegister = MODBUSSwapEndian( ( *Parser ).Request16.FirstRegister );
	( *Parser ).Request16.RegisterCount = MODBUSSwapEndian( ( *Parser ).Request16.RegisterCount );

	//Check if bytes count *2 is equal to registers count
	if ( ( *Parser ).Request16.RegisterCount != ( ( *Parser ).Request16.BytesCount >> 1 ) )
	{
		//Illegal data value error
		MODBUSException( 0x10, 0x03 );
		return;
	}

	//Check if registers are in valid range
	if ( ( *Parser ).Request16.FirstRegister >= MODBUSSlave.RegisterCount || ( *Parser ).Request16.FirstRegister + ( *Parser ).Request16.RegisterCount > MODBUSSlave.RegisterCount )
	{
		//Illegal data address error
		MODBUSException( 0x10, 0x02 );
		return; //EXCEPTION (in future)
	}

	//Write values to registers
	for ( i = 0; i < ( *Parser ).Request16.RegisterCount; i++ )
		MODBUSSlave.Registers[( *Parser ).Request16.FirstRegister + i] = ( *Parser ).Request16.Values[i];

	//FORMAT RESPONSE HERE
	printf( "OK\n" ); //*DEBUG*
}

void MODBUSParseRequest( uint8_t *Frame, uint8_t FrameLength )
{
	//Parse and interpret given modbus frame on slave-side

	//Init parser union
	//This one is actually unsafe, so it's easy to create a segmentation fault, so be careful here
	//Allowable frame array size in union is 256, but here I'm only allocating amount of frame length
	//It is even worse, compiler won't warn you, when you are outside the range
	//It works, and it uses much less memory, so I guess a bit of risk is fine in this case
	//If something goes wrong, this can be changed back
	//Also, user needs to free memory alocated for frame himself!
	union MODBUSParser *Parser;
	Parser = malloc( FrameLength );
	memcpy( ( *Parser ).Frame, Frame, FrameLength );

	//Reset response frame status
	MODBUSSlave.Response.Length = 0;


	//If frame is not broadcasted and address doesn't match skip parsing
	if ( ( *Parser ).Base.Address != MODBUSSlave.Address && ( *Parser ).Base.Address != 0 )
	{
		free( Parser );
		return;
	}

	switch( ( *Parser ).Base.Function )
	{
		case 3: //Read multiple holding registers
			MODBUSRequest03( Parser );
			break;

		case 6: //Write single holding register
			MODBUSRequest06( Parser );
			break;

		case 16: //Write multiple holding registers
			MODBUSRequest16( Parser );
			break;

		default:
			//Illegal function exception
			MODBUSException( ( *Parser ).Base.Function, 0x01 );
			break;
	}

	free( Parser );
}

void MODBUSSlaveInit( uint8_t Address, uint16_t *Registers, uint16_t RegisterCount )
{
	//Very basic init of slave side
	MODBUSSlave.Address = Address;
	MODBUSSlave.Registers = Registers;
	MODBUSSlave.RegisterCount = RegisterCount;

	//Reset response frame status
	MODBUSSlave.Response.Length = 0;
	MODBUSSlave.Response.Frame = malloc( 8 );
}