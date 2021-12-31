<?php

require('settings.php');

$security = trim($_GET['s']) ;
$date = $_GET['d'] ;
$stime = $_GET['c'] ;
$temperature = $_GET['t'] ;
$humidity = $_GET['h'] ;

$nowdate = date("Y-m-d");
$nowtime = date("H:i:s");
// note, it will replace date/time sent by module with serverdate/time

$ipaddress = $_SERVER['REMOTE_ADDR'];

if($security==$code && isset($date) && isset($stime) && isset($temperature) && isset($humidity))
{
  $datename = substr($nowdate, 0, 10) ;
  $filename = $datename.$extension ;
  
  if(!file_exists($filedir.$filename)) 
    {  
    $createfile = fopen($filedir.$filename, "w") or die("Cannot create file.");
    fclose($filedir.$filename);
    }
  
  $values = $nowdate.$delimiter.$nowtime.$delimiter.$temperature.$delimiter.$humidity.$delimiter.$ipaddress."\n" ;
  $oldfile = file_get_contents($filedir.$filename) ;
  $newfile = $values.$oldfile ;
  
  file_put_contents($filedir.$filename, $newfile) ;
  
  echo("|".$nowtime."_".$nowdate) ;
  // the server answers back: current time_date with a leading | character
  // in a future version this could be used to adjust the device clock if time differs too much
}
  
else
{
echo("ERROR") ;
}

?>