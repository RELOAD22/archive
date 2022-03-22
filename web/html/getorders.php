<?php
    $user = $_GET['user'];
    $begin_date = $_GET['begin-date'];
    $end_date = $_GET['end-date'];
    $query="
    SELECT O.ORDERID, BOOK_DATE, T.TICKET_dATE, TRAIN, T.DSNAME, T.ASNAME, T.SEATID, TICKETTYPE, O.PRICE, STATUS
    FROM  ORDERS AS O, PASSENGER AS P, TICKET AS T, SEAT AS S
    WHERE 
        P.USERNAME = $1
        AND T.USERNAME = P.USERNAME
        AND T.ORDERID = O.ORDERID 
        AND T.SEATID = S.SEATID
        AND T.TICKET_DATE >= to_date($2, 'yyyy-MM-dd')
        AND T.TICKET_DATE < to_date($3, 'yyyy-MM-dd')
    ORDER BY BOOK_DATE
    ;";
$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
$sql_a = array($user, $begin_date, $end_date);
$result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());
echo '<div class="row" >';
echo '<div class="col-md-6">';
echo '<table class="table table-striped">';
$oldid = '0';
while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
    echo "\t<tr>\n";
    $newid = $line['orderid'];
    if($newid != $oldid)
        echo "\t\t<td>".$line['orderid']."</td>\n";
    else
        echo "\t\t<td></td>\n";
    if($newid != $oldid)
        echo "\t\t<td>".substr($line['book_date'],0,19)."</td>\n";
    else
        echo "\t\t<td></td>\n";
    echo "\t\t<td>".substr($line['ticket_date'],0,10)."</td>\n";
    echo "\t\t<td>".$line['train']."</td>\n";
    echo "\t\t<td>".$line['dsname']."</td>\n";
    echo "\t\t<td>".$line['asname']."</td>\n";
    echo "\t\t<td>".$line['seatid']."</td>\n";
    echo "\t\t<td>".$line['tickettype']."</td>\n";
    if($newid != $oldid)
        echo "\t\t<td>".$line['price']."</td>\n";
    else
        echo "\t\t<td></td>\n";
    if($newid != $oldid)
        echo "\t\t<td>".$line['status']."</td>\n";
    else
        echo "\t\t<td></td>\n";

    if($newid != $oldid)
        echo "\t\t<td><a href='cancelorder.php?orderid=$newid'>取消</a></td>\n";
    else
        echo "\t\t<td></td>\n";
    $oldid  =  $newid;
    echo "\t</tr>\n";
}
echo "</table>\n";
echo '</div>';
echo '</div>';
?>

