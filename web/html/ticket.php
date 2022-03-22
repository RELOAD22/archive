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
function showtrains(dcity, acity, left_date, left_time)
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

  xmlhttp.open("GET","traininfo.php?"+"dcity="+dcity+"&acity="+acity+"&left-date="+left_date+"&left-time="+left_time,true);
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
<a href="train.php">车次查询</a>
<form action="traininfo.php" method="get">
出发城市: <input type="text" name="dcity" id="dcity" value="北京">
到达城市: <input type="text" name="acity" id="acity" value="徐州"">
出发日期：<input name="left_date" id="left_date" type="date" value="2019-11-05"/>
出发时间：<input name="left_time" id="left_time" type="time" value="00:00"/>
</form>
<button onclick="showtrains(dcity.value, acity.value, left_date.value, left_time.value)">提交</button>
<button onclick="returntrain()">返程</button>
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
function returntrain(){
    var dcity = document.getElementById("dcity").value;
    document.getElementById("dcity").value=document.getElementById("acity").value;
    document.getElementById("acity").value=dcity;
    var left_date = document.getElementById("left_date").value;
    var dateConvert1 = new Date(Date.parse(left_date));
    dateConvert1=dateConvert1.setDate(dateConvert1.getDate()+1);
    dateConvert1=new Date(dateConvert1);
    var year = dateConvert1.getFullYear(); 
    var month =(dateConvert1.getMonth() + 1).toString(); 
    var day = (dateConvert1.getDate()).toString();  
    if (month.length == 1) { 
        month = "0" + month; 
    } 
    if (day.length == 1) { 
        day = "0" + day; 
    }
    var dateTime = year + "-" + month + "-" + day;
    document.getElementById("left_date").value = dateTime;
    document.getElementById("left_time").value = '00:00';
}
</script>
</body>
</html>