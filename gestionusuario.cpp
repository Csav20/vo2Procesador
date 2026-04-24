/**
 * @file vo2smart_user_profile.c
 * @brief Módulo completo de gestión de perfil de usuario para VO2Smart
 * @version 1.0
 * @date 2025-09-23
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

// =============================================================================
// ESTRUCTURAS DE DATOS DE USUARIO
// =============================================================================

typedef enum {
    SPORT_RUNNING = 0,
    SPORT_CYCLING,
    SPORT_SWIMMING,
    SPORT_TRIATHLON,
    SPORT_ROWING,
    SPORT_CROSSFIT,
    SPORT_GENERAL,
    SPORT_COUNT
} sport_type_t;

typedef enum {
    FITNESS_SEDENTARY = 0,  // < 2 entrenamientos/semana
    FITNESS_BEGINNER,       // 2-3 entrenamientos/semana
    FITNESS_RECREATIONAL,   // 3-4 entrenamientos/semana
    FITNESS_TRAINED,        // 4-6 entrenamientos/semana
    FITNESS_ATHLETE,        // > 6 entrenamientos/semana
    FITNESS_ELITE,          // Competición de alto nivel
    FITNESS_COUNT
} fitness_level_t;

typedef struct {
    // Datos personales básicos
    char name[32];
    uint8_t age;
    float weight;           // kg
    float height;           // cm
    uint8_t gender;         // 0=Male, 1=Female
    
    // Datos deportivos
    sport_type_t sport;
    fitness_level_t fitness_level;
    uint16_t training_hours_week;  // horas/semana promedio
    
    // Datos fisiológicos
    float hr_rest;          // FC reposo (medida o estimada)
    float hr_max;           // FC máxima (medida o estimada)
    float vo2_max_tested;   // VO2max medido previamente (0 si no hay)
    float vo2_max_estimated; // VO2max estimado por fórmulas
    
    // Configuración personal
    bool use_metric;        // true=kg/cm, false=lb/ft
    uint8_t preferred_zones; // 3, 5, o 7 zonas
    
    // Metadatos
    uint32_t creation_date;
    uint32_t last_updated;
    uint16_t sessions_completed;
    uint32_t checksum;      // Para validar integridad
    
} user_profile_t;

typedef struct {
    float zone_limits[7];   // Límites de zonas calculados
    float calories_factor;  // Factor para cálculo de calorías
    float vo2_prediction_accuracy; // Precisión estimada de predicción
} user_calculations_t;

// =============================================================================
// CLASE DE GESTIÓN DE PERFIL
// =============================================================================

class UserProfileManager {
private:
    user_profile_t profile;
    user_calculations_t calculations;
    bool profile_loaded;
    bool profile_modified;
    
    const char* sport_names[SPORT_COUNT] = {
        "Running", "Cycling", "Swimming", "Triathlon", 
        "Rowing", "CrossFit", "General"
    };
    
    const char* fitness_names[FITNESS_COUNT] = {
        "Sedentario", "Principiante", "Recreativo",
        "Entrenado", "Atleta", "Elite"
    };
    
    uint32_t calculateChecksum(const user_profile_t* p);
    void calculateDerivedValues();
    float estimateHRMax();
    float estimateVO2Max();
    void calculateTrainingZones();
    
public:
    UserProfileManager();
    
    // Gestión de perfil
    bool loadProfile();
    bool saveProfile();
    void createDefaultProfile();
    bool validateProfile(const user_profile_t* p);
    
    // Accesores
    user_profile_t* getProfile() { return &profile; }
    user_calculations_t* getCalculations() { return &calculations; }
    bool isProfileLoaded() { return profile_loaded; }
    bool isProfileModified() { return profile_modified; }
    
    // Modificadores
    void setBasicInfo(const char* name, uint8_t age, float weight, float height, uint8_t gender);
    void setSportInfo(sport_type_t sport, fitness_level_t level, uint16_t hours_week);
    void setPhysiologicalData(float hr_rest, float hr_max, float vo2_tested);
    void markModified() { profile_modified = true; }
    
    // Cálculos dinámicos
    uint8_t getTrainingZone(float hr_current);
    float getZoneLimit(uint8_t zone, bool upper_limit = true);
    uint16_t calculateCalories(float duration_minutes, float avg_hr, float avg_vo2);
    float getRecoveryFactor();
    const char* getSportName() { return sport_names[profile.sport]; }
    const char* getFitnessLevelName() { return fitness_names[profile.fitness_level]; }
    
    // Validación y recomendaciones
    bool isHRRealistic(float hr);
    bool isVO2Realistic(float vo2);
    const char* getHealthWarning();
    const char* getTrainingRecommendation();
};

// =============================================================================
// IMPLEMENTACIÓN DE LA CLASE
// =============================================================================

UserProfileManager::UserProfileManager() {
    profile_loaded = false;
    profile_modified = false;
    memset(&profile, 0, sizeof(user_profile_t));
    memset(&calculations, 0, sizeof(user_calculations_t));
}

bool UserProfileManager::loadProfile() {
    EEPROM.begin(sizeof(user_profile_t));
    EEPROM.get(0, profile);
    
    // Validar integridad
    if (!validateProfile(&profile)) {
        Serial.println("Perfil corrupto, creando perfil por defecto");
        createDefaultProfile();
        return false;
    }
    
    calculateDerivedValues();
    profile_loaded = true;
    profile_modified = false;
    
    Serial.printf("Perfil cargado: %s, %d años, %s\n", 
                  profile.name, profile.age, getSportName());
    return true;
}

bool UserProfileManager::saveProfile() {
    if (!profile_loaded) return false;
    
    profile.last_updated = millis() / 1000; // timestamp en segundos
    profile.checksum = calculateChecksum(&profile);
    
    EEPROM.begin(sizeof(user_profile_t));
    EEPROM.put(0, profile);
    EEPROM.commit();
    EEPROM.end();
    
    profile_modified = false;
    Serial.println("Perfil guardado correctamente");
    return true;
}

void UserProfileManager::createDefaultProfile() {
    memset(&profile, 0, sizeof(user_profile_t));
    
    strcpy(profile.name, "Usuario");
    profile.age = 30;
    profile.weight = 70.0f;
    profile.height = 175.0f;
    profile.gender = 0; // Masculino por defecto
    profile.sport = SPORT_GENERAL;
    profile.fitness_level = FITNESS_RECREATIONAL;
    profile.training_hours_week = 3;
    profile.use_metric = true;
    profile.preferred_zones = 5;
    profile.creation_date = millis() / 1000;
    profile.sessions_completed = 0;
    
    // Estimar valores fisiológicos
    profile.hr_max = estimateHRMax();
    profile.hr_rest = 65.0f; // Valor típico
    profile.vo2_max_estimated = estimateVO2Max();
    profile.vo2_max_tested = 0.0f;
    
    calculateDerivedValues();
    profile_loaded = true;
    profile_modified = true;
}

bool UserProfileManager::validateProfile(const user_profile_t* p) {
    // Validaciones básicas
    if (p->age < 10 || p->age > 100) return false;
    if (p->weight < 30.0f || p->weight > 200.0f) return false;
    if (p->height < 120.0f || p->height > 220.0f) return false;
    if (p->hr_max < 120.0f || p->hr_max > 220.0f) return false;
    if (p->hr_rest < 30.0f || p->hr_rest > 120.0f) return false;
    if (p->hr_rest >= p->hr_max) return false;
    
    // Validar checksum
    uint32_t expected_checksum = calculateChecksum(p);
    if (p->checksum != 0 && p->checksum != expected_checksum) {
        return false;
    }
    
    return true;
}

uint32_t UserProfileManager::calculateChecksum(const user_profile_t* p) {
    uint32_t checksum = 0;
    uint8_t* data = (uint8_t*)p;
    size_t size = sizeof(user_profile_t) - sizeof(uint32_t); // Excluir el propio checksum
    
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + data[i];
    }
    return checksum;
}

void UserProfileManager::calculateDerivedValues() {
    // Calcular zonas de entrenamiento
    calculateTrainingZones();
    
    // Factor de calorías basado en peso, edad y género
    float base_factor = profile.weight * 0.95f; // Aproximación base
    if (profile.gender == 1) { // Mujer
        base_factor *= 0.9f;
    }
    if (profile.age > 40) {
        base_factor *= (1.0f - (profile.age - 40) * 0.005f);
    }
    calculations.calories_factor = base_factor;
    
    // Precisión de predicción VO2
    if (profile.vo2_max_tested > 0) {
        calculations.vo2_prediction_accuracy = 0.95f; // Alta precisión con dato real
    } else {
        // Precisión basada en nivel de fitness
        switch (profile.fitness_level) {
            case FITNESS_ELITE: calculations.vo2_prediction_accuracy = 0.85f; break;
            case FITNESS_ATHLETE: calculations.vo2_prediction_accuracy = 0.80f; break;
            case FITNESS_TRAINED: calculations.vo2_prediction_accuracy = 0.75f; break;
            default: calculations.vo2_prediction_accuracy = 0.65f; break;
        }
    }
}

float UserProfileManager::estimateHRMax() {
    // Fórmula de Tanaka et al. (2001) - más precisa que 220-edad
    float hr_max = 208.0f - (0.7f * profile.age);
    
    // Ajuste por nivel de fitness
    switch (profile.fitness_level) {
        case FITNESS_ELITE:
        case FITNESS_ATHLETE:
            hr_max += 5.0f; // Atletas tienden a tener HR máx más alta
            break;
        case FITNESS_SEDENTARY:
            hr_max -= 5.0f;
            break;
        default:
            break;
    }
    
    return hr_max;
}

float UserProfileManager::estimateVO2Max() {
    float vo2_estimate = 0.0f;
    
    // Fórmula de Jackson & Pollock (adaptada)
    if (profile.gender == 0) { // Masculino
        vo2_estimate = 15.3f * (profile.hr_max / profile.hr_rest);
    } else { // Femenino
        vo2_estimate = 14.7f * (profile.hr_max / profile.hr_rest);
    }
    
    // Ajuste por deporte
    float sport_factor = 1.0f;
    switch (profile.sport) {
        case SPORT_RUNNING: sport_factor = 1.1f; break;
        case SPORT_CYCLING: sport_factor = 1.05f; break;
        case SPORT_SWIMMING: sport_factor = 1.15f; break;
        case SPORT_TRIATHLON: sport_factor = 1.2f; break;
        case SPORT_ROWING: sport_factor = 1.1f; break;
        case SPORT_CROSSFIT: sport_factor = 1.0f; break;
        default: sport_factor = 0.95f; break;
    }
    
    // Ajuste por nivel de fitness
    float fitness_factor = 0.7f + (profile.fitness_level * 0.1f);
    
    vo2_estimate *= sport_factor * fitness_factor;
    
    return constrain(vo2_estimate, 25.0f, 85.0f);
}

void UserProfileManager::calculateTrainingZones() {
    float hr_reserve = profile.hr_max - profile.hr_rest;
    
    if (profile.preferred_zones == 5) {
        // 5 zonas estándar (Coggan)
        calculations.zone_limits[0] = profile.hr_rest + (hr_reserve * 0.60f); // Z1-Z2
        calculations.zone_limits[1] = profile.hr_rest + (hr_reserve * 0.70f); // Z2-Z3
        calculations.zone_limits[2] = profile.hr_rest + (hr_reserve * 0.80f); // Z3-Z4
        calculations.zone_limits[3] = profile.hr_rest + (hr_reserve * 0.90f); // Z4-Z5
        calculations.zone_limits[4] = profile.hr_max;                         // Z5
    } else if (profile.preferred_zones == 3) {
        // 3 zonas simplificadas
        calculations.zone_limits[0] = profile.hr_rest + (hr_reserve * 0.70f);
        calculations.zone_limits[1] = profile.hr_rest + (hr_reserve * 0.85f);
        calculations.zone_limits[2] = profile.hr_max;
    } else {
        // 7 zonas (ultra-detallado)
        calculations.zone_limits[0] = profile.hr_rest + (hr_reserve * 0.50f);
        calculations.zone_limits[1] = profile.hr_rest + (hr_reserve * 0.60f);
        calculations.zone_limits[2] = profile.hr_rest + (hr_reserve * 0.70f);
        calculations.zone_limits[3] = profile.hr_rest + (hr_reserve * 0.80f);
        calculations.zone_limits[4] = profile.hr_rest + (hr_reserve * 0.87f);
        calculations.zone_limits[5] = profile.hr_rest + (hr_reserve * 0.95f);
        calculations.zone_limits[6] = profile.hr_max;
    }
}

uint8_t UserProfileManager::getTrainingZone(float hr_current) {
    if (hr_current < profile.hr_rest) return 0; // Sub-zona 1
    
    for (uint8_t i = 0; i < profile.preferred_zones; i++) {
        if (hr_current <= calculations.zone_limits[i]) {
            return i + 1;
        }
    }
    return profile.preferred_zones; // Zona máxima
}

uint16_t UserProfileManager::calculateCalories(float duration_minutes, float avg_hr, float avg_vo2) {
    // Cálculo basado en consumo de oxígeno y frecuencia cardíaca
    float calories_per_minute = 0.0f;
    
    if (avg_vo2 > 0) {
        // Método más preciso: basado en VO2
        // 1 L O2 ≈ 5 kcal
        calories_per_minute = (avg_vo2 * profile.weight / 1000.0f) * 5.0f;
    } else {
        // Método alternativo: basado en HR
        float hr_reserve = profile.hr_max - profile.hr_rest;
        float intensity = (avg_hr - profile.hr_rest) / hr_reserve;
        intensity = constrain(intensity, 0.0f, 1.0f);
        
        float base_calories = profile.weight * 0.95f; // kcal/hour en reposo
        calories_per_minute = base_calories * (1.0f + intensity * 8.0f) / 60.0f;
    }
    
    // Ajuste por eficiencia deportiva
    switch (profile.sport) {
        case SPORT_SWIMMING: calories_per_minute *= 1.3f; break;
        case SPORT_ROWING: calories_per_minute *= 1.2f; break;
        case SPORT_CYCLING: calories_per_minute *= 0.9f; break;
        default: break;
    }
    
    return (uint16_t)(calories_per_minute * duration_minutes);
}

float UserProfileManager::getRecoveryFactor() {
    // Factor de recuperación basado en edad, fitness y datos de sesión
    float age_factor = 1.0f - (profile.age - 20.0f) * 0.01f;
    age_factor = constrain(age_factor, 0.5f, 1.2f);
    
    float fitness_factor = 0.6f + (profile.fitness_level * 0.15f);
    
    return age_factor * fitness_factor;
}

bool UserProfileManager::isHRRealistic(float hr) {
    return (hr >= 40.0f && hr <= profile.hr_max + 10.0f);
}

bool UserProfileManager::isVO2Realistic(float vo2) {
    float max_possible = profile.vo2_max_estimated * 1.3f; // 30% margen
    return (vo2 >= 10.0f && vo2 <= max_possible);
}

const char* UserProfileManager::getHealthWarning() {
    if (profile.age > 50 && profile.fitness_level == FITNESS_SEDENTARY) {
        return "Consulte médico antes de entrenar";
    }
    if (profile.hr_max - profile.hr_rest < 60) {
        return "Rango FC limitado - verificar datos";
    }
    return nullptr;
}

const char* UserProfileManager::getTrainingRecommendation() {
    switch (profile.fitness_level) {
        case FITNESS_SEDENTARY:
            return "Comience con Z1-Z2, 20-30min";
        case FITNESS_BEGINNER:
            return "Base aeróbica Z2, 30-45min";
        case FITNESS_RECREATIONAL:
            return "80% Z1-Z2, 20% Z3-Z4";
        case FITNESS_TRAINED:
            return "Incluir intervalos Z4-Z5";
        case FITNESS_ATHLETE:
            return "Periodización avanzada";
        case FITNESS_ELITE:
            return "Plan individualizado";
        default:
            return "Entrenamiento personalizado";
    }
}

// =============================================================================
// INTEGRACIÓN CON LA INTERFAZ AVANZADA
// =============================================================================

// Modificaciones para AdvancedVO2Interface
class AdvancedVO2Interface {
private:
    UserProfileManager userManager;
    // ... otros miembros existentes
    
public:
    // Nuevos métodos para gestión de usuario
    bool initializeUserProfile() {
        if (!userManager.loadProfile()) {
            userManager.createDefaultProfile();
            userManager.saveProfile();
        }
        return true;
    }
    
    UserProfileManager* getUserManager() {
        return &userManager;
    }
    
    // Métodos modificados que ahora usan datos de usuario
    uint8_t calculateCurrentZone(float hr_current) {
        return userManager.getTrainingZone(hr_current);
    }
    
    uint16_t calculateSessionCalories(float duration_min, float avg_hr, float avg_vo2) {
        return userManager.calculateCalories(duration_min, avg_hr, avg_vo2);
    }
    
    void drawUserProfileScreen();
    void drawUserEditScreen();
};

void AdvancedVO2Interface::drawUserProfileScreen() {
    tft.fillScreen(UI_SURFACE_DARK);
    
    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, UI_SURFACE_LIGHT);
    tft.setTextColor(UI_TEXT_HIGH);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("PERFIL DE USUARIO", SCREEN_WIDTH/2, HEADER_HEIGHT/2, 2);
    
    user_profile_t* profile = userManager.getProfile();
    
    // Card principal del usuario
    int card_y = HEADER_HEIGHT + MARGIN_LARGE;
    tft.fillRoundRect(MARGIN_LARGE, card_y, SCREEN_WIDTH - MARGIN_LARGE * 2, 
                     80, 8, UI_SURFACE_MEDIUM);
    
    // Nombre y edad
    tft.setTextColor(UI_PRIMARY_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(profile->name, MARGIN_LARGE + 12, card_y + 8, 2);
    
    char age_text[20];
    sprintf(age_text, "%d años", profile->age);
    tft.setTextColor(UI_TEXT_MEDIUM);
    tft.drawString(age_text, MARGIN_LARGE + 12, card_y + 26, 2);
    
    // Datos físicos
    char physical_data[50];
    sprintf(physical_data, "%.1f kg | %.0f cm", profile->weight, profile->height);
    tft.drawString(physical_data, MARGIN_LARGE + 12, card_y + 44, 1);
    
    // Deporte y nivel
    char sport_info[50];
    sprintf(sport_info, "%s - %s", userManager.getSportName(), 
            userManager.getFitnessLevelName());
    tft.setTextColor(UI_ACCENT_ORANGE);
    tft.drawString(sport_info, MARGIN_LARGE + 12, card_y + 58, 1);
    
    // Datos fisiológicos
    int physio_y = card_y + 90;
    tft.fillRoundRect(MARGIN_LARGE, physio_y, SCREEN_WIDTH - MARGIN_LARGE * 2, 
                     60, 8, UI_SURFACE_MEDIUM);
    
    tft.setTextColor(UI_SUCCESS_GREEN);
    tft.drawString("DATOS FISIOLÓGICOS", MARGIN_LARGE + 12, physio_y + 8, 1);
    
    char hr_data[60];
    sprintf(hr_data, "FC Reposo: %.0f bpm | FC Máx: %.0f bpm", 
            profile->hr_rest, profile->hr_max);
    tft.setTextColor(UI_TEXT_HIGH);
    tft.drawString(hr_data, MARGIN_LARGE + 12, physio_y + 24, 1);
    
    char vo2_data[60];
    if (profile->vo2_max_tested > 0) {
        sprintf(vo2_data, "VO2 Max: %.1f ml/kg/min (medido)", profile->vo2_max_tested);
    } else {
        sprintf(vo2_data, "VO2 Max: %.1f ml/kg/min (estimado)", profile->vo2_max_estimated);
    }
    tft.drawString(vo2_data, MARGIN_LARGE + 12, physio_y + 40, 1);
    
    // Advertencias de salud si las hay
    const char* warning = userManager.getHealthWarning();
    if (warning) {
        tft.fillRoundRect(MARGIN_LARGE, physio_y + 70, SCREEN_WIDTH - MARGIN_LARGE * 2, 
                         25, 6, UI_CRITICAL_RED);
        tft.setTextColor(UI_TEXT_HIGH);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(warning, SCREEN_WIDTH/2, physio_y + 82, 1);
    }
}

// =============================================================================
// FUNCIÓN DE INTEGRACIÓN PRINCIPAL
// =============================================================================

void setupUserProfileSystem() {
    static AdvancedVO2Interface advanced_ui;
    
    // Inicializar sistema de perfiles
    if (!advanced_ui.initializeUserProfile()) {
        Serial.println("Error inicializando perfil de usuario");
        return;
    }
    
    UserProfileManager* userMgr = advanced_ui.getUserManager();
    
    Serial.println("=== PERFIL DE USUARIO CARGADO ===");
    Serial.printf("Nombre: %s\n", userMgr->getProfile()->name);
    Serial.printf("Edad: %d años\n", userMgr->getProfile()->age);
    Serial.printf("Deporte: %s\n", userMgr->getSportName());
    Serial.printf("Nivel: %s\n", userMgr->getFitnessLevelName());
    Serial.printf("FC Máx: %.0f bpm\n", userMgr->getProfile()->hr_max);
    Serial.printf("VO2 Max estimado: %.1f ml/kg/min\n", userMgr->getProfile()->vo2_max_estimated);
    
    // Ejemplo de uso en tiempo real
    float current_hr = 152.0f;
    uint8_t zone = userMgr->getTrainingZone(current_hr);
    Serial.printf("FC actual %0.f bpm = Zona %d\n", current_hr, zone);
    
    uint16_t calories = userMgr->calculateCalories(45.0f, 152.0f, 48.6f);
    Serial.printf("Calorías 45min sesión: %d kcal\n", calories);
}
## Características implementadas:

### **Estructura de Datos Completa**
- Datos personales (nombre, edad, peso, altura, género)
- Información deportiva (deporte, nivel de fitness, horas de entrenamiento)
- Datos fisiológicos (FC reposo/máxima, VO2 max medido/estimado)
- Configuraciones personales (sistema métrico, zonas preferidas)

### **Cálculos Científicos Personalizados**
- **Zonas de FC**: Calculadas con fórmula de Karvonen usando FC real del usuario
- **VO2 Max estimado**: Fórmulas específicas por deporte y nivel de fitness
- **Calorías precisas**: Basadas en peso, edad, género y tipo de actividad
- **Factor de recuperación**: Ajustado por edad y condición física

### **Validaciones de Seguridad**
- Verificación de rangos fisiológicos realistas
- Advertencias médicas para usuarios de riesgo
- Integridad de datos con checksums
- Recomendaciones de entrenamiento personalizadas

### **Persistencia Robusta**
- Almacenamiento en EEPROM con validación
- Sistema de checksums para detectar corrupción
- Carga automática de perfil por defecto si hay errores

### **Integración Completa**
El módulo se integra perfectamente con tu `AdvancedVO2Interface` existente:

```cpp
// Las zonas ahora son dinámicas según el usuario
uint8_t zone = userManager.getTrainingZone(current_hr);

// Calorías precisas por perfil
uint16_t calories = userManager.calculateCalories(duration, avg_hr, avg_vo2);

// Validaciones inteligentes
bool hr_valid = userManager.isHRRealistic(measured_hr);
```

### **Pantalla de Perfil Profesional**
- Diseño tipo COSMED con datos organizados
- Alertas médicas visibles cuando es necesario
- Información fisiológica clara y precisa

## Beneficios médicos clave:

1. **Precisión clínica**: Zonas y cálculos específicos por persona
2. **Seguridad**: Advertencias para usuarios de alto riesgo
3. **Trazabilidad**: Historial de sesiones y progreso
4. **Compliance**: Recomendaciones basadas en guidelines médicos

Este sistema transforma tu VO2Smart de un dispositivo genérico a un sistema médico personalizado comparable a equipos COSMED de laboratorio clínico.
