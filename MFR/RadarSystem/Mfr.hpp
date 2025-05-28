#pragma once

#include "StepMotorController.hpp"
#include "MfrLcCommManager.hpp"
#include "MfrSimCommManager.hpp"
#include "IReceiver.hpp"
#include "PacketProtocol.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <chrono>


class Mfr : public IReceiver {
private:
    /// MFR(다기능 레이더)의 고유 ID
    const unsigned int mfrId = 101001;
    const Pos2D mfrCoords = { 3754811601, 12699611663 };
    RadarMode mfrMode;
    const unsigned int motorTargetRPM = 1;
    double goalMotorAngle;

    const long long limitDetectionRange = 100;
    const double limitedFoV = 30.0;
    unsigned int goalTargetId;
    
    Pos2D lcCoords;

    MfrStatus mfrStatus;

    StepMotorController* stepMotorManager;
    MfrLcCommManager* lcCommManager;
    MfrSimCommManager* simCommManager;
    
    std::shared_mutex mockTargetMutex;
    std::shared_mutex mockMissileMutex;
    std::shared_mutex detectedTargetMutex;
    std::shared_mutex detectedMissileMutex;

    std::unordered_map<unsigned int, SimData> mockTargets;
    std::unordered_map<unsigned int, SimData> mockMissile;
    std::unordered_map<unsigned int, SimData> detectedTargets;
    std::unordered_map<unsigned int, SimData> detectedMissile;

    std::thread detectionThread;
    std::atomic<bool> detectionThreadRunning{false};

public:
    /**
     * Mfr 생성자
     */
    Mfr();

    ~Mfr();
    /**
     * Simulator 또는 LC(발사통제기) 통신 매니저로부터 수신한 메시지를 처리
     * 
     * @param packet 수신된 메시지 데이터. 명령어(1 byte)와 직렬화된 페이로드로 구성
     */
    void callBackData(const std::vector<char>& packet) override;
    
private:
    /**
     * 전달된 구조체 T를 직렬화하여 바이트 벡터로 변환
     *
     * @tparam T 직렬화할 구조체 타입
     * @param status 직렬화 대상 구조체
     * @return 직렬화된 바이트 배열(std::vector<char>)
     */
    template <typename T>
    std::vector<char> serializePacketforSend(const T& status);

    /**
     * LC(발사통제기)로부터 초기 설정 데이터를 요청하는 패킷을 전송
     */
    void requestLcInitData();

    /**
    * 현재 MFR의 상태 정보를 LC로 전송
    */
    void sendMfrStatus();

    /**
     * 탐지된 표적 및 미사일 정보를 직렬화하여 패킷으로 생성
     *
     * @param targets 탐지된 표적 정보 목록
     * @param missiles 탐지된 미사일 정보 목록
     * @return 직렬화된 바이트 배열(std::vector<char>)
     */
    std::vector<char> serializeDetectionPacket(const std::vector<MfrToLcTargetInfo>& targets, const std::vector<MfrToLcMissileInfo>& missiles);

    /**
     * Lㅊ(발사통제기)로부터 수신한 ModeCahngeData 패킷을 파싱하고 모터 제어 Manager 호출
     *
     * @param packet 수신된 데이터 패킷. 명령어를 제외한 ModeCahngeData 구조체 직렬화된 형태
     */
    void parsingModeChangeData(const std::vector<char>& payload);

    /**
    * LC로부터 수신한 초기화 데이터 패킷을 파싱하여 내부 상태를 초기화
    *
    * @param payload 수신된 초기화 데이터 페이로드
    */
    void parsingLcInitData(const std::vector<char>& payload);

    /**
     * Simulator로부터 수신한 SimData 패킷을 파싱하고 내부 객체(MockTarget 등)를 갱신
     *
     * @param packet 수신된 데이터 패킷. 명령어를 제외한 SimData 구조체 직렬화된 형태
     */
    void parsingSimData(const std::vector<char>& packet);    

    /**
     * std::chrono::system_clock::time_point를 밀리초 단위의 epoch 시간으로 변환
     *
     * @param tp 변환할 시간 포인트
     * @return 1970년 1월 1일 기준 경과 시간 (밀리초)
     */
    unsigned long toEpochMillis(std::chrono::system_clock::time_point tp);
    
    /**
     * 탐지 알고리즘을 백그라운드 스레드에서 실행
     * 
     * 내부적으로 무한 루프를 실행하며 표적 탐지를 수행
     */
    void startDetectionAlgoThread();
    
    /**
    * 실행 중인 탐지 알고리즘 스레드를 종료
    */
    void stopDetectionAlgoThread();

    /**
     * 두 좌표 간의 거리 값 계산
     * 
     * @param x1 첫 번째 좌표의 x값
     * @param y1 첫 번째 좌표의 y값
     * @param x2 두 번째 좌표의 x값
     * @param y2 두 번째 좌표의 y값
     * @return 거리 값 (long long)
     */
    long long calcDistance(long long x1, long long y1, long long x2, long long y2);

    /**
     * 기준 좌표(x1, y1)로부터 목표 좌표(x2, y2)까지의 각도를 계산합니다.
     * 
     * @param x1 기준 x좌표
     * @param y1 기준 y좌표
     * @param x2 목표 x좌표
     * @param y2 목표 y좌표
     * @return 기준점으로부터 목표점까지의 각도 (도 단위, -180 ~ 180)
     */
    double calcAngleToTarget(long long x1, long long y1, long long x2, long long y2);
    
    /**
     * 입력받은 targetAngle이 현재 MFR의 FoV에 포함되는지 판별
     *
     * @param targetAngle 목표 대상의 상대 각도
     * @return 시야 내에 존재하면 true, 그렇지 않으면 false
     */
    bool isInFoV(double targetAngle);

    /**
     * 레이더 모의기 탐지 유무 판단 알고리즘
     */
    void mfrDetectionAlgo();

    /**
     * MockTarget을 목록에 추가
     *
     * @param target 추가할 MockTarget
     */
    void addMockTarget(const SimData& target);
    
    /**
     * ID로 MockTarget을 검색
     *
     * @param id 해당 MockTarget의 고유 ID
     * @return 해당 ID를 가진 MockTarget 포인터 (없으면 nullptr)
     */
    SimData* getMockTargetById(unsigned int id);

    /**
     * 지정된 ID의 MockTarget을 목록에서 제거
     *
     * @param id 제거할 MockTarget의 ID
     */
    void removeMockTargetById(unsigned int id);

    /**
     * 모든 MockTarget을 목록에서 제거
     */
    void clearMockTargets();

    /**
     * MockMissile을 목록에 추가
     *
     * @param target 추가할 MockMissile
     */
    void addMockMissile(const SimData& missile);

    /**
     * ID로 MockMissile을 검색
     *
     * @param id 해당 MockMissile의 고유 ID
     * @return 해당 ID를 가진 MockMissile 포인터 (없으면 nullptr)
     */
    SimData* getMockMissileById(unsigned int id);

    /**
     * 지정된 ID의 MockMissile을 목록에서 제거
     *
     * @param id 제거할 MockMissile의 ID
     */
    void removeMockMissileById(unsigned int id);

    /**
     * 모든 MockMissile을 목록에서 제거
     */
    void clearMockMissiles();
};