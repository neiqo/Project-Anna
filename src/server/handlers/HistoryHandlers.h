#pragma once
#include <WebServer.h>
#include "../../storage/HistoricalData.h"

void registerHistoryHandlers(WebServer &server, HistoricalData &history);
