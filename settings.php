<?php

// 8 hex digits security code, must match the code from the Arduino device (working example here)
$code = '8a7b6c5d' ;

$projectname = 'Lcd-Meter' ;
$copyright = 'Daniel Krug 2021 - <a href=https://github.com/DKR-HIPS/lcdmeter>Project on github</a>' ;

// directory for saved data files (ends with '/' but not prefixed)
$filedir = 'files/' ;
// file extension to filter for, typically this is '.csv'
$extension = '.csv' ;
// delimiter to separate data columns, typically this is ';' or ','
$delimiter = ';' ;

$rowcolor[0]='#f8f8f8';
$rowcolor[1]='white';

?>