#pragma once
#include <string>
#include "Relay.h"

namespace Firewall {
	bool blockRelay(Relay& relay);
	bool unblockRelay(Relay& relay);
	std::string listOutboundFirewallRules() noexcept;

};

