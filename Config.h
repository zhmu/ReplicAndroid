/*-
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2022 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
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
