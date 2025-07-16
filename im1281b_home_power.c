#include "im1281b_home_power.h"
#include "data.h"
#include "exit.h"

// 发送查询数据
uint8_t data_send[8] = {0x01, 0x03, 0x00, 0x48, 0x00, 0x08, 0xC4, 0x1A};

/* 电压异常线程 */
pthread_t    abnormal_voltage_thread;
volatile int abnormal_voltage_thread_running = 0;
volatile int abnormal_voltage_keeping = 0;

void* abnormal_voltage_handler(void* arg) {
    pthread_detach(abnormal_voltage_thread);   // 线程分离
    log_info("启动电压异常线程");
    static int sys_res;

    // 用来锁定只需要执行一次的任务
    static int pwoff_task_lock = 0;
    static int lowV_task_lock  = 0;
    static int highV_task_lock = 0;

    while(abnormal_voltage_keeping != 0 && keep_running && !check_keep_file()) {
        switch (abnormal_voltage_keeping)
        {
            case 1:
                if(!pwoff_task_lock)
                {
                    log_info("当前电压为%.1fV, 判断为断电状态", IM1281B_OUT.U);
                    sys_res = system("/root/send_mail.sh 1");
                    if(sys_res != 0) log_error("邮件发送异常!");
                    write_pw_status_to_html(1);  // 断电：UPS支撑不了太久
                }
                pwoff_task_lock = 1;
                lowV_task_lock  = 0;
                highV_task_lock = 0;
                break;
            case 2:
                if(!lowV_task_lock)
                {
                    log_info("电压过低: %.1fV", IM1281B_OUT.U);
                    sys_res = system("/root/send_mail.sh 2");
                    if(sys_res != 0) log_error("邮件发送异常!");
                    write_pw_status_to_html(0);  // 缺电：开关电源仍然可以正常工作
                }
                lowV_task_lock  = 1;
                pwoff_task_lock = 0;
                highV_task_lock = 0;
                break;
            case 3:
                if(!highV_task_lock)
                {
                    log_info("电压过高: %.1fV", IM1281B_OUT.U);
                    sys_res = system("/root/send_mail.sh 3");
                    if(sys_res != 0) log_error("邮件发送异常!");
                    write_pw_status_to_html(1);  // 高压：不确定，判定为异常状态
                }
                highV_task_lock = 1;
                pwoff_task_lock = 0;
                lowV_task_lock  = 0;
                break;
            default:
                break;
        }
        usleep(1000*1000);
    }

    write_pw_status_to_html(0);
    if(keep_running && !check_keep_file())
    {
        sys_res = system("/root/send_mail.sh 0");
        if(sys_res != 0) log_error("邮件发送异常!");
    }

    pwoff_task_lock = 0;
    lowV_task_lock  = 0;
    highV_task_lock = 0;

    log_info("电压已恢复到正常范围, 退出电压异常线程");
    abnormal_voltage_thread_running = 0;
    pthread_exit(NULL);
}

/* 超负荷线程 */
pthread_t    power_overload_thread;
volatile int power_overload_thread_running = 0;
volatile int power_overload_keeping = 0;

void* power_overload_handler(void* arg) {
    pthread_detach(power_overload_thread);   // 线程分离
    log_info("启动超负荷线程");
    static int sys_res;
    static int pOver_task_lock = 0;

    while(power_overload_keeping != 0 && keep_running && !check_keep_file()) {

        if(!pOver_task_lock)
        {
            log_warn("警告：当前功率为%.1fW, 已超负荷!!!", IM1281B_OUT.P);
            sys_res = system("/root/send_mail.sh 5");
            if(sys_res != 0) log_error("邮件发送异常!");
        }
        pOver_task_lock = 1;
        usleep(1000*1000);
    }

    pOver_task_lock = 0;
    log_info("功率已恢复到正常范围, 退出超负荷线程");
    power_overload_thread_running = 0;

    if(keep_running && !check_keep_file())
    {
        sys_res = system("/root/send_mail.sh 6");
        if(sys_res != 0) log_error("邮件发送异常!");
    }
    pthread_exit(NULL);
}


/* 温度异常线程 */
pthread_t    overheat_thread;
volatile int overheat_thread_running = 0;
volatile int overheat_keeping = 0;

void* overheat_handler(void* arg) {
    pthread_detach(overheat_thread);   // 线程分离
    log_info("启动温度异常线程");
    static int sys_res;
    static int overH_task_lock = 0;

    while(overheat_keeping != 0 && keep_running && !check_keep_file()) {

        if(!overH_task_lock)
        {
            log_warn("警告：当前温度为%.1f°C!!!", IM1281B_OUT.T);
            sys_res = system("/root/send_mail.sh 4");
            if(sys_res != 0) log_error("邮件发送异常!");
        }
        overH_task_lock = 1;
        usleep(1000*1000);
    }

    overH_task_lock = 0;
    log_info("当前温度已恢复正常, 退出温度异常线程");
    overheat_thread_running = 0;

    if(keep_running && !check_keep_file())
    {
        sys_res = system("/root/send_mail.sh 6");
        if(sys_res != 0) log_error("邮件发送异常!");
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    // 设置 locale
    setlocale(LC_ALL, "zh_CN.UTF-8");

    // 创建 KEEP_FILE
    FILE *fp_keep = fopen(KEEP_FILE, "w");
    if(fp_keep == NULL)
    {
        log_error("创建 KEEP_FILE 失败!");
        return -1;
    }
    fclose(fp_keep);

    // 打开日志文件
    FILE *fp_log = fopen(LOG_FILE, "a");
    if(log_add_fp(fp_log, LOG_INFO) != 0)
    {
        log_error("打开日志文件失败!");
        return -1;
    }

    // 检查网页文件
    if (access(WEB_FILE, W_OK) != 0) {
        log_error("没有 %s 的写入权限", WEB_FILE);
        return -1; 
    }
    // 初始化网页文件
    write_pw_status_to_html(0);

    log_info("启动进程 %s", argv[0]);

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc < 2 || argc > 3) {
        printf("Usage: %s <port 1>\n", argv[0]);
        return -1;
    }
    char *port_name = argv[1];

    /* The ports we will use. */
    struct sp_port *port;

    /* Open and configure each port. */
    log_info("检查串口是否可用: %s", port_name);
    check(sp_get_port_by_name(port_name, &port));

    log_info("打开串口");
    check(sp_open(port, SP_MODE_READ_WRITE));

    log_info("串口参数: %d 8N1, no flow control", BaudRate);
    check(sp_set_baudrate(port, BaudRate));
    check(sp_set_bits(port, 8));
    check(sp_set_parity(port, SP_PARITY_NONE));
    check(sp_set_stopbits(port, 1));
    check(sp_set_rts(port, SP_RTS_OFF));
    check(sp_set_cts(port, SP_CTS_IGNORE));
    check(sp_set_dtr(port, SP_DTR_OFF));
    check(sp_set_dsr(port, SP_DSR_IGNORE));
    check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

    // 清屏并将光标移动到开头
    //printf("\033[2J\033[H");

    int size = sizeof(data_send);
    int result;
    int thread_res;
    int receive_size = 37;
    uint8_t  *buf = malloc(receive_size + 1);

    int count_0 = 0, count_1 = 0, count_2 = 0, count_3 = 0;
    int p_count = 0;

    while(keep_running && !check_keep_file())
    {
        result = check(sp_blocking_write(port, data_send, size, Timeout));
        if (result != size)
        {
            log_error("发送超时, %d/%d 字节已发送", result, size);

            // 关闭串口
            free(buf);
            check(sp_close(port));
            sp_free_port(port);

            // 发送邮件
            if(system("/root/send_mail.sh 10") != 0) log_error("邮件发送异常!");

            log_error("退出进程");
            return -1;
        }

        result = check(sp_blocking_read(port, buf, receive_size, Timeout));
        while (result != receive_size)
        {
            static int send_retry = 0;
            if(send_retry > 5) {
                log_error("重试超过5次, 退出进程");

                // 关闭串口
                free(buf);
                check(sp_close(port));
                sp_free_port(port);

                // 发送邮件
                if(system("/root/send_mail.sh 10") != 0) log_error("邮件发送异常!");

                log_error("退出进程");
                return -1;
            }

            error_to_data_file();  // 写入错误状态到数据文件
            log_warn("发送超时, %d/%d 字节已接收, 正在重试: %d", result, receive_size, send_retry+1);
            result = check(sp_blocking_read(port, buf, receive_size, Timeout));
            send_retry++;
        }


        /* Check if we received the same data we sent. */
        buf[result] = '\0';

        // https://stackoverflow.com/questions/30225423/send-hex-values-via-serial-port-in-c
        // https://github.com/tree-water/acac/blob/main/Core/Src/IM1281B.c
        uint8_t crc_non_data[receive_size - 2];
        uint8_t main_data[receive_size - 5];
        for(int j=0; j < (receive_size - 2); j++)
        {
            crc_non_data[j] = buf[j];
            if(j < (receive_size - 5)) main_data[j] = buf[j+3];
        }

        uint16_t data_crc = modbus_crc16(crc_non_data, sizeof(crc_non_data));
        if(combine_bytes(buf[receive_size - 1], buf[receive_size - 2]) != data_crc)
        {
            log_warn("数据校验失败!");
            error_to_data_file();  // 写入错误状态到数据文件
            // for(int i = 0; i < receive_size; i++) {
            //     printf("%02X ", buf[i]);
            // }
        }
        else
        {
            get_module_data(main_data);

            // 模块温度过高
            if(IM1281B_OUT.T > Temp_HIGH) {
                overheat_keeping = 1;
                // 创建温度异常线程
                if(!overheat_thread_running)
                {
                    thread_res = pthread_create(&overheat_thread, NULL, overheat_handler, NULL);
                    if(thread_res != 0)
                    {
                        overheat_thread_running = 0;
                        log_error("创建温度异常线程失败: %d", thread_res);
                    }
                    else
                    {
                        overheat_thread_running = 1;
                        log_info("创建温度异常线程成功");
                    }
                }
            } else {
                overheat_keeping = 0;
            }

            // 延时计数判断是否超负荷
            if(IM1281B_OUT.P > Power_HIGH) {
                p_count++;
                if(p_count >= 2) {
                    power_overload_keeping = 1;
                    // 创建超负荷线程
                    if(!power_overload_thread_running)
                    {
                        thread_res = pthread_create(&power_overload_thread, NULL, power_overload_handler, NULL);
                        if(thread_res != 0)
                        {
                            power_overload_thread_running = 0;
                            log_error("创建超负荷线程失败: %d", thread_res);
                        }
                        else
                        {
                            power_overload_thread_running = 1;
                            log_info("创建超负荷线程成功");
                        }
                    }
                    p_count = 0;
                }
            } else {
                p_count++;
                if(p_count >= 2) {
                    power_overload_keeping = 0;
                    p_count = 0;
                }
            }

            // 延时计数判断电压状态
            if(IM1281B_OUT.U < Voltage_POF) {
                count_1++;
                if(count_1 >= 2) {
                    abnormal_voltage_keeping = 1;
                    count_0 = count_1 = count_2 = count_3 = 0;
                }
            } else if(IM1281B_OUT.U < Voltage_LOW && IM1281B_OUT.U > Voltage_POF) {
                count_2++;
                if(count_2 >= 2) {
                    abnormal_voltage_keeping = 2;
                    count_0 = count_1 = count_2 = count_3 = 0;
                }
            } else if(IM1281B_OUT.U > Voltage_HIGH) {
                count_3++;
                if(count_3 >= 2) {
                    abnormal_voltage_keeping = 3;
                    count_0 = count_1 = count_2 = count_3 = 0;
                }
            } else {
                count_0++;
                if(count_0 >= 2) {
                    abnormal_voltage_keeping = 0;
                    count_0 = count_1 = count_2 = count_3 = 0;
                }
            }

            // 判断电压状态 & 创建电压异常线程
            if(!abnormal_voltage_thread_running && abnormal_voltage_keeping != 0)
            {
                thread_res = pthread_create(&abnormal_voltage_thread, NULL, abnormal_voltage_handler, NULL);
                if(thread_res != 0)
                {
                    abnormal_voltage_thread_running = 0;
                    log_error("创建电压异常线程失败: %d", thread_res);
                }
                else
                {
                    abnormal_voltage_thread_running = 1;
                    log_info("创建电压异常线程成功");
                }
            }
        }
        //printf("keep_running: %d\n", keep_running);
        usleep(500*1000);
    }

    // 退出清空数据
    FILE *fp = fopen(DATA_FILE, "w");
    if (fp != NULL) {
        fprintf(fp, "NONE");
        fclose(fp);
    } else {
        log_error("打开 %s 失败", DATA_FILE);
    }

    /* Free receive buffer. */
    free(buf);

    /* Close ports and free resources. */
    check(sp_close(port));
    sp_free_port(port);

    log_info("~~~再见~~~");

    // 关闭日志
    fclose(fp_log);
    return 0;
}
