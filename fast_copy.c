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
static int source_fd, dest_fd;

struct io_data {
	int read;
	off_t first_offset, offset;
	size_t first_len;
	struct iovec iov;
};

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

int get_file_size(int fd, off_t *size) {
	struct stat st;
	if (fstat(fd, &st) < 0) {
		return -1;
	}
	if (S_ISBLK(st.st_mode)) {
		unsigned long long bytes;
		if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
			return -1;
		}
		*size = bytes;
		return 0;
	} else if (S_ISREG(st.st_mode)) {
		*size = st.st_size;
		return 0;
	}
	return -1;
}

static void queue_prepped(struct io_uring *ring, struct io_data *data) {
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(ring);

	if (data->read)
		io_uring_prep_readv(sqe, source_fd, &data->iov, 1, data->offset);
	else
		io_uring_prep_writev(sqe, dest_fd, &data->iov, 1, data->offset);

	io_uring_sqe_set_data(sqe, data);
}


static int queue_read(struct io_uring *ring, off_t size, off_t offset) {
	struct io_uring_sqe *sqe;
	struct io_data *data;

	data = malloc(size + sizeof(*data));
	if (!data)
		return 1;
	
	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		free(data);
		return 1;
	}

	data->read = 1;
	data->offset = data->first_offset = offset;

	data->iov.iov_base = data + 1;
	data->iov.iov_len = size;
	data->first_len = size;

	io_uring_prep_readv(sqe, source_fd, &data->iov, 1, offset);
	io_uring_sqe_set_data(sqe, data);
	return 0;
}

static void queue_write(struct io_uring *ring, struct io_data *data) {
	data->read = 0;
	data->offset = data->first_offset;

	data->iov.iov_base = data + 1;
	data->iov.iov_len = data->first_len;

	queue_prepped(ring, data);
	io_uring_submit(ring);
}

static int copy_file(struct io_uring *ring, off_t insize) {
	unsigned long reads, writes;
	struct io_uring_cqe *cqe;
	off_t write_left, offset;
	int ret;

	write_left = insize;
	writes = reads = offset = 0;
	
	while (insize || write_left) {
		unsigned long had_reads;
		int got_comp;

		had_reads = reads;
		while (insize) {
			off_t this_size = insize;
			
			if (reads + writes >= DEPTH)
				break;
			if (this_size > BUF_SIZE) {
				this_size = BUF_SIZE;
			} else if (!this_size) {
				break;
			}

			if (queue_read(ring, this_size, offset))
				break;

			insize -= this_size;
			offset += this_size;
			reads++;
		}

		if (had_reads != reads) {
			ret = io_uring_submit(ring);
			if (ret < 0) {
				fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
				break;
			}
		}

		got_comp = 0;
		while (write_left) {
			struct io_data *data;

			if (!got_comp) {
				ret = io_uring_wait_cqe(ring, &cqe);
				got_comp = 1;
			} else {
				ret = io_uring_peek_cqe(ring, &cqe);
				if (ret == -EAGAIN) {
					cqe = NULL;
					ret = 0;
				}
			}
			if (ret < 0) {
				fprintf(stderr, "io_uring_peek_cqe: %s\n",
							strerror(-ret));
				return 1;
			}
			if (!cqe)
				break;

			data = io_uring_cqe_get_data(cqe);
			if (cqe->res < 0) {
				if (cqe->res == -EAGAIN) {
					queue_prepped(ring, data);
					io_uring_submit(ring);
					io_uring_cqe_seen(ring, cqe);
					continue;
				}
				fprintf(stderr, "cqe failed: %s\n",
						strerror(-cqe->res));
				return 1;
			} else if ((size_t)cqe->res != data->iov.iov_len) {
				/* Short read/write, adjust and requeue */
				data->iov.iov_base += cqe->res;
				data->iov.iov_len -= cqe->res;
				data->offset += cqe->res;
				queue_prepped(ring, data);
				io_uring_submit(ring);
				io_uring_cqe_seen(ring, cqe);
				continue;
			}

			/*
			 * All done. if write, nothing else to do. if read,
			 * queue up corresponding write.
			 */
			if (data->read) {
				queue_write(ring, data);
				write_left -= data->first_len;
				reads--;
				writes++;
			} else {
				free(data);
				writes--;
			}
			io_uring_cqe_seen(ring, cqe);
		}
	}

	/**wait for write*/
	while (writes) {
		struct io_data *data;

		ret = io_uring_wait_cqe(ring, &cqe);
		if (ret) {
			fprintf(stderr, "wait_cqe=%d\n", ret);
			return 1;
		}
		if (cqe->res < 0) {
			fprintf(stderr, "write res=%d\n", cqe->res);
			return 1;
		}
		data = io_uring_cqe_get_data(cqe);
		free(data);
		writes--;
		io_uring_cqe_seen(ring, cqe);
	}

	return 0;
}


/* {{{ string fast_copy( [ string $src, string $dest ] ) */
PHP_FUNCTION(fast_copy)
{
    char *src;
	char *dest;
    size_t src_len = 0;
	size_t dest_len = 0;
    zend_long result = 0L;
	off_t insize;
	int ret;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(src, src_len)
        Z_PARAM_STRING(dest, dest_len)
    ZEND_PARSE_PARAMETERS_END();

    const char *source_file = src;
	const char *dest_file = dest;

	source_fd = open(source_file, O_RDONLY);
	if (source_fd == -1) {
		//perror("open source file error!");
		result = -1L;
	}

	dest_fd = open(dest_file, O_WRONLY| O_CREAT| O_TRUNC, 0644);
	if (dest_fd == -1) {
		//perror("open dest file error!");
		result = -1L;
	}

	struct io_uring ring;
	if (io_uring_queue_init(DEPTH, &ring, 0) < 0) {
		//perror("io_uring init error!");
		result = -1L;
	}

	if (get_file_size(source_fd, &insize))
		result = -1L;
	ret = copy_file(&ring, insize);
	result = ret;
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
