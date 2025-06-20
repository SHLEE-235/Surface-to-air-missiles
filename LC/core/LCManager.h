#pragma once
#include "SystemStatus.h"
#include "CommonMessage.h"
#include "IStatusSender.h"
#include "IReceiver.h"
#include "SenderType.h"
#include "IReceiverCallback.h"
#include "TcpMFR.h"
#include <memory>
#include <string>
#include <mutex>
#include <functional>
#include <vector>
#include "SerialLS.h"
#include "timeTrans.h"
class LCManager : public IReceiverCallback {
private:
    SystemStatus status;
    mutable std::mutex statusMutex;
    std::shared_ptr<IStatusSender> consoleSender;
    std::shared_ptr<IStatusSender> mfrSender;
    //수정필요
    std::shared_ptr<SerialLS> serialLS;

public:
    // 실행
    void run();
    // void startRadarDebugLoop();
    void onLCPositionRequest();
    // void startLSCommandLoop();  
    
    // 수신 콜백
    void onRadarStatusReceived(const Common::RadarStatus& msg);
    void onRadarDetectionReceived(const Common::RadarDetection& msg);
    void onMessage(const Common::CommonMessage& msg) override;

    // 초기화
    void init(const std::string& configPath, const std::string& ip, int port);
    void initialize(const std::string& iniPath);

    // 메시지 처리
    void dispatch(const Common::CommonMessage& msg);
    void handleECCCommand(const Common::CommonMessage& msg);  // ✅ 추가
    void handleMFRCommand(const Common::CommonMessage& msg);  // ✅ 추가

    // 송신자 등록
    void setConsoleSender(std::shared_ptr<IStatusSender> sender);
    void setMFRSender(std::shared_ptr<IStatusSender> sender);

    // 상태 접근
    void withLockedStatus(std::function<void(SystemStatus&)> func);
    SystemStatus getStatusCopy() const;
    bool hasLSSender() const;
    void sendToLS(const std::vector<uint8_t>& packet);
    bool hasConsoleSender() const;
    void sendToConsole(const std::vector<uint8_t>& packet);


    // 상태 출력 및 전송
    void sendStatus();
    void printStatus(const SystemStatus& status);
    void startLSCommandLoop();
    // 상태 업데이트
    void updateStatus(const MFRStatus& mfr);
    void updateStatus(const LSStatus& ls);
    void updateStatus(const LCStatus& lc);
    void updateStatus(const std::vector<MissileStatus>& missiles);
    void updateStatus(const std::vector<TargetStatus>& targets);
    void updateCalTime(const unsigned long long& calTime);
    void sendLCPositionToMFR();
    void onLSStatusReceived(const Common::LSReport& ls);


    // 유틸
    long long squaredDistance(const Pos2D& a, const Pos2D& b);
    TargetStatus* findTargetById(std::vector<TargetStatus>& targets, unsigned int id);
    double calculateDetectionAngle(const Pos2D& from, const Pos2D& to);
    double LaunchAngleCalc();
    double DetectionAngleCalc();
    std::string getRadarCommandInput();

    // 전송 헬퍼 (선택)
    bool hasMFRSender() const;
    void sendToMFR(const std::vector<uint8_t>& packet);

    void startStatusPrintingLoop();


    void printStatus() const ;
};