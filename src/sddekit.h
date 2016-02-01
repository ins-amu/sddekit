/* copyright 2016 Apache 2 sddekit authors */

/**  SDDEKit
 */

#ifndef SDDEKIT_H
#define SDDEKIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* standard C includes {{{ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
/* }}} */

/* -DSD_API_EXPORT dllexport {{{ */
#ifdef SD_API_EXPORT
#define SD_API __declspec(dllexport)
#else
#ifdef SD_API_IMPORT
#define SD_API __declspec(dllimport)
#else
#define SD_API
#endif
#endif
/* }}} */

/* ignore restrict if compiler doesn't support it {{{ */
#if defined(__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#   define restrict __restrict
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#   define restrict __restrict
#else
#   define restrict
#endif
/* }}} */

/* cf. https://gist.github.com/maedoc/f22f691c1ee22fe1961d#prelude {{{ */
#define SD_AS(obj, type) (((obj)->type)(obj))

#define SD_CALL_AS(obj, type, meth, ...) \
	SD_AS(obj, type)->meth(SD_AS(obj, type), __VA_ARGS__)

#define SD_CALL_AS_(obj, type, meth) \
	SD_AS(obj, type)->meth(SD_AS(obj, type))
/* }}} */


/* contains actual C declarations of API */
#include "sddekit_api.h"

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif

/* vim: foldmethod=marker
 */
