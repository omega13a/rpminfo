/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2018 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Remi Collet <remi@php.net>                                   |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include <rpm/rpmio.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>

#include "php_rpminfo.h"

ZEND_DECLARE_MODULE_GLOBALS(rpminfo)

/* True global resources - no need for thread safety here */
// static int le_rpminfo;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("rpminfo.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_rpminfo_globals, rpminfo_globals)
    STD_PHP_INI_ENTRY("rpminfo.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_rpminfo_globals, rpminfo_globals)
PHP_INI_END()
*/
/* }}} */

static rpmts rpminfo_getts(rpmVSFlags flags) {
	if (!RPMINFO_G(ts)) {
		RPMINFO_G(ts) = rpmtsCreate();
	}
	if (RPMINFO_G(ts)) {
		(void)rpmtsSetVSFlags(RPMINFO_G(ts), flags);
	}
	return RPMINFO_G(ts);
}

static void rpminfo_freets(void) {
	if (RPMINFO_G(ts)) {
		rpmtsFree(RPMINFO_G(ts));
		RPMINFO_G(ts) = NULL;
	}
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpminfo, 0, 0, 1)
	ZEND_ARG_INFO(0, path)
	ZEND_ARG_INFO(0, full)
ZEND_END_ARG_INFO()

/* {{{ proto array rpminfo(string path [, bool full])
   Retrieve information from a RPM file */
PHP_FUNCTION(rpminfo)
{
	char *path, *msg=NULL, *val;
	size_t len;
	zend_bool full = 0;
	FD_t f;
	int rc;
	Header h;
	HeaderIterator hi;
	rpmTagVal tag;
	rpmTagType type;
	rpmts ts = rpminfo_getts(_RPMVSF_NODIGESTS | _RPMVSF_NOSIGNATURES | RPMVSF_NOHDRCHK);

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "p|b", &path, &len, &full) == FAILURE) {
		return;
	}

	f = Fopen(path, "r");
	if (f) {
		rc = rpmReadPackageFile(ts, f, "rpminfo", &h);
		if (rc == RPMRC_OK || rc == RPMRC_NOKEY || rc == RPMRC_NOTTRUSTED) {

			array_init(return_value);
			hi = headerInitIterator(h);
			while ((tag=headerNextTag(hi)) != RPMTAG_NOT_FOUND) {
				switch (tag) {
					case RPMTAG_NAME:
					case RPMTAG_VERSION:
					case RPMTAG_RELEASE:
					case RPMTAG_EPOCH:
					case RPMTAG_ARCH:
						break;
					default:
						if (!full) {
							continue;
						}
				}

				type = rpmTagGetTagType(tag);
				switch (type) {
					case RPM_STRING_TYPE:
					case RPM_I18NSTRING_TYPE:
						val = headerGetString(h, tag);
						if (val) {
							add_assoc_string(return_value, rpmTagGetName(tag), headerGetAsString(h, tag));
						} else {
							add_assoc_null(return_value, rpmTagGetName(tag));
						}
						break;
					case RPM_CHAR_TYPE:
					case RPM_INT8_TYPE:
					case RPM_INT16_TYPE:
					case RPM_INT32_TYPE:
					case RPM_INT64_TYPE:
						add_assoc_long(return_value, rpmTagGetName(tag), (zend_long)headerGetNumber(h, tag));
						break;
					default:
						val = headerGetAsString(h, tag);
						if (val) {
							add_assoc_string(return_value, rpmTagGetName(tag), headerGetAsString(h, tag));
						} else {
							add_assoc_null(return_value, rpmTagGetName(tag));
						}
				}
			}
			if (full) {
				add_assoc_bool(return_value, "IsSource", headerIsSource(h));
			}
			if (h) {
				headerFree(h);
			}
			Fclose(f);
			return;

		} else if (rc == RPMRC_NOTFOUND) {
			php_error_docref(NULL, E_WARNING, "Can't read '%s': Argument is not a RPM file", path);

		} else if (rc == RPMRC_NOTFOUND) {
			php_error_docref(NULL, E_WARNING, "Can't read '%s': Error reading header from package", path);

		} else {
			php_error_docref(NULL, E_WARNING, "Can't read '%s': Unkown error", path);
		}

		Fclose(f);
	} else {
		php_error_docref(NULL, E_WARNING, "Can't open '%s': %s", path, Fstrerror(f));
	}

	RETURN_FALSE;
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpmvercmp, 0, 0, 2)
	ZEND_ARG_INFO(0, evr1)
	ZEND_ARG_INFO(0, evr2)
ZEND_END_ARG_INFO()

/* {{{ proto int rpmcmpver(string evr1, string evr2)
   Compare 2 RPM evr (epoch:version-release) strings */
PHP_FUNCTION(rpmvercmp)
{
	char *evr1, *evr2;
	size_t len1, len2;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &evr1, &len1, &evr2, &len2) == FAILURE) {
		return;
	}

	RETURN_LONG(rpmvercmp(evr1, evr2));
}
/* }}} */

/* {{{ php_rpminfo_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_rpminfo_init_globals(zend_rpminfo_globals *rpminfo_globals)
{
	rpminfo_globals->global_value = 0;
	rpminfo_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(rpminfo)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(rpminfo)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(rpminfo)
{
#if defined(COMPILE_DL_RPMINFO) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(rpminfo)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(rpminfo)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "rpminfo support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_RPMINFO_VERSION);
	php_info_print_table_row(2, "RPM library version", RPMVERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(rpminfo) /* {{{ */
{
#if defined(COMPILE_DL_SESSION) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	rpminfo_globals->ts = NULL;
}
/* }}} */

/* {{{ PHP_GSHUTDOWN_FUNCTION
*/
PHP_GSHUTDOWN_FUNCTION(rpminfo)
{
	rpminfo_freets();
}
/* }}} */

/* {{{ rpminfo_functions[]
 *
 * Every user visible function must have an entry in rpminfo_functions[].
 */
const zend_function_entry rpminfo_functions[] = {
	PHP_FE(rpminfo,           arginfo_rpminfo)
	PHP_FE(rpmvercmp,         arginfo_rpmvercmp)
	PHP_FE_END
};
/* }}} */

/* {{{ rpminfo_module_entry
 */
zend_module_entry rpminfo_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	"rpminfo",
	rpminfo_functions,
	PHP_MINIT(rpminfo),
	PHP_MSHUTDOWN(rpminfo),
	PHP_RINIT(rpminfo),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(rpminfo),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(rpminfo),
	PHP_RPMINFO_VERSION,
	PHP_MODULE_GLOBALS(rpminfo),
	PHP_GINIT(rpminfo),
	PHP_GSHUTDOWN(rpminfo),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_RPMINFO
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(rpminfo)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
