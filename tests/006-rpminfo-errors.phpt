--TEST--
Check for rpminfo function errors
--SKIPIF--
<?php if (!extension_loaded("rpminfo")) print "skip"; ?>
--FILE--
<?php 
echo "+ PHP Warnings\n";
var_dump(rpminfo(__DIR__ . "/missing.rpm"));
var_dump(rpminfo(__FILE__));

echo "\n+ PHP Warnings\n";
var_dump(rpminfo(__DIR__ . "/missing.rpm", true,  $error), 
	$error);
var_dump(rpminfo(__FILE__,                 false, $error), 
	$error);
var_dump(is_array(rpminfo(__DIR__ . "/bidon.rpm",   false, $error)), 
	$error);
?>
Done
--EXPECTF--
+ PHP Warnings

Warning: rpminfo(): Can't open '%s/tests/missing.rpm': No such file or directory in %s on line %d
NULL

Warning: rpminfo(): Can't read '%s/tests/006-rpminfo-errors.php': Argument is not a RPM file in %s on line %d
NULL

+ PHP Warnings
NULL
string(%d) "Can't open '%s/tests/missing.rpm': No such file or directory"
NULL
string(%d) "Can't read '%s/tests/006-rpminfo-errors.php': Argument is not a RPM file"
bool(true)
NULL
Done
