#pragma once

#include "CommonMessage.h"
#include "SenderType.h"
#include <memory>
#include "timeTrans.h"
class LCManager;

namespace LCCommandHandler {

// 통합 명령 처리 진입점
void handleCommand(SenderType sender, const Common::CommonMessage& msg, LCManager& manager);

// 개별 핸들러(직접 호출 금지, 테스트용으로만 사용)
void handleECCCommand(const Common::CommonMessage& msg, LCManager& manager);
void handleMFRCommand(const Common::CommonMessage& msg, LCManager& manager);
void handleLSCommand(const Common::CommonMessage& msg, LCManager& manager);

}