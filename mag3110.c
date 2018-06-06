////////////////////////////////////////////////////////////////////////////////
/// \copiright ox223252, 2018
///
/// This program is free software: you can redistribute it and/or modify it
///     under the terms of the GNU General Public License published by the Free
///     Software Foundation, either version 2 of the License, or (at your
///     option) any later version.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
///     ANY WARRANTY; without even the implied of MERCHANTABILITY or FITNESS FOR
///     A PARTICULAR PURPOSE. See the GNU General Public License for more
///     details.
///
/// You should have received a copy of the GNU General Public License along with
///     this program. If not, see <http://www.gnu.org/licenses/>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \file mag3110.c
/// \brief library used to manage mag3110 micro
/// \author ox223252
/// \date 2018-06
/// \copyright GPLv2
/// \version 0.1
/// \warning work in progress
/// \bug not tested
////////////////////////////////////////////////////////////////////////////////

#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

typedef enum
{
	DR_STATUS, // data ready status per axis
	OUT_X_MSB, // bits [ 15 : 8 ] of X measurement
	OUT_X_LSB, // bits [  7 : 0 ] of X measurement
	OUT_Y_MSB, // bits [ 15 : 8 ] of Y measurement
	OUT_Y_LSB, // bits [  7 : 0 ] of Y measurement
	OUT_Z_MSB, // bits [ 15 : 8 ] of Z measurement
	OUT_Z_LSB, // bits [  7 : 0 ] of Z measurement
	WHO_AM_I,  // Device ID Number
	SYSMOD,    // Current System Mode
	OFF_X_MSB, // bits [ 14 : 7 ] of user X offset
	OFF_X_LSB, // bits [  6 : 0 ] of user X offset
	OFF_Y_MSB, // bits [ 14 : 7 ] of user Y offset
	OFF_Y_LSB, // bits [  6 : 0 ] of user Y offset
	OFF_Z_MSB, // bits [ 14 : 7 ] of user Z offset
	OFF_Z_LSB, // bits [  6 : 0 ] of user Z offset
	DIE_TEMP,  // Temperature, signed 8 bits in °C
	CTRL_REG1, // Operation modes
	CTRL_REG2, // Operation modes
	REG_NUMBER
}
registers;

typedef struct
{
	STANDBY = 0x00, // STANDBY mode.
	ARCTIVE_RAW = 0x01, // ACTIVE mode, RAW data.
	ARCTIVE_NON_RAW = 0x10 // ACTIVE mode, non-RAW user-corrected data.
}
sysmodRegStatus;

typedef struct
{
	uint8_t AC:1, // Operating mode selection. Default value: 0 
	              // 0: STANDBY mode.
	              // 1: ACTIVE mode
	              // ACTIVE mode will make periodic measurements based on values
	              // programmed int the Data Rate (DR) and Over Sampling Ration 
	              // bits (OS).
	    TM:1,     // Trigger immediate measurement. Default value: 0
	              // 0: Normal operation besad on AC condition.
	              // 1: Trigger measurement.
	              // if part is ACTIVE mode, any measurment in progress will 
	              // complete before tiggered measurment.
	              // In STANDBY mode triggerer measurment will occur immediately
	              // and part will return to STANDBY mode as soon as the 
	              // measurment is complete.
	    FR:1,     // Fast Read selection. Default value: 0
	              // 0: The full 16-bits values are read.
	              // 1: Fast Read, 8-bit values read from the MSB registers.
	    OS:2,     // This register configure the over sampling ratio or 
	              // measurement integration time.
	              // Default value: 0.
	              // use ctrl1RegOsStatus to configure it.
	    DR:3      // Data rate selection. Default value: 0.
	              // use ctrl1RegDrStatus to configure it.
}
ctrl1Reg;

typedef struct
{
	DR_80,
	DR_40,
	DR_20,
	DR_10,
	DR_5,
	DR_2_5,
	DR_1_25,
	DR_0_63,
	DR_NO_CHANGE
}
ctrl1RegDrStatus;

typedef struct
{
	OS_1,
	OS_2,
	OS_4,
	OS_8,
	OS_NO_CHANGE
}
ctrl1RegOsStatus;

typedef struct
{
	uint8_t ST_X:1, // Self-test X-axis. Default value: 0.
	                // 0: No Selft-test.
	                // 1: Selft-test, active X-axis.
	                // Whenthis bot is asserted, a magnetic field is generated 
	                // on the MAG3110 X-axis to test the operation of this axis.
	                // The Size of the resulting change is defined as Self-test 
	                // Output Change. The ST_X bot should be de-asserted at the 
	                // end of the  self-test.
		ST_Y:1,     // Self-test Y-axis. Default value: 0.
	                // 0: No Selft-test.
	                // 1: Selft-test, active Y-axis.
	                // Whenthis bot is asserted, a magnetic field is generated 
	                // on the MAG3110 Y-axis to test the operation of this axis.
	                // The Size of the resulting change is defined as Self-test 
	                // Output Change. The ST_Y bot should be de-asserted at the 
	                // end of the  self-test.
		ST_Z:1,     // Self-test Z-axis. Default value: 0.
	                // 0: No Selft-test.
	                // 1: Selft-test, active Z-axis.
	                // Whenthis bot is asserted, a magnetic field is generated 
	                // on the MAG3110 Z-axis to test the operation of this axis.
	                // The Size of the resulting change is defined as Self-test 
	                // Output Change. The ST_Z bot should be de-asserted at the 
	                // end of the  self-test.
		un1:1,      // unused
		Mag_RST:1,  // Magnetic Sensor Reset. Default value: 0.
		            // 0: Reset cycle not active.
		            // 1: Reset cycle initiate or Reset cycle busy/active.
		            // When asserted, initiatea magnetic sensor reset cycle taht
		            // will restor correct operation after exposure to an 
		            // excessive magnetic field which exceeds the Full Scale 
		            // Range but is less than the Maximum Applied Magnetic Field
		            // When the cycle is finished, value return to 0.
		RAW:1,      // Data output correction. Default value: 0.
		            // 0: Normal mode: data arec corrected by the user offset 
		            // register values.
		            // 1: Raw mode: data values are not corrected by the user 
		            // offset register values.
		            // The factory calibration is always applied to the measured
		            // data stored in register 0x01 to 0x06 irrespective of the
		            // setting of the RAW bit.
		un2:1,      // unused
		AUTO_MRST_EN:1 // Automatic Magnetic Sensor reset. Default value: 0.
		            // 0: Automatic magnetic sensor resets off.
		            // 1: Automatic magnetic sensor resets on.
		            // Similar to Mag_RST, however, the resets occur before each
		            // data acquisition.
}
ctrl2Reg;

static sysmodRegStatus sysmod = STANDBY;

static int configMAG3110 ( const int mag3110Fd )
{
	struct 
	{
		ctrl1Reg ctrl1;
		ctrl2Reg ctrl2;
	}
	cofnig;
	uint8_t buf[ 3 ];
	
	if ( testSYSMODMAG3110 ( mag3110Fd ) )
	{
		return ( __LINE__ );
	}
	
	buf[ 0 ] = CTRL_REG1;

	if ( write ( mag3110, buf, 1 ) )
	{ // request for systmod
		return ( __LINE__ );
	} 
	if ( read ( mag3110, &cofnig, 2 ) )
	{ // Read sysmod
		return ( __LINE__ );
	}

	if ( sysmod != STANDBY )
	{ // need to swith mode to standby
		config.ctrl1.AC = 0;
		config.ctrl1.DR = DR_80;
		config.ctrl1.OS = OS_8;
		config.ctrl1.FR = 0;

		memcpy ( &buf[ 1 ], config, 2 );

		if ( write ( mag3110, buf, 3 ) )
		{ // request for systmod
			return ( __LINE__ );
		}
	}

	config.ctrl1.AC = 1;
	config.ctrl1.DR = DR_80;
	config.ctrl1.OS = OS_8;
	config.ctrl1.FR = 0;

	memcpy ( &buf[ 1 ], config, 2 );

	if ( write ( mag3110, buf, 3 ) )
	{ // request for systmod
		return ( __LINE__ );
	}

	if ( testSYSMODMAG3110 ( mag3110Fd ) )
	{
		return ( __LINE__ );
	}

	return ( 0 );
}

int testSYSMODMAG3110 ( const int mag3110Fd )
{
	uint8_t addr = SYSMOD;
	
	if ( write ( mag3110, addr, 1 ) )
	{ // request for systmod
		return ( __LINE__ );
	} 
	if ( read ( mag3110, &sysmod, 1 ) )
	{ // Read sysmod
		return ( __LINE__ );
	}
	return ( 0 );
}

int openMAG3110 ( const char busName[], const uint8_t address, 
	int * const mag3110Fd )
{
	uint8_t buf[ 2 ] = { 0 }; //buffer to store data needed to be sent
	int rt = 0; // return value

	*mag3110 = open ( busName, O_RDWR );
	if ( *mag3110 < 0 )
	{
		return ( __LINE__ );
	}

	if ( ioctl ( *mag3110, I2C_SLAVE, address ) < 0 )
	{
		return ( __LINE__ );
	}

	if ( configMAG3110 ( *mag3110 ) )
	{
		return ( __LINE__ );
	}

	return (  );
}

int closeMAG3110 ( const int mag3110Fd )
{
	return ( close ( mag3110Fd ) );
}

int readMAG3110 ( uint16_t * const x, uint16_t * const y, uint16_t * const z, 
	const int mag3110Fd  )
{
	uint8_t addr = 0;

	if ( !x ||
		!y ||
		!z )
	{
		errno = EINVAL;
		return ( __LINE__ );
	}

	addr = OUT_X_MSB;

	if ( write ( mag3110, &addr, 1 ) )
	{
		return ( __LINE__ );
	}
	if ( read ( mag3110, x, 2 ) ||
		read ( mag3110, y, 2 ) ||
		read ( mag3110, z, 2 ) )
	{
		return ( __LINE__ );
	}

	// inver MSB / LSB to reorder Bytes
	addr = x[ 0 ];
	x[ 0 ] = x[ 1 ];
	x[ 1 ] = addr;

	addr = y[ 0 ];
	y[ 0 ] = y[ 1 ];
	y[ 1 ] = addr;

	addr = z[ 0 ];
	z[ 0 ] = z[ 1 ];
	z[ 1 ] = addr;

	return ( 0 );
}