/* fast_copy extension for PHP */

#ifndef PHP_FAST_COPY_H
# define PHP_FAST_COPY_H

extern zend_module_entry fast_copy_module_entry;
# define phpext_fast_copy_ptr &fast_copy_module_entry

# define PHP_FAST_COPY_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_FAST_COPY)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_FAST_COPY_H */
