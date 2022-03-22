<?php
session_start();
//获取价格
function get_price($train, $dstation, $astation){
    $dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
    $filename = "sql/get_price.sql"; $handle = fopen($filename, "r"); $contents = fread($handle, filesize ($filename));
    $type_array=array('硬座'=>'HP','软座'=>'SP','硬卧上'=>'HST','硬卧中'=>'HSM','硬卧下'=>'HSB','软卧上'=>'SST','软卧下'=>'SSB');
    $contents=str_replace('$4',$type_array[$_GET["q"]],$contents);
    $sql_a = array($train, $dstation, $astation);
    $result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
    $line = pg_fetch_array($result, null, PGSQL_ASSOC);
    $price = $line['price'];
    echo $price;
}

$status = (int)$_GET['status'];
if($status == 0){
    get_price($_SESSION["train"],$_SESSION["dstation"],$_SESSION["astation"]);
}else if($status == 1){
    get_price($_SESSION["train1"],$_SESSION["dstation1"],$_SESSION["astation1"]);
}else{
    get_price($_SESSION["train2"],$_SESSION["dstation2"],$_SESSION["astation2"]);
}
?>