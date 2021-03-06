--TEST--
Check for rpmdbinfo function on Name
--SKIPIF--
<?php if (!extension_loaded("rpminfo")) print "skip"; ?>
--FILE--
<?php 
var_dump(rpmdbinfo('doesntexistsinrpmdb'));
var_dump(rpmdbinfo('bash'));
var_dump(rpmaddtag(RPMTAG_INSTALLTIME));
var_dump(rpmaddtag(RPMTAG_BUILDTIME));
var_dump(rpmaddtag(RPMTAG_INSTALLTIME));
var_dump(rpmdbinfo('bash'));
?>
Done
--EXPECTF--
NULL
array(1) {
  [0]=>
  array(5) {
    ["Name"]=>
    string(4) "bash"
    ["Version"]=>
    string(%d) "%s"
    ["Release"]=>
    string(%d) "%s"
    ["Summary"]=>
    string(26) "The GNU Bourne Again shell"
    ["Arch"]=>
    string(%d) "%s"
  }
}
bool(true)
bool(true)
bool(false)
array(1) {
  [0]=>
  array(7) {
    ["Name"]=>
    string(4) "bash"
    ["Version"]=>
    string(%d) "%s"
    ["Release"]=>
    string(%d) "%s"
    ["Summary"]=>
    string(26) "The GNU Bourne Again shell"
    ["Buildtime"]=>
    int(%d)
    ["Installtime"]=>
    int(%d)
    ["Arch"]=>
    string(%d) "%s"
  }
}
Done
