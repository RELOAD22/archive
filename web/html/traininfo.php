<?php
/*
echo $_GET["dcity"];
echo $_GET["acity"];
echo $_GET["left-date"];
echo $_GET["left-time"];*/
// Connecting, selecting database
$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")
    or die('Could not connect: ' . pg_last_error());
  
// Performing SQL query
$query = 'SELECT * FROM passenger';
$result = pg_query($query) or die('Query failed: ' . pg_last_error());
$filename = "sql/5_traininfo.sql";
$handle = fopen($filename, "r");//读取二进制文件时，需要将第二个参数设置成'rb'
//通过filesize获得文件大小，将整个文件一下子读到一个字符串中
$contents = fread($handle, filesize ($filename));
$sql_a = array($_GET["dcity"],$_GET["acity"],$_GET["left-date"],$_GET["left-time"]);
$result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
fclose($handle);
// Printing results in HTML
$p_array = ['硬座','软座','硬卧上','硬卧中','硬卧下','软卧上','软卧下'];
echo '<table class="table table-striped">';
echo "\t<tr>\n";
echo "\t\t<td>车次</td>\n";
echo "\t\t<td>发站</td>\n";
echo "\t\t<td>id</td>\n";
echo "\t\t<td>到站</td>\n";
echo "\t\t<td>id</td>\n";
echo "\t\t<td>出发时</td>\n";
echo "\t\t<td>到达时</td>\n";
echo "\t\t<td>硬座价</td>\n";
echo "\t\t<td>软座价</td>\n";
echo "\t\t<td>硬卧上价</td>\n";
echo "\t\t<td>硬卧中价</td>\n";
echo "\t\t<td>硬卧下价</td>\n";
echo "\t\t<td>软卧上价</td>\n";
echo "\t\t<td>软卧下价</td>\n";
echo "\t\t<td>硬座余</td>\n";
echo "\t\t<td>软座余</td>\n";
echo "\t\t<td>硬卧上余</td>\n";
echo "\t\t<td>硬卧中余</td>\n";
echo "\t\t<td>硬卧下余</td>\n";
echo "\t\t<td>软卧上余</td>\n";
echo "\t\t<td>软卧下余</td>\n";
echo "\t\t<td>低价</td>\n";
echo "\t</tr>\n";
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    $count = 0;
    foreach ($line as $col_value) {
        if($count > 13 and $count < 21 and $col_value > 0){
        echo '<td><a href="bookconfirm.php?';
        echo 'train='.$line['tr'];
        echo '&dstation='.$line['sn1'];
        echo '&astation='.$line['sn2'];
        echo '&date='.$_GET["left-date"];
        echo '&tickettype='.($count - 14);
        echo '">';
        echo $col_value;
        echo '</a></td>';
        }else{
            echo "\t\t<td>$col_value</td>\n";
        }
        $count += 1;

    }
    echo "\t</tr>\n";    
}
echo "</table>\n";


$filename = "sql/5_train2info.sql";
$handle = fopen($filename, "r");//读取二进制文件时，需要将第二个参数设置成'rb'
//通过filesize获得文件大小，将整个文件一下子读到一个字符串中
$contents = fread($handle, filesize ($filename));
$sql_a = array($_GET["dcity"],$_GET["acity"]);
$contents=str_replace('$1',"'".(string)$_GET["dcity"]."'",$contents);
$contents=str_replace('$2', "'".(string)$_GET["acity"]."'",$contents);
$contents=str_replace('$3', "'".(string)$_GET["left-time"]."'",$contents);
$contents=str_replace('$4', "'".(string)$_GET["left-date"]."'",$contents);
$hour = (int)substr($_GET["left-time"], 0,2);
$minute = (int)substr($_GET["left-time"], 3,2);
$contents=str_replace('$5', $hour, $contents);
$contents=str_replace('$6', $minute, $contents);
$sql_array=preg_split("/;[\r\n]+/", $contents);    
pg_query($sql_array[0]);  
pg_query($sql_array[1]); 
pg_query($sql_array[2]);  
pg_query($sql_array[3]) or die('Query failed: ' . pg_last_error()); 
pg_query($sql_array[4]) or die('Query failed: ' . pg_last_error());        
pg_query($sql_array[5]) or die('Query failed: ' . pg_last_error());        
$result = pg_query($sql_array[6]) or die('Query failed: ' . pg_last_error());        
fclose($handle);
// Printing results in HTML
echo '<table class="table table-striped">';
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    foreach ($line as $col_value) {
        echo "\t\t<td>$col_value</td>\n";
    }
    echo '<td><a href="bookconfirm.php?';
    echo 'train1='.$line['tr1'];
    echo '&dstation1='.$line['sn1'];
    echo '&astation1='.$line['sn2'];
    echo '&date1='.$_GET["left-date"];
    echo '&train2='.$line['tr2'];
    echo '&dstation2='.$line['sn3'];
    echo '&astation2='.$line['sn4'];
    echo '&date2='.$_GET["left-date"];
    echo '">';
    echo "预订";
    echo '</a></td>';
    echo "\t</tr>\n";
}
echo "</table>\n";
// Free resultset
pg_free_result($result);

// Closing connection
pg_close($dbconn);
?>
