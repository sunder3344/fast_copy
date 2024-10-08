# php_fast_copy
big file fast copy(base on io_uring), faster than the common copy method(php8, linux kernel >= 5.10).

## complile

- /your_php_installed_folder/bin/phpize
- go into this fast_copy file and run "./configure --with-php-config=/your_php_installed_folder/bin/php-config --with-fast_copy=/usr/include"   (please check file /usr/include/liburing.php exist)
- add your liburing path to LD_LIBRARY_PATH and LD_RUN_PATH in file ~/.bashrc.
  after editing, execute `source ~/.bashrc`
	```

		# Some more alias to avoid making mistakes:
		# alias rm='rm -i'
		# alias cp='cp -i'
		# alias mv='mv -i'
		
		alias ll='ls -l'
		
		export LD_LIBRARY_PATH=/usr/include/liburing
		export LD_RUN_PATH=/usr/include/liburing
	```
- make && make install
- add fast_josn.so to your php.ini like this: `extension=/usr/local/php-8.3.6/lib/php/extensions/no-debug-non-zts-20230831/fast_copy.so`

## how to use

open the demo.php, here is the code, it's very simple, fist parameter is source file, second parameter is destination file

```
	<?php
		fast_copy("/usr/local/nginx/logs/111.log", "/tmp/111.log");

```

## execute efficiency

more than `50% performance promotion than before`, [for more test detail](https://github.com/sunder3344/linux_kernel_magic/tree/main/io_uring)

![Alt Text](pic/compare.jpg)

we can find the io_uring embed in php run faster(nearly 100%) than golang function(fast.go)
