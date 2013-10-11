#!/bin/bash
# 功能: Shell实现跳板机
# 2013/10/8 guangxyou

# 用户名/密码文件, 格式 用户名:密码
passwd_file=./data/user_shell

function red() 
{
	# 显示红色
	echo -e "\033[031;40m$*\033[0m\n"
}

function green()
{
	# 显示绿色
	echo -e "\033[032;40m$*\033[0m\n"
}

#忽略Ctrl+c等所有信号
for sig in `seq 1 64`
do
	trap $sig &> /dev/null
done

clear

#关闭回显,输入密码
function getchar()
{
	stty cbreak -echo
	dd if=/dev/tty bs=1 count=1 2>/dev/null
	stty -cbreak echo
}

#登录部分
while :
do
	read -p "输入用户名: " username
	echo -n "请输入密码: "
	while :
	do
		ret=$(getchar)
		if [ "$ret" == "" ];then
			echo
			break
		fi
		passwd="$passwd$ret"
		echo -n '*'
	done
	right_passwd=$(awk -F: "/^$username:/{ print \$2 }" $passwd_file)
	if [ -z "$username" -o -z "$passwd" ];then
		red "用户名和密码不能为空"
		continue;
	fi
	if [ "$passwd" != "$right_passwd" ];then
		red "用户名或者密码错误,请重新输入"
	else
		red "登录成功."
		break;
	fi
done

#执行命令
logfile=./data/log_shell
function run()
{
	local command="$@"
	{
		echo -n -e "\033[32;40m$username\033[0m\033[7G --"
		echo -n -e "\033[31;40m`date '+%Y-%m-%d %H:%M:%S'` --\033[0m$command"
		echo
	} >>$logfile
	$command
}

host=`hostname`
cd ~
while :
do
	read -e -p "$username@$host:`pwd`$ "
	cmd=$REPLY
	run "$cmd"
done
