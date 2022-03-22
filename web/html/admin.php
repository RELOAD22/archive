<html>
<head>
<meta charset="utf-8">
<!-- 新 Bootstrap 核心 CSS 文件 -->
<link href="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet">
 
<!-- jQuery文件。务必在bootstrap.min.js 之前引入 -->
<script src="https://cdn.staticfile.org/jquery/2.1.1/jquery.min.js"></script>
 
<!-- 最新的 Bootstrap 核心 JavaScript 文件 -->
<script src="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/js/bootstrap.min.js"></script>
<title>ADMIN</title>
</head>
<body>
<?php
$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());

$query = "SELECT count(*) from orders;";
$result = pg_query($query);
$line = pg_fetch_array($result, null, PGSQL_ASSOC);
$ordercount = (int)$line['count'];
echo '<div class="container">';
echo '<div class="row" >';
echo '订单总数为：'.$ordercount.'<br>';
echo '</div>';
echo '</div>';
$query = "SELECT 
SUM(硬座  ) AS 硬座  ,
SUM(软座  ) AS 软座  ,
SUM(硬卧上) AS 硬卧上,
SUM(硬卧中) AS 硬卧中,
SUM(硬卧下) AS 硬卧下,
SUM(软卧上) AS 软卧上,
SUM(软卧下) AS 软卧下
FROM
(SELECT 
    SUM(CASE SE.TICKETTYPE WHEN '硬座'   THEN (TS2.HP  -TS1.HP ) ELSE 0 END) AS 硬座  ,
    SUM(CASE SE.TICKETTYPE WHEN '软座'   THEN (TS2.SP  -TS1.SP ) ELSE 0 END) AS 软座  ,
    SUM(CASE SE.TICKETTYPE WHEN '硬卧上' THEN (TS2.HST -TS1.HST) ELSE 0 END) AS 硬卧上,
    SUM(CASE SE.TICKETTYPE WHEN '硬卧中' THEN (TS2.HSM -TS1.HSM) ELSE 0 END) AS 硬卧中,
    SUM(CASE SE.TICKETTYPE WHEN '硬卧下' THEN (TS2.HSB -TS1.HSB) ELSE 0 END) AS 硬卧下,
    SUM(CASE SE.TICKETTYPE WHEN '软卧上' THEN (TS2.SST -TS1.SST) ELSE 0 END) AS 软卧上,
    SUM(CASE SE.TICKETTYPE WHEN '软卧下' THEN (TS2.SSB -TS1.SSB) ELSE 0 END) AS 软卧下
FROM TICKET AS ST, TRAIN_STATION AS TS1, TRAIN_STATION AS TS2, SEAT AS SE
WHERE 
        ST.TRAIN = TS1.TRAIN AND ST.TRAIN = TS2.TRAIN
    AND TS1.SNAME = ST.DSNAME AND TS2.SNAME = ST.ASNAME
    AND ST.SEATID = SE.SEATID
GROUP BY SE.TICKETTYPE) AS RESULT
;";
$result = pg_query($query) or die('Query failed: ' . pg_last_error());
echo '<div class="row" >';
echo '各类票价汇总<br>';
echo '<div class="col-md-3">';
echo '<table class="table table-striped">';
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    foreach ($line as $col_value) {
        echo "\t\t<td>$col_value</td>\n";
    }
    echo "\t</tr>\n";
}
echo "</table>\n";
echo '</div>';
echo '</div>';
echo "<br>最热车次：<br>";
$query = "SELECT TRAIN, COUNT(*) AS HOTNUM
FROM TICKET
GROUP BY TRAIN
ORDER BY COUNT(*) DESC LIMIT 10
;";
$result = pg_query($query);
echo '<div class="row" >';
echo '<div class="col-md-3">';
echo '<table class="table table-striped">';
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    foreach ($line as $col_value) {
        echo "\t\t<td>$col_value</td>\n";
    }
    echo "\t</tr>\n";
}
echo "</table>\n";
echo '</div>';
echo '</div>';
echo '<div class="row" >';
echo '<div class="col-md-3">';
echo "注册用户：<br>";
$query = "SELECT USERNAME, NAME, IDNUM, PHONENUM, CARDNUM
FROM PASSENGER
;";
$result = pg_query($query);
echo '<table class="table table-striped">';
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    foreach ($line as $col_value) {
        echo "\t\t<td>$col_value</td>\n";
    }
    echo '<td><a href="getorders.php?';
    echo 'user='.$line['username'];
    echo "&begin-date=2000-01-01";
    echo "&end-date=2100-01-01";
    echo '">';
    echo "查看";
    echo '</a></td>';
    echo "\t</tr>\n";
}
echo "</table>\n";
echo '</div>';
echo '</div>';

?>

</body>
</html>


