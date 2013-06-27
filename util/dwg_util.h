/*
 * dwg_util.h
 *
 *  Created on: Sep 19, 2012
 *      Author: carlos
 */

#ifndef DWG_UTIL_H_
#define DWG_UTIL_H_

#include <stdio.h>

typedef unsigned int 	_uint;
typedef unsigned short 	_ushort;
typedef unsigned char 	_uchar;
typedef unsigned int 	_bool;

typedef struct str
{
	char *s;
	int len;
} str_t;


#define FALSE			0
#define TRUE			1

#define L_DEBUG			0
#define L_WARNING		1
#define L_ERROR			2

#define LOG(LEVEL, format, ...) 			\
		if (LEVEL == L_DEBUG) { 			\
			printf("[DEBUG] ");  			\
			printf(format, __VA_ARGS__); 	\
		} 									\
		else if (LEVEL == L_WARNING) { 		\
			printf("[WARNING] ");  			\
			printf(format, __VA_ARGS__); 	\
		} 									\
		else if (LEVEL == L_ERROR) { 		\
			printf("[ERROR] ");  			\
			printf(format, __VA_ARGS__); 	\
		}

#define STR_ALLOC(_str_, _size_) _str_.s       = (char *) calloc(_size_, sizeof(char)); \
                                                                  _str_.len = _size_;

#define STR_FREE(_str_) free(_str_.s);

#define STR_FREE_NON_0(_str_) if (_str_.s != NULL) free(_str_.s);

#define str_copy(_str_to_, _str_from_) _str_to_.s = calloc(_str_from_.len + 1, sizeof(char)); \
									   _str_to_.len = _str_from_.len; \
									   strcpy(_str_to_.s, _str_from_.s); \


#endif /* DWG_UTIL_H_ */
