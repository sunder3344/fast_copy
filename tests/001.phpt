--TEST--
Check if fast_copy is loaded
--EXTENSIONS--
fast_copy
--FILE--
<?php
echo 'The extension "fast_copy" is available';
?>
--EXPECT--
The extension "fast_copy" is available
