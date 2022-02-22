#include "tracker_ctrl.h"
#include <iostream>
#include <base/logger/logger.h>
int main()
{
	uint16_t Port, Port6;
	LOG_INFO << "Input IPv4 port&IPv6 port:";
	std::cin >> Port >> Port6;
	TrackerCtrl App;
	App.init(Port, Port6);
	while (1)
	{
		std::string strCommand;
		std::cin >> strCommand;
		App.output(strCommand);
	}
}