#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "UserProfile.h"
#include "Calculations.h"
#include "EEPROMHandler.h"
#include "Validation.h"

class UserManager {
public:
    UserManager();
    void loadProfileFromEEPROM();
    uint8_t getTrainingZone(uint16_t current_hr);
    uint16_t calculateCalories(uint32_t duration, uint16_t avg_hr, float avg_vo2);
    bool isHRRealistic(uint16_t hr);

private:
    UserProfile userProfile;
    EEPROMHandler eepromHandler;
    Validation validation;
};

#endif // USER_MANAGER_H
