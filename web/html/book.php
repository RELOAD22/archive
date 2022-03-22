<?php 
session_start();
$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
function book($price, $seatid, $train, $dstation, $astation, $date){

    //获取订单号
    $query = "SELECT count(*) from orders;";
    $result = pg_query($query);
    $line = pg_fetch_array($result, null, PGSQL_ASSOC) or die('Query failed: ' . pg_last_error());
    $orderid = (int)$line['count'] + 1;
    //获取票号
    $query = "SELECT count(*) from ticket;";
    $result = pg_query($query);
    $line = pg_fetch_array($result, null, PGSQL_ASSOC) or die('Query failed: ' . pg_last_error());
    $ticketid = (int)$line['count'] + 1;
    echo '订单号'.$orderid.'<br>';
    echo '票号'.$ticketid.'<br>';
    echo '列车'.$train.'<br>';
    echo '出发站'.$dstation.'<br>';
    echo '到达站'.$astation.'<br>';
    echo '座位号'.$seatid.'<br>';
    echo '总票价'.$price.'<br>';
    //插入一张订单
    $query = "INSERT INTO ORDERS VALUES ($1, NOW(), $2, $3, '正常',$4);";
    $sql_a = array($orderid,$dstation,$astation,$price);
    $result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());

    //插入一张票
    $query = "INSERT INTO ticket VALUES ($1, $2, $3, $4, $5, $6, $7, $8);";
    $sql_a = array($ticketid,$date,$train,$dstation,$astation,$seatid, $_SESSION['username'], $orderid);
    $result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());
}
function book2($bookid, $price, $seatid, $train, $dstation, $astation, $date){
    //获取票号
    $query = "SELECT count(*) from ticket;";
    $result = pg_query($query);
    $line = pg_fetch_array($result, null, PGSQL_ASSOC) or die('Query failed: ' . pg_last_error());
    $ticketid = (int)$line['count'] + 1;

    if($bookid == 0){    
        //获取订单号
        $query = "SELECT count(*) from orders;";
        $result = pg_query($query);
        $line = pg_fetch_array($result, null, PGSQL_ASSOC) or die('Query failed: ' . pg_last_error());
        $orderid = (int)$line['count'] + 1;
        //插入一张订单
        $query = "INSERT INTO ORDERS VALUES ($1, NOW(), $2, $3, '正常',$4);";
        $sql_a = array($orderid,$dstation,$astation,$price);
        $result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());  
        echo '订单号'.$orderid.'<br>';
        echo '票号'.$ticketid.'<br>';
        echo '列车'.$train.'<br>';
        echo '出发站'.$dstation.'<br>';
        echo '到达站'.$astation.'<br>';
        echo '座位号'.$seatid.'<br>';           
    }else{
        $orderid = $bookid;
        echo '票号'.$ticketid.'<br>';
        echo '列车'.$train.'<br>';
        echo '出发站'.$dstation.'<br>';
        echo '到达站'.$astation.'<br>';
        echo '座位号'.$seatid.'<br>';
        echo '总票价'.$price.'<br>';
    }

    //插入一张票
    $query = "INSERT INTO ticket VALUES ($1, $2, $3, $4, $5, $6, $7, $8);";
    $sql_a = array($ticketid,$date,$train,$dstation,$astation,$seatid, $_SESSION['username'], $orderid);
    $result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error());
    return $orderid;
}
$status = (int)$_GET['status'];
if($status == 1)  {
    $price = (float)$_POST["price"]  + 5;
    book($price,$_POST["seatid"],$_SESSION["train"],$_SESSION["dstation"],$_SESSION["astation"],$_SESSION["date"]);
}else{
    echo "换乘订票";
    $bookid = 0;
    $price = (float)$_POST["price1"] +  (float)$_POST["price2"] + 10;
    $bookid = book2($bookid, $price,$_POST["seatid1"],$_SESSION["train1"],$_SESSION["dstation1"],$_SESSION["astation1"],$_SESSION["date1"]);
    book2($bookid, $price,$_POST["seatid2"],$_SESSION["train2"],$_SESSION["dstation2"],$_SESSION["astation2"],$_SESSION["date2"]); 
}
?>
订购成功

<a href="ticket.php">return</a>