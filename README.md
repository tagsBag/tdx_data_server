# tdx_data_server

通达信可以写c++函数,和选股 dll扩展

其中选股dll中可以调用 tdx 提供 的 查询历史数据的回调函数

测试了两天,tdx查询服务,获取数据可以返回历史数据个数,但是不返回具体的值
估计是取消了这个功能

1.如何测试 动态 加载的dll
	1.下载 debugview,这个软件是个人写的,但是收录在微软官网中
		网址:https://docs.microsoft.com/en-us/sysinternals/downloads/debugview
		下载地址:https://download.sysinternals.com/files/DebugView.zip
		可以在系统级捕获     OutputDebugString(line) 输出的字符串,很方便调试程序
		但是在 vs 中debug 时无法捕获,不知为何
		
2.tdx 如何 加载 指标选股dll
	1.编译后将 dll 放在 tdx 安装目录下的 plugin 目录下
	2.打开tdx,选择指标选股,既可以看到 该 dll
	
3.如何测试
	tdx加载好dll后,会输出隔 port,通过127.0.0.1:port可以连接,之后使用 tcp_client.py 中的测试代码可以测试
	
4.关于此工程
	1.Plugin.dll
		tdx 指标选股扩展,在此dll加载过程中会加载 DataServer.dll
	2.DataServer.dll
		该dll实现了一个 tcp select 方式的 server,接受 tcp 客户端数据请求
	3.tdx 加载 plugin.dll 时会传入一个 tdx 的回调函数,此函数的以向 tdx 查询数据
		plugin.dll 加载 DataServer.dll 时 会将此回调传入 DataServer.dll
		DataServer.dll 通过 tcp 连接 接收 客户端请求,解析后向 tdx 回调函数请求数据,然后转发给客户端
		
5.Tdx_mock.exe
	是一个 模拟 tdx 加载 dll 的桩程序,实现一个 同 tdx 一样的回调函数定义,用于加载dll 并测试其功能
	
6.该程序用 vs2017,win10 toolkit 编译通过,并运行正确
