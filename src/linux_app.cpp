#include "tracker_ctrl.h"
#include <iostream>
int main()
{

	TrackerCtrl App;
	App.init();
	while (1)
	{
		std::string strCommand;
		std::cin >> strCommand;
		App.output(strCommand);
	}
}