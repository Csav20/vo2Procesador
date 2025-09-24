#include "UserManager.h"

UserManager::UserManager() {
    loadProfileFromEEPROM();
}

void UserManager::loadProfileFromEEPROM() {
    if (!eepromHandler.load(userProfile)) {
        // Handle default profile setup
    }
}

uint8_t UserManager::getTrainingZone(uint16_t current_hr) {
    return Calculations::calculateTrainingZone(userProfile, current_hr);
}

uint16_t UserManager::calculateCalories(uint32_t duration, uint16_t avg_hr, float avg_vo2) {
    return Calculations::calculateCalories(userProfile, duration, avg_hr, avg_vo2);
}

bool UserManager::isHRRealistic(uint16_t hr) {
    return validation.isHRWithinRange(hr, userProfile);
}