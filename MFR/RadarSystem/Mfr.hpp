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
#include <sstream>
#include <iomanip>


class Mfr : public IReceiver {
private:
    /// MFR(다기능 레이더)의 고유 ID
    const unsigned int mfrId = 101001;

    /// @brief  MFR 좌표
    const Pos3D mfrCoords = { 37.54811601, 126.99611663, 244.0};

    /// @brief MFR 모드 정보
    RadarMode mfrMode;

    /// @brief Motor 목표 속도
    const unsigned int motorTargetRPM = 1;

    /// @brief Motor 목표 각도
    double goalMotorAngle;

    /// @brief MFR 탐지 범위 (m)
    const long long limitDetectionRange = 100000;

    /// @brief MFR의 시야(FoV) 제한 각도
    const double limitedFoV = 15.0;

    /// @brief 목표 표적 ID
    unsigned int goalTargetId;

    /// @brief 발사통제기(LC) 좌표
    Pos3D lcCoords;

    /// @brief MFR의 상태 정보
    MfrStatus mfrStatus;

    /// @brief 모터 관리 Manager
    StepMotorController* stepMotorManager;
    
    /// @brief LC(발사통제기) 통신 매니저 - TCP/IP
    MfrLcCommManager* lcCommManager;
    
    /// @brief Simulator 통신 매니저 - UDP/IP
    MfrSimCommManager* simCommManager;

    /// @brief 모의 표적 관리 자료구조
    std::unordered_map<unsigned int, localMockSimData> mockTargets;

    /// @brief 모의 미사일 관리 자료구조
    std::unordered_map<unsigned int, localMockSimData> mockMissile;

    /// @brief 탐지된 모의 표적 관리 자료구조
    std::unordered_map<unsigned int, localMockSimData> detectedTargets;

    /// @brief 탐지된 모의 미사일 관리 자료구조
    std::unordered_map<unsigned int, localMockSimData> detectedMissile;

    /// @brief 탐지 알고리즘 스레드
    std::thread detectionThread;

    /// @brief 탐지 알고리즘 스레드 관리 변수
    std::atomic<bool> detectionThreadRunning{false};

    /// @brief Haversine Formula에 사용되는 상수(r)
    const double EARTH_RADIUS_M = 6'371'000.0; // 지구 반지름 (m)
    
    /// @brief 좌표 인코딩 및 디코딩에 사용되는 스케일링 상수
    const double SCALE = 1e8;

    bool motorRotationFlag = false;

public:
    /**
     * Mfr 생성자
     */
    Mfr();

    /**
     * Mfr 소멸자
     */
    ~Mfr();

    /**
     * Simulator 또는 LC(발사통제기) 통신 매니저로부터 수신한 메시지를 처리
     * 
     * @param packet 수신된 메시지 데이터. 명령어(1 byte)와 직렬화된 페이로드로 구성
     */
    void callBackData(const std::vector<char>& packet) override;
    
private:
    /**
     * @brief deg -> rad 변환 함수
     * @param deg 각도 정보
     * @return 라디안 값
     */
    double deg2rad(double deg);

    /**
     * @brief rad -> deg 변환 함수
     * @param rad 라디안 정보
     * @return 각도 값
     */
    double rad2deg(double rad);

    /**
     * @brief Pos3D 구조체를 EncodedPos3D 구조체로 변환
     * 
     * @param e EncodedPos3D 구조체를 디코딩하여 Pos3D 구조체로 변환
     * @return Pos3D 구조체
     */
    Pos3D decode(const EncodedPos3D& e);

    /**
     * @brief MFR의 상태 정보를 인코딩
     *
     * @param p Pos3D 구조체를 인코딩하여 EncodedPos3D 구조체로 변환
     * @return EncodedPos3D 구조체
     */
    EncodedPos3D encode(const Pos3D& p);

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
     * Haversine Formula 적용
     * @param mfrCoord 레이더 3차원 좌표
     * @param mockCoord 모의기 3차원 좌표
     * @return 거리 값 (long long)
     */
    double calcDistance(Pos3D mfrCoord, Pos3D mockCoord);

    /**
     * 두 좌표 간의 각도 값 계산
     * @param mfrCoord 레이더 3차원 좌표
     * @param mockCoord 모의기 3차원 좌표
     * @return 각도 값 (double, -180 ~ 180)
     */
    double calcBearing(Pos3D mfrCoord, Pos3D mockCoord);

    /**
     * 기준 각도(baseAngle)와 목표 각도(targetAngle) 간의 차이를 계산
     * 
     * @param baseAngle 기준 각도 (도 단위, -180 ~ 180)
     * @param targetAngle 목표 각도 (도 단위, -180 ~ 180)
     * @return 두 각도 간의 차이 (도 단위, -180 ~ 180)
     */
    double angleDiff(double baseAngle, double targetAngle);

    /**
     * 레이더 모의기 탐지 유무 판단 알고리즘
     */
    void mfrDetectionAlgo();

    /**
     * MockTarget을 목록에 추가
     *
     * @param target 추가할 MockTarget
     */
    void addMockTarget(const localMockSimData& target);
    
    /**
     * ID로 MockTarget을 검색
     *
     * @param id 해당 MockTarget의 고유 ID
     * @return 해당 ID를 가진 MockTarget 포인터 (없으면 nullptr)
     */
    localMockSimData* getMockTargetById(unsigned int id);

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
    void addMockMissile(const localMockSimData& missile);

    /**
     * ID로 MockMissile을 검색
     *
     * @param id 해당 MockMissile의 고유 ID
     * @return 해당 ID를 가진 MockMissile 포인터 (없으면 nullptr)
     */
    localMockSimData* getMockMissileById(unsigned int id);

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