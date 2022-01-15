#include <QtWidgets/QApplication>
#include <base/logger/logger.h>
#include <module_db/database.h>
#include <base/encoder.h>
#include "app_ctrl.h"
#include <string>
#include <stdint.h>
#include <module_file/file/file.h>
using namespace std;
using namespace file;
int app(int argc, char* argv[])
{
	QApplication a(argc, argv);
	AppCtrl App;
	bool bRes=App.init();
	if (false == bRes)
	{
		return a.exec();
	}
	App.start();
	return a.exec();
}

bool func()
{
	LOG_ERROR << "1";
	return false;
}
union IP
{
	sockaddr s1;
	sockaddr_in v4;
	sockaddr_in6 v6;
};
void test()
{
	IP ip = { 0 };

	ip.v4.sin_family = AF_INET;
	
	inet_pton(AF_INET, "192.168.50.69", &ip.v4.sin_addr);		//设定新连接的IP
	ip.v4.sin_port = htons(11451);							//设定新连接的端口
	//LOG_ERROR << sizeof(IP);
}

int main(int argc, char* argv[])
{
	return app(argc, argv);
	//test();
	return 0;
}