#include "StepMotorController.hpp"

StepMotorController::StepMotorController()
{

}

void StepMotorController::runSpeedMode(int speed) 
{
    std::cout << "[runSpeedMode] Run Speed Mode" << std::endl;
}

void StepMotorController::runAngleMode(double targetMotorAngle) 
{
    std::cout << "[runAngleMode] Run Angle Mode, target Motor Angle: " << targetMotorAngle << std::endl;
}