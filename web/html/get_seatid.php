<?php
session_start();
//获取剩余座位号
function get_seatid($train, $dstation, $astation, $date, $type){
    $dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
    $filename = "sql/select_seat.sql"; $handle = fopen($filename, "r"); $contents = fread($handle, filesize ($filename));

    $sql_a = array($train,$dstation, $astation, $date,$type);
    $result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
    $line = pg_fetch_array($result, null, PGSQL_ASSOC);
    $seatid = $line['seatid'];
    echo $seatid;
}


$status = (int)$_GET['status'];
if($status ==0){
    get_seatid($_SESSION["train"],$_SESSION["dstation"],$_SESSION["astation"],$_SESSION["date"],$_GET["q"]);
}elseif($status ==1){
    get_seatid($_SESSION["train1"],$_SESSION["dstation1"],$_SESSION["astation1"],$_SESSION["date1"],$_GET["q"]);
}else{
    get_seatid($_SESSION["train2"],$_SESSION["dstation2"],$_SESSION["astation2"],$_SESSION["date2"],$_GET["q"]);
} 
?>