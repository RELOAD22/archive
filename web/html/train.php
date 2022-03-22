<html>
<head>
<meta charset="utf-8">
<!-- 新 Bootstrap 核心 CSS 文件 -->
<link href="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet">
 
<!-- jQuery文件。务必在bootstrap.min.js 之前引入 -->
<script src="https://cdn.staticfile.org/jquery/2.1.1/jquery.min.js"></script>
 
<!-- 最新的 Bootstrap 核心 JavaScript 文件 -->
<script src="https://cdn.staticfile.org/twitter-bootstrap/3.3.7/js/bootstrap.min.js"></script>
<title>订票</title>
<script>
function showtrains(train, left_date)
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

  xmlhttp.open("GET","traininfo2.php?"+"train="+train+"&left-date="+left_date,true);
  xmlhttp.send();
}
</script>
</head>
<body>
<?php 
	// 开启Session
	session_start();
 
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
<a href="space.php">个人中心</a>
<a href="ticket.php">返回订票</a>
<form action="traininfo.php" method="get">
查询车次: <input type="text" name="train"" id="train" value="1228">
出发日期：<input name="left_date" id="left_date" type="date" value="2019-11-05"/>
</form>
<button onclick="showtrains(train.value, left_date.value)">提交</button>
<br>
<div id="txtHint">车次信息将显示在这...</div>

<script>
function Getdefaultleftdate() {
    var now = new Date();
    var year = now.getFullYear();
    var month = ("0"+(now.getMonth()+1)).slice(-2);
    var date = ("0"+(now.getDate()+1)).slice(-2);
    return year+"-"+month+"-"+date;
} 
document.getElementById("left_date").value=Getdefaultleftdate();
</script>
</body>
</html>