/* fast_copy extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_fast_copy.h"
#include "fast_copy_arginfo.h"
#include <sys/ioctl.h>
#include <liburing.h>

#define BUF_SIZE (32*1024)
#define DEPTH 64	//depth determin the capacity of the ring
unsigned long count = 0;

off_t get_file_size(int fd) {
	struct stat st;
	if (fstat(fd, &st) < 0) {
		return -1;
	}
	if (S_ISBLK(st.st_mode)) {
		unsigned long long bytes;
		if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
			return -1;
		}
		return bytes;
	} else if (S_ISREG(st.st_mode)) {
		return st.st_size;
	}
	return -1;
}

void cqe_recv(struct io_uring *ring, struct io_uring_cqe *cqe, int ret) {
	while (ret) {
		int res = io_uring_wait_cqe(ring, &cqe);
		//printf("cqe res = %lu, %s\n", cqe->res, cqe->user_data);
		//release mem
		struct io_data *data = io_uring_cqe_get_data(cqe);
		data = NULL;
		
		io_uring_cqe_seen(ring, cqe);
		ret--;
	}
}


/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

/* {{{ void test1() */
PHP_FUNCTION(test1)
{
	ZEND_PARSE_PARAMETERS_NONE();

	php_printf("The extension %s is loaded and working!\r\n", "fast_copy");
}
/* }}} */

/* {{{ string test2( [ string $var ] ) */
PHP_FUNCTION(test2)
{
	char *var = "World";
	size_t var_len = sizeof("World") - 1;
	zend_string *retval;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING(var, var_len)
	ZEND_PARSE_PARAMETERS_END();

	retval = strpprintf(0, "Hello %s", var);

	RETURN_STR(retval);
}
/* }}}*/


/* {{{ string fast_copy( [ string $src, string $dest ] ) */
PHP_FUNCTION(fast_copy)
{
    char *src;
	char *dest;
    size_t src_len = 0;
	size_t dest_len = 0;
    zend_long result = 0L;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(src, src_len)
        Z_PARAM_STRING(dest, dest_len)
    ZEND_PARSE_PARAMETERS_END();

    const char *source_file = src;
	const char *dest_file = dest;
	struct io_uring_cqe *cqe;

	int source_fd = open(source_file, O_RDONLY);
	if (source_fd == -1) {
		//perror("open source file error!");
		result = -1L;
	}

	int dest_fd = open(dest_file, O_WRONLY| O_CREAT| O_TRUNC, 0644);
	if (dest_fd == -1) {
		//perror("open dest file error!");
		result = -1L;
	}

	struct io_uring ring;
	if (io_uring_queue_init(DEPTH, &ring, 0) < 0) {
		//perror("io_uring init error!");
		result = -1L;
	}

	off_t file_sz = get_file_size(source_fd);
	//printf("size:= %lu", file_sz);
	off_t offset = 0;
	off_t insize = file_sz;
	int ret, ret2, j;
	unsigned long i = 1;
	unsigned long flag = 0;
	char *buffer = malloc(sizeof(char)*BUF_SIZE);
	//memset(buffer, 0, BUF_SIZE);

	while (1) {
		off_t this_size;
		int depth;
		this_size = BUF_SIZE;
		if (this_size > insize) {
			this_size = insize;
		}
		
		struct io_uring_sqe *sqe;

		sqe = io_uring_get_sqe(&ring);
		io_uring_prep_read(sqe, source_fd, buffer, this_size, offset);
		//sqe->flags |= IOSQE_IO_LINK;
		io_uring_sqe_set_data(sqe, buffer);

		sqe = io_uring_get_sqe(&ring);
		io_uring_prep_write(sqe, dest_fd, buffer, this_size, offset);
		io_uring_sqe_set_data(sqe, buffer);
		flag += 2;
		count += flag;
		//printf("%d, %lu, %lu, %lu\n", flag, count, insize, this_size);
		
		if ((this_size == insize) || flag >= DEPTH) {		//where queue count equals DEPTH, then submit and read queue
			ret = io_uring_submit(&ring);
			flag = 0;
			//printf("submit ret = %d\n", ret);
			cqe_recv(&ring, cqe, ret);
		}
		
        insize -= this_size;
		offset += this_size;

		//printf("offset = %lu, file_sz = %lu\n", offset, file_sz);
		if (offset >= file_sz)
			break;
	}

	free(buffer);
	close(source_fd);
	close(dest_fd);
	io_uring_queue_exit(&ring);
	
    RETURN_LONG(result);
}
/* }}}*/


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(fast_copy)
{
#if defined(ZTS) && defined(COMPILE_DL_FAST_COPY)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(fast_copy)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "fast_copy support(based on io_uring)", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ fast_copy_module_entry */
zend_module_entry fast_copy_module_entry = {
	STANDARD_MODULE_HEADER,
	"fast_copy",					/* Extension name */
	ext_functions,					/* zend_function_entry */
	NULL,							/* PHP_MINIT - Module initialization */
	NULL,							/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(fast_copy),			/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(fast_copy),			/* PHP_MINFO - Module info */
	PHP_FAST_COPY_VERSION,		/* Version */
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FAST_COPY
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fast_copy)
#endif
