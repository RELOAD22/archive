<?php 

		# 接收用户的登录信息
		$username = trim($_POST['username']);
		$password = trim($_POST['password']);
        // 判断提交的登录信息
        $dbconn = pg_connect("dbname=ticket user=dbms password=dbms")
        or die('Could not connect: ' . pg_last_error());
    
        $query = "SELECT USERNAME,PASSWORD FROM PASSENGER WHERE USERNAME = $1;";
        $result = pg_query_params($query,  array($username) );

        // Printing results in HTML
        echo pg_fetch_result($result, 0, 0);
        echo pg_fetch_result($result, 0, 1);

		if (($username == '') || ($password == '')) {
			// 若为空,视为未填写,提示错误,并3秒后返回登录界面
			header('refresh:3; url=register.html');
			echo "用户名或密码不能为空,系统将在3秒后跳转到注册界面,请重新填写注册信息!";
			exit;
		}  else {
			# 用户名和密码都正确
			$query = "INSERT INTO PASSENGER VALUES($1, $2, $3, $4, $5, $6);";
			if ($_POST['name'] == '')
				$name = NULL;
			else
				$name = $_POST['name'];
			if ($_POST['idnum'] == '')
				$idnum = NULL;
			else
				$idnum = $_POST['idnum'];
			if ($_POST['phonenum'] == '')
				$phonenum = NULL;
			else
				$phonenum = $_POST['phonenum'];
			if ($_POST['cardnum'] == '')
				$cardnum = NULL;
			else
				$cardnum = $_POST['cardnum'];
            $sql_a = [$username, $name, $idnum, $phonenum, $cardnum,$password];
            $result = pg_query_params($query,  $sql_a) or die('Query failed: ' . pg_last_error());

			// 处理完附加项后跳转到登录成功的首页
			header('refresh:1; url=login.html');
		}
 ?>