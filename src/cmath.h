#ifndef _CMATH_H_
#define _CMATH_H_
/*******************************************************************************
 * (c) Copyright 2012-2019 Norbit Subsea. All rights reserved.                 
 *******************************************************************************/
/* This file has been prepared for Doxygen automatic documentation generation. */
/****************************************************************************//**
* @file cmath.h
*
* @brief Collection of various math functions 
* @copyright 2012-2019 Norbit Subsea. All rights reserved.
* @author Magnus Andersen (Magnus.Andersen@norbit.no)
*
*/
/** @cond */
#include <stdint.h>
#ifdef __unix__
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
/** @endcond */

/** Simple macro for max of two values */
#define MAX(a,b) (((a)>(b))?(a):(b))
/** Simple macro for min of two values */
#define MIN(a,b) (((a)<(b))?(a):(b))
/** Simple macro for clip a value between two limits */
#define LIMIT(d,a,b) (MIN( MAX(d,a), b))
/** Simple macro for sign of value */
#define SIGN(a) ((a)<0?-1:1)
/** Simple macro for absolute value of value */
#define ABS(a) ((a)<0?-(a):(a))
/** Simple macro for median of three values */
#define MEDIAN3(a,b,c) (MAX(MIN(a,b), MIN(MAX(a,b),c)))

#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736 /** log2(e) */
#endif


#endif

