<html>
 <head>
  <title>取消成功</title>
 </head>
 <body>
 <?php
    $orderid = $_GET['orderid'];
    $query = "UPDATE ORDERS SET STATUS='已取消' WHERE ORDERID=$1;";
    $sql_a = array($orderid);
    $dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
    $result = pg_query_params($query, $sql_a) or die('Query failed: ' . pg_last_error()); 
    echo '取消成功';
?>

<a href="space.php">返回个人中心</a>

 </body>
</html>
