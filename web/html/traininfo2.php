<?php

$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")
    or die('Could not connect: ' . pg_last_error());
$query = "SELECT SNAME
FROM TRAIN_STATION
WHERE TRAIN = $1
AND STOPNUM = 1;
";
$sql_a = array($_GET["train"]);
$result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());
$line = pg_fetch_array($result, null, PGSQL_ASSOC);
$startstation = $line['sname'];
$filename = "sql/train.sql";
$handle = fopen($filename, "r");
$contents = fread($handle, filesize ($filename));
$sql_a = array($_GET["train"],$_GET["left-date"]);
$result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
fclose($handle);
// Printing results in HTML
$p_array = ['硬座','软座','硬卧上','硬卧中','硬卧下','软卧上','软卧下'];
echo "始发站：".$startstation;
echo '<table class="table table-striped">';
echo "\t<tr>\n";
echo "\t\t<td>车次</td>\n";
echo "\t\t<td>到站</td>\n";
echo "\t\t<td>id</td>\n";
echo "\t\t<td>到达时</td>\n";
echo "\t\t<td>出发时</td>\n";
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
echo "\t</tr>\n";
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    $count = 0;
    foreach ($line as $col_value) {
        if($count > 11 and $col_value > 0){
        echo '<td><a href="bookconfirm.php?';
        echo 'train='.$line['train'];
        echo '&dstation='.$startstation;
        echo '&astation='.$line['sname'];
        echo '&date='.$_GET["left-date"];
        echo '&tickettype='.($count - 12);
        echo '">';
        echo $col_value;
        echo '</a></td>';
        }else{
            echo "\t\t<td>$col_value</td>\n";
        }
        $count += 1;
    }
}
echo "</table>\n";
?>