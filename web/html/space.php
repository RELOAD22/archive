<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<!-- 新 Bootstrap 核心 CSS 文件 -->
<link href="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet">
 
<!-- jQuery文件。务必在bootstrap.min.js 之前引入 -->
<script src="https://cdn.staticfile.org/jquery/2.1.1/jquery.min.js"></script>
 
<!-- 最新的 Bootstrap 核心 JavaScript 文件 -->
<script src="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/js/bootstrap.min.js"></script>
<title>个人中心</title>
<?php
	// 开启Session
	session_start();
    $user = $_SESSION['username'];
    echo "<script> var user=\"$user\";</script>";
?>
<script>
function showorders(begin_date, end_date)
{
  var xmlhttp;    
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
      document.getElementById("txtHint").innerHTML=xmlhttp.responseText;
    }
  }
  xmlhttp.open("GET","getorders.php?user="+user+"&begin-date="+begin_date+"&end-date="+end_date,true);
  xmlhttp.send();
}
</script>
</head>
<body>
<?php 

 
	// 首先判断Cookie是否有记住了用户信息
	if (isset($_COOKIE['username'])) {
		# 若记住了用户信息,则直接传给Session
		$_SESSION['username'] = $_COOKIE['username'];
		$_SESSION['islogin'] = 1;
	}
	if (isset($_SESSION['islogin'])) {
		// 若已经登录
		echo "你好! ".$_SESSION['username'].' ,欢迎!<br>';
		echo "<a href='logout.php'>注销</a>";
	} else {
		// 若没有登录
		echo "您还没有登录,请<a href='login.html'>登录</a>";
	}
?>
<a href="ticket.php">返回订票</a>
<form action="" method="get">

起：<input name="begindate" id="begindate" type="date" value="2019-11-01"/>
终：<input name="enddate" id="enddate" type="date" value="2019-12-01"/>
</form>
<button onclick="showorders(begindate.value,enddate.value)">提交</button>



<br>
<div id="txtHint">订单信息将显示在这...</div>


</body>
</html>