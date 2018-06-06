#ifndef __MAG3110_H__
#define __MAG3110_H__

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

#include <stint.h>

////////////////////////////////////////////////////////////////////////////////
/// \file mag3110.h
/// \brief library used to manage mag3110 micro
/// \author ox223252
/// \date 2018-06
/// \copyright GPLv2
/// \version 0.1
/// \warning work in progress
/// \bug not tested
////////////////////////////////////////////////////////////////////////////////

static int configMAG3110 ( const int mag3110Fd );

int testSYSMODMAG3110 ( const int mag3110Fd );

int openMAG3110 ( const char busName[], const uint8_t address, 
	int * const mag3110Fd );

int closeMAG3110 ( const int mag3110Fd );

int readMAG3110 ( uint16_t * const x, uint16_t * const y, uint16_t * const z, 
	const int mag3110Fd  );

#endif