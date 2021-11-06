/*
	MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
	Copyright (C) 2021 MistEO and Contributors

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// The way how the function is called
#if !defined(MEO_CALL)
#if defined(_WIN32)
#define MEO_CALL __stdcall
#else
#define MEO_CALL
#endif /* _WIN32 */
#endif /* MEO_CALL */

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define MEO_DLL_IMPORT __declspec(dllimport)
#define MEO_DLL_EXPORT __declspec(dllexport)
#define MEO_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MEO_DLL_IMPORT __attribute__ ((visibility ("default")))
#define MEO_DLL_EXPORT __attribute__ ((visibility ("default")))
#define MEO_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define MEO_DLL_IMPORT
#define MEO_DLL_EXPORT
#define MEO_DLL_LOCAL
#endif
#endif

#ifdef MEO_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define MEOAPI_PORT MEO_DLL_EXPORT
#else
#define MEOAPI_PORT MEO_DLL_IMPORT
#endif // MEO_DLL_EXPORTS

#define MEOAPI MEOAPI_PORT MEO_CALL

#define MEOLOCAL MEO_DLL_LOCAL MEO_CALL