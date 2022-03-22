<!DOCTYPE html> 
<html> 
<head> 
<meta charset="utf-8"> 
<title>菜鸟教程(runoob.com)</title> 
<script>
function showSite(str)
{
    if (str=="")
    {
        document.getElementById("txtHint").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('txtHint').innerHTML=xmlhttp.responseText;
            document.getElementById('seatid').value=xmlhttp.responseText;
            getprice(str);
        }
    }
    xmlhttp.open("GET","get_seatid.php?q="+str+"&status=0",true);
    xmlhttp.send();
}
function getprice(str)
{
    if (str=="")
    {
        document.getElementById("priceHint").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('priceHint').innerHTML=xmlhttp.responseText;
            document.getElementById('price').value=xmlhttp.responseText;
        }
    }
    xmlhttp.open("GET","get_price.php?q="+str+"&status=0",true);
    xmlhttp.send();
}
function showSite1(str)
{
    if (str=="")
    {
        document.getElementById("txtHint1").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('txtHint1').innerHTML=xmlhttp.responseText;
            document.getElementById('seatid1').value=xmlhttp.responseText;
            getprice1(str);
        }
    }
    xmlhttp.open("GET","get_seatid.php?q="+str+"&status=1",true);
    xmlhttp.send();
}
function getprice1(str)
{
    if (str=="")
    {
        document.getElementById("priceHint1").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('priceHint1').innerHTML=xmlhttp.responseText;
            document.getElementById('price1').value=xmlhttp.responseText;
        }
    }
    xmlhttp.open("GET","get_price.php?q="+str+"&status=1",true);
    xmlhttp.send();
}
function showSite2(str)
{
    if (str=="")
    {
        document.getElementById("txtHint2").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('txtHint2').innerHTML=xmlhttp.responseText;
            document.getElementById('seatid2').value=xmlhttp.responseText;
            getprice2(str);
        }
    }
    xmlhttp.open("GET","get_seatid.php?q="+str+"&status=2",true);
    xmlhttp.send();
}
function getprice2(str)
{
    if (str=="")
    {
        document.getElementById("priceHint2").innerHTML="";
        return;
    } 
    if (window.XMLHttpRequest)
    {
        // IE7+, Firefox, Chrome, Opera, Safari 浏览器执行代码
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        // IE6, IE5 浏览器执行代码
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            document.getElementById('priceHint2').innerHTML=xmlhttp.responseText;
            document.getElementById('price2').value=xmlhttp.responseText;
        }
    }
    xmlhttp.open("GET","get_price.php?q="+str+"&status=2",true);
    xmlhttp.send();
}


</script>
</head>
<body>
<?php 
function getprice($tickettype, $train, $dstation, $astation){
    $filename = "sql/get_price.sql"; $handle = fopen($filename, "r"); $contents = fread($handle, filesize ($filename));
    $type_array=array('硬座'=>'HP','软座'=>'SP','硬卧上'=>'HST','硬卧中'=>'HSM','硬卧下'=>'HSB','软卧上'=>'SST','软卧下'=>'SSB');
    $contents=str_replace('$4',$type_array[$tickettype],$contents);
    $sql_a = array($train, $dstation, $astation);
    $result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
    $line = pg_fetch_array($result, null, PGSQL_ASSOC);
    return $line['price'];    
}

//获取剩余座位种类
function get_type($train, $dstation, $astation, $date){
    $p_array = ['硬座','软座','硬卧上','硬卧中','硬卧下','软卧上','软卧下'];
    $tickettype = $p_array[$_GET["tickettype"]];
    $filename = "sql/select_type.sql";
    $handle = fopen($filename, "r");
    $contents = fread($handle, filesize ($filename));
    $sql_a = array($train, $dstation, $astation, $date);
    $result = pg_query_params($contents, $sql_a) or die('Query failed: ' . pg_last_error());
    $type_array = [];
    while ($line = pg_fetch_array($result, null, PGSQL_ASSOC)) {
        foreach ($line as $col_value) {
            $price = getprice($col_value,$train, $dstation, $astation);
            if($price != null)
                array_push($type_array,$col_value);
        }
    }
    return $type_array;
}

function print_option($type_array){
    if (in_array('硬座',$type_array))
        echo '<option value="硬座">硬座</option>';
    if (in_array('软座',$type_array))
        echo '<option value="软座">软座</option>';
    if (in_array('硬卧上',$type_array))
        echo '<option value="硬卧上">硬卧上</option>';
    if (in_array('硬卧中',$type_array))
        echo '<option value="硬卧中">硬卧中</option>';
    if (in_array('硬卧下',$type_array))
        echo '<option value="硬卧下">硬卧下</option>';
    if (in_array('软卧上',$type_array))
        echo '<option value="软卧上">软卧上</option>';
    if (in_array('软卧下',$type_array))
        echo '<option value="软卧下">软卧下</option>';
}
$dbconn = pg_connect("dbname=ticket user=dbms password=dbms")or die('Could not connect: ' . pg_last_error());
session_start();
if(isset($_GET['train'])) {
$_SESSION['train'] = $_GET["train"];
$_SESSION['dstation'] = $_GET["dstation"];
$_SESSION['astation'] = $_GET["astation"];
$_SESSION['date'] = $_GET["date"];
$_SESSION['booktype'] = 1;

echo $_GET["train"];
echo $_GET["dstation"];
echo $_GET["astation"];
echo $tickettype;
?>
<p>座位号：</p>
<div id="txtHint"><b>座位号显示在这里……</b></div>
<p>价格：</p>
<div id="priceHint"><b>价格信息显示在这里……</b></div>
<form action="book.php?status=1" method='POST'>

<?php

$type_array = get_type($_SESSION['train'], $_SESSION['dstation'], $_SESSION['astation'], $_SESSION['date']);
echo '<select name="tickettype" id="tickettype" onchange="showSite(this.value)">';
print_option($type_array);
echo '</select>';
} else{
    $_SESSION['train1'] = $_GET["train1"];
    $_SESSION['dstation1'] = $_GET["dstation1"];
    $_SESSION['astation1'] = $_GET["astation1"];
    $_SESSION['date1'] = $_GET["date1"];
    $_SESSION['train2'] = $_GET["train2"];
    $_SESSION['dstation2'] = $_GET["dstation2"];
    $_SESSION['astation2'] = $_GET["astation2"];
    $_SESSION['date2'] = $_GET["date2"];
    $_SESSION['booktype'] = 2;
    

    echo $_GET["train1"];
    echo $_GET["dstation1"];
    echo $_GET["astation1"];
    echo $_GET["date1"];
?>
<form action="book.php" method='POST'>

<p>座位号：</p>
<div id="txtHint1"><b>座位号显示在这里……</b></div>
<p>价格：</p>
<div id="priceHint1"><b>价格信息显示在这里……</b></div>
<form action="book.php?status=2" method='POST'>

<?php
    $type_array1 = get_type($_SESSION['train1'], $_SESSION['dstation1'], $_SESSION['astation1'], $_SESSION['date1']);
    echo '<select name="tickettype1" id="tickettype1" onchange="showSite1(this.value)">';
    print_option($type_array1);
    echo '</select>';
    echo "<br>";
    echo $_GET["train2"];
    echo $_GET["dstation2"];
    echo $_GET["astation2"];
    echo $_GET["date2"];
?>
<p>座位号：</p>
<div id="txtHint2"><b>座位号显示在这里……</b></div>
<p>价格：</p>
<div id="priceHint2"><b>价格信息显示在这里……</b></div>

<?php
    $type_array2 = get_type($_SESSION['train2'], $_SESSION['dstation2'], $_SESSION['astation2'], $_SESSION['date2']);
    echo '<select name="tickettype2" id="tickettype2" onchange="showSite2(this.value)">';
    print_option($type_array2);
    echo '</select>';
}
?>
</select>
<input id="seatid" name="seatid" type='text' value="1" hidden>
<input id="seatid1" name="seatid1" type='text' value="1" hidden>
<input id="seatid2" name="seatid2" type='text' value="1" hidden>
<input id="price"  name="price" type='text' value="1" hidden>
<input id="price1" name="price1" type='text' value="1" hidden>
<input id="price2" name="price2" type='text' value="1" hidden>
<input type="submit" value="Submit">
</form>
<a href="ticket.php">return</a>
<script>
document.getElementById('tickettype').onchange();

</script>
</body>
</html>