from socket import *
def main():
    # 1.创建tcp_client_socket 套接字对象
    tcp_client_socket = socket(AF_INET,SOCK_STREAM)
    # 作为客户端，主动连接服务器较多，一般不需要绑定端口

    # 2.连接服务器
    tcp_client_socket.connect(("127.0.0.1",54572))
    while True:
        """无限循环可以实现无限聊天"""
        # 3.向服务器发送数据
        #      代码             市场         类型       日期格式:"2000-01-02 22:10:10"(日期和时间中间隔一个空格,日期用"-"隔开,时间用":"隔开,前置"0"都可以不用,因为会被转化为整数使用)
        """
        #define PER_MIN5		0		//5分钟数据
        #define PER_MIN15		1		//15分钟数据
        #define PER_MIN30		2		//30分钟数据
        #define PER_HOUR		3		//1小时数据
        #define PER_DAY			4		//日线数据
        #define PER_WEEK		5		//周线数据
        #define PER_MONTH		6		//月线数据
        #define PER_MIN1		7		//1分钟数据
        #define PER_MINN		8		//多分析数据(10)
        #define PER_DAYN		9		//多天线数据(45)
        #define PER_SEASON		10		//季线数据
        #define PER_YEAR		11		//年线数据
        #define PER_SEC5		12		//5秒线
        #define PER_SECN		13		//多秒线(15)
        #define PER_PRD_DIY0	14		//DIY周期
        #define PER_PRD_DIY10	24		//DIY周期
        
        #define REPORT_DAT2		102	//行情数据(第二版)
        #define GBINFO_DAT		103	//股本信息
        #define	STKINFO_DAT		105	//股票相关数据
        
        #define TPPRICE_DAT		121		//涨跌停数据
        """
        if(True):
            #mid 发送请求命令长度
            meg = "Code:600000\r\n Market:1\r\n Type:4\r\n From:2019-12-02 22:10:10\r\n To:2020-01-02 22:10:10\r\n".encode()
            count = len(meg)
            count_str = str(count)
            count_str = count_str.ljust(512,'\0').encode()        
            tcp_client_socket.send(count_str)  
        
        if(True):
            #mid 发送请求命令
            tcp_client_socket.send(meg) 
        # 在linux中默认是utf-8编码
        # 在udp协议中使用的sendto() 因为udp发送的为数据报，包括ip port和数据，
        # 所以sendto()中需要传入address，而tcp为面向连接，再发送消息之前就已经连接上了目标主机

        # 4.接收服务器返回的消息
        #recv_data = tcp_client_socket.recv(1024)  # 此处与udp不同，客户端已经知道消息来自哪台服务器，不需要用recvfrom了

        data = tcp_client_socket.recv(512)    
        if not data: 
            break
        total_data.append(data)
    
    
        if recv_data:
            print("返回的消息为:",recv_data.decode('gbk'))
        else:
            print("对方已离线。。")
            break

    tcp_client_socket.close()


if __name__ == '__main__':
    main()