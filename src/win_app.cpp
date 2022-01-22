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



int main(int argc, char* argv[])
{
	return app(argc, argv);
	//test();
	return 0;
}