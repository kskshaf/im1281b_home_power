#!/usr/bin/bash

Bat_VT_File="/tmp/aht30_ina226_data"
Home_Power_File="/tmp/im1281b_data"

if [ -f $Home_Power_File ]; then
    U=$(cat $Home_Power_File|grep U|cut -d'=' -f2)
    I=$(cat $Home_Power_File|grep I|cut -d'=' -f2)
    P=$(cat $Home_Power_File|grep 'P='|cut -d'=' -f2)
    PF=$(cat $Home_Power_File|grep PF|cut -d'=' -f2)
    T=$(cat $Home_Power_File|grep T|cut -d'=' -f2)
    F=$(cat $Home_Power_File|grep F|cut -d'=' -f2)
else
    Nn="None"
    U=$Nn
    I=$Nn
    P=$Nn
    PF=$Nn
    T=$Nn
    F=$Nn
fi

post_mail='your@mail.com'

date_time() {
    date "+%Y/%m/%d %H:%M:%S"
}

send_mail() {
    (echo -e "$2\n时间：`date_time`" | mail -s "$1" -r $post_mail $post_mail "$3")&
}

get_bat_sensor_data() {
    if [ -f $Bat_VT_File ]; then
        BAT_T=$(cat $Bat_VT_File|grep T|cut -d'=' -f2)
        BAT_V=$(cat $Bat_VT_File|grep V|cut -d'=' -f2)
    else
        BAT_T="None"
        BAT_V="None"
    fi
}

case $1 in

  # 电压监测
  1)
    send_mail "停电通知" "已断电\n\n电压=$U\n电流=$I\n功率=$P\n温度=$T\n" "your_extra@mail.com"
    ;;

  2)
    send_mail "电压过低" "电压过低\n\n电压=$U\n电流=$I\n功率=$P\n温度=$T\n" "your_extra@mail.com"
    ;;

  3)
    send_mail "电压过高" "电压过高\n\n电压=$U\n电流=$I\n功率=$P\n温度=$T\n" "your_extra@mail.com"
    ;;

  0)
    send_mail "电压恢复正常" "电压恢复正常\n\n电压=$U\n电流=$I\n功率=$P\n温度=$T\n" "your_extra@mail.com"
    ;;

  # 温度、功率监测
  4)
    send_mail "警告: 模块温度过高!!!!!!" "温度=$T\n功率=$P\n电压=$U\n电流=$I\n" "your_extra@mail.com"
    ;;

  5)
    send_mail "警告: 插座负荷过高!!!!!!" "温度=$T\n功率=$P\n电压=$U\n电流=$I\n" "your_extra@mail.com"
    ;;

  6)
    send_mail "温度或功率已恢复正常" "电压=$U\n电流=$I\n功率=$P\n温度=$T\n" "your_extra@mail.com"
    ;;

    # 错误: IM1281B
  10)
    send_mail "IM1281B: 与模块通信失败，请检查设备状态!"
    ;;
    # 错误: INA226
  11)
    send_mail "INA226: 与模块通信失败，请检查设备状态!"
    ;;
    # 错误: AHT30
  12)
    send_mail "AHT30: 与模块通信失败，请检查设备状态!"
    ;;

    # 电池电压
  20)
    get_bat_sensor_data
    send_mail "警告: 电池电压过低!!!!!!" "系统即将关机!!!\n电池电压: $BAT_V伏\n电池温度: $BAT_T度\n"
    ;;

  21)
    get_bat_sensor_data
    send_mail "警告: 电池电压过高!!!!!!" "充电电路可能已经损坏，请及时断开适配器!!!\n电池电压: $BAT_V伏\n电池温度: $BAT_T度\n" "your_extra@mail.com"
    ;;

  22)
    get_bat_sensor_data
    send_mail "警告: 电池已断开!!!" "电池电压: $BAT_V伏\n电池温度: $BAT_T度\n"
    ;;

  23)
    get_bat_sensor_data
    send_mail "电池电压已恢复正常" "电池电压: $BAT_V伏\n电池温度: $BAT_T度\n"
    ;;

    # 电池温度
  30)
    get_bat_sensor_data
    send_mail "警告: 电池端温度过高!!!!!!" "电池温度: $BAT_T度\n电池电压: $BAT_V伏\n"
    ;;

  31)
    get_bat_sensor_data
    send_mail "电池端温度已恢复正常" "电池温度: $BAT_T度\n电池电压: $BAT_V伏\n"
    ;;


  *)
    echo "Please Input Value"
    ;;
esac
