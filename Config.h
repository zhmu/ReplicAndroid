#pragma once

#include <vector>
#include <string>

struct NumbersAvailable {
	unsigned int totalNumberOfItems{};
	size_t totalNumberOfBytes{};
};

struct ItemsUpdate {
	unsigned int itemsTransferredSuccessfully{};
	unsigned int itemsTransferredFailures{};
	unsigned int itemsTransferredSkipped{};
	size_t bytesRead{};
	size_t bytesSkipped{};
};

struct FailedItem {
	std::string destPath;
};

struct WhatItem {
	std::vector<std::string> path;
};

struct Configuration
{
	std::vector<WhatItem> what;
	std::string where;
};
