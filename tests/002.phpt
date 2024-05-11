--TEST--
test1() Basic test
--EXTENSIONS--
fast_copy
--FILE--
<?php
$ret = test1();

var_dump($ret);
?>
--EXPECT--
The extension fast_copy is loaded and working!
NULL
