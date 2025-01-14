#pragma once

#include <string>
#include <vector>

struct Relay {
	std::string code;	// waw
	std::string name;	// Warsaw (Poland)
	std::vector<std::string> IPv4s; // 192.168.0.1
	//std::vector<std::string> ports; // [27015, 27060]

	// Based on Outbound rules in Firewall Settings

	bool blocked = false;

};