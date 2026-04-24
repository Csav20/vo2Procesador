/*
 * VO2Smart Advanced UI - Basado en LVGL_Demos.ino funcional
 * Hardware: ESP32-WROOM-32E + ILI9341 (320x240) + XPT2046 Touch
 * 
 * Pin Configuration:
 *                   CS  DC/RS  RESET    SDI/MOSI  SCK   SDO/MISO  BL   RTP_DOUT   RTP_DIN   RTP_SCK   RTP_CS   RTP_IRQ
 * ESP32-WROOM-32E:   15    2   ESP32-EN     13      14      12     21      39        32        25        33        36
 * 
 * Fecha: 1 de octubre de 2025
 * Autor: PhD(c)MgC Claudio Abarca
 * Patente: N° 2024024875
 */
#ifndef USE_DUAL_CORE_UI
#include <lvgl.h>
#include <TFT_eSPI.h> 
#include <TFT_Touch.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <vector>
// ============================================================================
// CONFIGURACIÓN DE HARDWARE (EXACTA DE LVGL_Demos.ino)
// ============================================================================
#define RTP_DOUT 39
#define RTP_DIN  32
#define RTP_SCK  25
#define RTP_CS   33
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
// ============================================================================
// OBJETOS GLOBALES (Adaptado para LVGL v9)
// ============================================================================
static lv_color_t buf[screenWidth * 30];  // Buffer de 30 líneas
TFT_eSPI my_lcd = TFT_eSPI();
TFT_Touch my_touch = TFT_Touch(RTP_CS, RTP_SCK, RTP_DIN, RTP_DOUT);
// ============================================================================
// ESTRUCTURAS DE DATOS VO2SMART
// ============================================================================
struct UserProfile {
    char name[32];
    int age;
    float weight;  // kg
    float height;  // cm
    int gender;    // 0=male, 1=female
};
struct BLEDeviceData {
    String name;
    String address;
    int rssi;
    bool connected;
};
struct VO2Data {
    float vo2;
    float vco2;
    float hr;
    float ve;
    float rer;
    float time;
    float ee;
    uint32_t lastUpdate;
};
// Variables globales
static UserProfile currentUser = {"Usuario", 30, 70.0f, 175.0f, 0};
static std::vector<BLEDeviceData> bleDevices;
static VO2Data vo2Data = {0, 0, 0, 0, 0, 0, 0, 0};
static Preferences preferences;
// BLE
static BLEScan* pBLEScan = nullptr;
static bool scanningBLE = false;
static bool bleConnected = false;
// ============================================================================
// PANTALLAS LVGL
// ============================================================================
enum ScreenID {
    SCREEN_SPLASH = 0,
    SCREEN_SENSOR_SELECT,
    SCREEN_BLE_DEVICES,
    SCREEN_USER_PROFILE,
    SCREEN_START_TEST,
    SCREEN_DASHBOARD,
    SCREEN_COUNT
};
static ScreenID currentScreen = SCREEN_SPLASH;
static lv_obj_t* currentScreenObj = nullptr;
// ============================================================================
// CALLBACKS LVGL (Adaptado para LVGL v9)
// ============================================================================
/* Display flushing - LVGL v9 API */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    lv_draw_sw_rgb565_swap(px_map, w * h);
    my_lcd.startWrite();
    my_lcd.setAddrWindow(area->x1, area->y1, w, h);
    my_lcd.pushPixelsDMA((uint16_t*)px_map, w * h);
    my_lcd.endWrite();
    lv_display_flush_ready(disp);
}
/* Touch reading - LVGL v9 API */
void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
    bool touched = my_touch.Pressed();
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = my_touch.X();
        data->point.y = my_touch.Y();
    }
}
// ============================================================================
// FUNCIONES DE PANTALLAS VO2SMART
// ============================================================================
void createScreen_Splash();
void createScreen_SensorSelect();
void createScreen_BLEDevices();
void createScreen_UserProfile();
void createScreen_StartTest();
void createScreen_Dashboard();
static void btn_next_event_cb(lv_event_t * e);
static void btn_back_event_cb(lv_event_t * e);
static void btn_scan_ble_event_cb(lv_event_t * e);
static void btn_start_test_event_cb(lv_event_t * e);
// ============================================================================
// PANTALLA 0: SPLASH
// ============================================================================
void createScreen_Splash()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x1E1E1E), 0);
    lv_obj_t * title = lv_label_create(currentScreenObj);
    lv_label_set_text(title, "VO2Smart");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t * subtitle = lv_label_create(currentScreenObj);
    lv_label_set_text(subtitle, "Sistema Avanzado de Analisis Fisiologico");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t * version = lv_label_create(currentScreenObj);
    lv_label_set_text(version, "v2.0 - Patente 2024024875");
    lv_obj_set_style_text_color(version, lv_color_hex(0x888888), 0);
    lv_obj_align(version, LV_ALIGN_CENTER, 0, 40);
    lv_obj_t * btn = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn, 140, 45);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, btn_next_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "INICIAR");
    lv_obj_center(btn_label);
    Serial.println("✓ Splash screen created");
}
// ============================================================================
// PANTALLA 1: SENSOR SELECT
// ============================================================================
void createScreen_SensorSelect()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x1E1E1E), 0);
    lv_obj_t * header = lv_label_create(currentScreenObj);
    lv_label_set_text(header, "Seleccionar Sensores");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);
    // Card VO2Smart
    lv_obj_t * card1 = lv_obj_create(currentScreenObj);
    lv_obj_set_size(card1, 280, 60);
    lv_obj_align(card1, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(card1, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_color(card1, lv_color_hex(0x00D9FF), 0);
    lv_obj_set_style_border_width(card1, 2, 0);
    lv_obj_t * card1_label = lv_label_create(card1);
    lv_label_set_text(card1_label, "Dispositivo VO2Smart\nAnalizador de gases");
    lv_obj_set_style_text_color(card1_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(card1_label);
    // Card HR
    lv_obj_t * card2 = lv_obj_create(currentScreenObj);
    lv_obj_set_size(card2, 280, 60);
    lv_obj_align(card2, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(card2, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_color(card2, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_border_width(card2, 2, 0);
    lv_obj_t * card2_label = lv_label_create(card2);
    lv_label_set_text(card2_label, "Sensor Frecuencia Cardiaca\nPolar / Garmin / Wahoo");
    lv_obj_set_style_text_color(card2_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(card2_label);
    lv_obj_t * btn = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn, 200, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00D9FF), 0);
    lv_obj_add_event_cb(btn, btn_scan_ble_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "BUSCAR Y CONECTAR");
    lv_obj_center(btn_label);
    Serial.println("✓ Sensor Select screen created");
}
// ============================================================================
// PANTALLA 2: BLE DEVICES
// ============================================================================
void createScreen_BLEDevices()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x1E1E1E), 0);
    lv_obj_t * header = lv_label_create(currentScreenObj);
    lv_label_set_text(header, "Dispositivos Bluetooth");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t * status = lv_label_create(currentScreenObj);
    if (scanningBLE) {
        lv_label_set_text(status, "Escaneando...");
    } else if (bleDevices.size() > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d dispositivos encontrados", bleDevices.size());
        lv_label_set_text(status, buf);
    } else {
        lv_label_set_text(status, "Sin dispositivos");
    }
    lv_obj_set_style_text_color(status, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 40);
    int y_offset = 70;
    for (int i = 0; i < (int)bleDevices.size() && i < 4; i++) {
        lv_obj_t * card = lv_obj_create(currentScreenObj);
        lv_obj_set_size(card, 280, 35);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y_offset);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x2A2A2A), 0);
        if (bleDevices[i].connected) {
            lv_obj_set_style_border_color(card, lv_color_hex(0x4CAF50), 0);
            lv_obj_set_style_border_width(card, 2, 0);
        }
        lv_obj_t * lbl = lv_label_create(card);
        char label_text[64];
        snprintf(label_text, sizeof(label_text), "%s (%d dBm)",
                 bleDevices[i].name.c_str(), bleDevices[i].rssi);
        lv_label_set_text(lbl, label_text);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 10, 0);
        y_offset += 40;
    }
    lv_obj_t * btn_back = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_back, 100, 35);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, btn_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bbl = lv_label_create(btn_back);
    lv_label_set_text(bbl, "ATRAS");
    lv_obj_center(bbl);
    if (bleDevices.size() > 0) {
        lv_obj_t * btn_cont = lv_btn_create(currentScreenObj);
        lv_obj_set_size(btn_cont, 100, 35);
        lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_set_style_bg_color(btn_cont, lv_color_hex(0x00D9FF), 0);
        lv_obj_add_event_cb(btn_cont, btn_next_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t * bcl = lv_label_create(btn_cont);
        lv_label_set_text(bcl, "CONTINUAR");
        lv_obj_center(bcl);
    }
    Serial.println("✓ BLE Devices screen created");
}
// ============================================================================
// PANTALLA 3: USER PROFILE
// ============================================================================
void createScreen_UserProfile()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x1E1E1E), 0);
    lv_obj_t * header = lv_label_create(currentScreenObj);
    lv_label_set_text(header, "Perfil de Usuario");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);
    int y = 50;
    char buf[64];
    const struct { const char* fmt; float val; int is_int; } fields[] = {
        {"Nombre: %s", 0, -1},
        {"Edad: %d anos", (float)currentUser.age, 1},
        {"Peso: %.1f kg", currentUser.weight, 0},
        {"Altura: %.0f cm", currentUser.height, 0},
    };
    // Nombre
    lv_obj_t * lbl = lv_label_create(currentScreenObj);
    snprintf(buf, sizeof(buf), "Nombre: %s", currentUser.name);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 20, y); y += 25;
    // Edad
    lbl = lv_label_create(currentScreenObj);
    snprintf(buf, sizeof(buf), "Edad: %d anos", currentUser.age);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 20, y); y += 25;
    // Peso
    lbl = lv_label_create(currentScreenObj);
    snprintf(buf, sizeof(buf), "Peso: %.1f kg", currentUser.weight);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 20, y); y += 25;
    // Altura
    lbl = lv_label_create(currentScreenObj);
    snprintf(buf, sizeof(buf), "Altura: %.0f cm", currentUser.height);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 20, y); y += 25;
    // Género
    lbl = lv_label_create(currentScreenObj);
    snprintf(buf, sizeof(buf), "Genero: %s", currentUser.gender == 0 ? "Masculino" : "Femenino");
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 20, y);
    lv_obj_t * btn_back = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_back, 100, 35);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, btn_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bbl = lv_label_create(btn_back);
    lv_label_set_text(bbl, "ATRAS");
    lv_obj_center(bbl);
    lv_obj_t * btn_cont = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_cont, 100, 35);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn_cont, lv_color_hex(0x00D9FF), 0);
    lv_obj_add_event_cb(btn_cont, btn_next_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bcl = lv_label_create(btn_cont);
    lv_label_set_text(bcl, "INICIAR");
    lv_obj_center(bcl);
    Serial.println("✓ User Profile screen created");
}
// ============================================================================
// PANTALLA 4: START TEST
// ============================================================================
void createScreen_StartTest()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x1E1E1E), 0);
    lv_obj_t * header = lv_label_create(currentScreenObj);
    lv_label_set_text(header, "Iniciar Prueba");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t * instr = lv_label_create(currentScreenObj);
    lv_label_set_text(instr, "Preparese para iniciar\nla prueba de esfuerzo");
    lv_obj_set_style_text_align(instr, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(instr, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(instr, LV_ALIGN_CENTER, 0, -20);
    lv_obj_t * btn_start = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_start, 180, 60);
    lv_obj_align(btn_start, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(btn_start, btn_start_test_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bsl = lv_label_create(btn_start);
    lv_label_set_text(bsl, "INICIAR");
    lv_obj_set_style_text_font(bsl, &lv_font_montserrat_24, 0);
    lv_obj_center(bsl);
    lv_obj_t * btn_back = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_back, 100, 35);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, btn_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bbl = lv_label_create(btn_back);
    lv_label_set_text(bbl, "ATRAS");
    lv_obj_center(bbl);
    Serial.println("✓ Start Test screen created");
}
// ============================================================================
// PANTALLA 5: DASHBOARD
// ============================================================================
void createScreen_Dashboard()
{
    if (currentScreenObj != nullptr) lv_obj_del(currentScreenObj);
    currentScreenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(currentScreenObj, screenWidth, screenHeight);
    lv_obj_set_style_bg_color(currentScreenObj, lv_color_hex(0x000000), 0);
    lv_obj_t * header = lv_label_create(currentScreenObj);
    lv_label_set_text(header, "VO2Smart - En Prueba");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    // Grid 2x3: VO2, VCO2, HR / VE, RER, Time
    int sx = 10, sy = 30;
    char b[16];
    // VO2
    lv_obj_t * l = lv_label_create(currentScreenObj);
    lv_label_set_text(l, "VO2");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(l, sx, sy);
    lv_obj_t * v = lv_label_create(currentScreenObj);
    snprintf(b, sizeof(b), "%.1f", vo2Data.vo2);
    lv_label_set_text(v, b);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_pos(v, sx, sy + 20);
    // VCO2
    l = lv_label_create(currentScreenObj); lv_label_set_text(l, "VCO2");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0); lv_obj_set_pos(l, sx+110, sy);
    v = lv_label_create(currentScreenObj); snprintf(b, sizeof(b), "%.1f", vo2Data.vco2);
    lv_label_set_text(v, b); lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0xFF9800), 0); lv_obj_set_pos(v, sx+110, sy+20);
    // HR
    l = lv_label_create(currentScreenObj); lv_label_set_text(l, "HR");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0); lv_obj_set_pos(l, sx+220, sy);
    v = lv_label_create(currentScreenObj); snprintf(b, sizeof(b), "%.0f", vo2Data.hr);
    lv_label_set_text(v, b); lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0xFF6B6B), 0); lv_obj_set_pos(v, sx+220, sy+20);
    // Segunda fila
    sy = 100;
    // VE
    l = lv_label_create(currentScreenObj); lv_label_set_text(l, "VE");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0); lv_obj_set_pos(l, sx, sy);
    v = lv_label_create(currentScreenObj); snprintf(b, sizeof(b), "%.1f", vo2Data.ve);
    lv_label_set_text(v, b); lv_obj_set_style_text_font(v, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0x2196F3), 0); lv_obj_set_pos(v, sx, sy+20);
    // RER
    l = lv_label_create(currentScreenObj); lv_label_set_text(l, "RER");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0); lv_obj_set_pos(l, sx+110, sy);
    v = lv_label_create(currentScreenObj); snprintf(b, sizeof(b), "%.2f", vo2Data.rer);
    lv_label_set_text(v, b); lv_obj_set_style_text_font(v, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0x9C27B0), 0); lv_obj_set_pos(v, sx+110, sy+20);
    // Time
    l = lv_label_create(currentScreenObj); lv_label_set_text(l, "Tiempo");
    lv_obj_set_style_text_color(l, lv_color_hex(0x888888), 0); lv_obj_set_pos(l, sx+220, sy);
    v = lv_label_create(currentScreenObj);
    int min = (int)vo2Data.time / 60, sec = (int)vo2Data.time % 60;
    snprintf(b, sizeof(b), "%02d:%02d", min, sec);
    lv_label_set_text(v, b); lv_obj_set_style_text_font(v, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0xFFEB3B), 0); lv_obj_set_pos(v, sx+220, sy+20);
    // Botón DETENER
    lv_obj_t * btn_stop = lv_btn_create(currentScreenObj);
    lv_obj_set_size(btn_stop, 140, 40);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0xF44336), 0);
    lv_obj_add_event_cb(btn_stop, btn_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * bsl = lv_label_create(btn_stop);
    lv_label_set_text(bsl, "DETENER");
    lv_obj_center(bsl);
    Serial.println("✓ Dashboard screen created");
}
// ============================================================================
// EVENT CALLBACKS
// ============================================================================
static void btn_next_event_cb(lv_event_t * e)
{
    currentScreen = (ScreenID)((int)currentScreen + 1);
    if (currentScreen >= SCREEN_COUNT) currentScreen = SCREEN_SPLASH;
    Serial.printf("→ Next screen: %d\n", currentScreen);
    switch (currentScreen) {
        case SCREEN_SPLASH:        createScreen_Splash();        break;
        case SCREEN_SENSOR_SELECT: createScreen_SensorSelect();  break;
        case SCREEN_BLE_DEVICES:   createScreen_BLEDevices();    break;
        case SCREEN_USER_PROFILE:  createScreen_UserProfile();   break;
        case SCREEN_START_TEST:    createScreen_StartTest();      break;
        case SCREEN_DASHBOARD:     createScreen_Dashboard();      break;
        default: break;
    }
}
static void btn_back_event_cb(lv_event_t * e)
{
    int prev = (int)currentScreen - 1;
    if (prev < 0) prev = 0;
    currentScreen = (ScreenID)prev;
    Serial.printf("← Back screen: %d\n", currentScreen);
    switch (currentScreen) {
        case SCREEN_SPLASH:        createScreen_Splash();        break;
        case SCREEN_SENSOR_SELECT: createScreen_SensorSelect();  break;
        case SCREEN_BLE_DEVICES:   createScreen_BLEDevices();    break;
        case SCREEN_USER_PROFILE:  createScreen_UserProfile();   break;
        case SCREEN_START_TEST:    createScreen_StartTest();      break;
        case SCREEN_DASHBOARD:     createScreen_Dashboard();      break;
        default: break;
    }
}
static void btn_scan_ble_event_cb(lv_event_t * e)
{
    Serial.println("📡 Iniciando escaneo BLE...");
    scanningBLE = true;
    BLEDeviceData dev1 = {"VO2Smart-001", "AA:BB:CC:DD:EE:FF", -45, false};
    BLEDeviceData dev2 = {"Polar H10",    "11:22:33:44:55:66", -52, false};
    bleDevices.clear();
    bleDevices.push_back(dev1);
    bleDevices.push_back(dev2);
    scanningBLE = false;
    currentScreen = SCREEN_BLE_DEVICES;
    createScreen_BLEDevices();
}
static void btn_start_test_event_cb(lv_event_t * e)
{
    Serial.println("🚀 Iniciando prueba...");
    vo2Data = {0, 0, 0, 0, 0, 0, 0, (uint32_t)millis()};
    currentScreen = SCREEN_DASHBOARD;
    createScreen_Dashboard();
}
// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("========================================");
    Serial.println("VO2Smart v2.0 - LVGL v9");
    Serial.println("Hardware: ESP32-WROOM-32E + ILI9341 + XPT2046");
    Serial.println("========================================");
    my_lcd.init();
    my_lcd.fillScreen(0xFFFF);
    my_lcd.setRotation(1);
    my_touch.setCal(495, 3398, 721, 3448, 320, 240, 1);
    Serial.println("✓ Touch calibrated (495,3398,721,3448)");
    lv_init();
    delay(10);
    Serial.println("✓ LVGL v9 initialized");
    lv_display_t * disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("✓ Display configured");
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    Serial.println("✓ Touch input registered");
    preferences.begin("vo2smart", false);
    String name = preferences.getString("userName", "Usuario");
    strlcpy(currentUser.name, name.c_str(), sizeof(currentUser.name));
    currentUser.age    = preferences.getInt("userAge", 30);
    currentUser.weight = preferences.getFloat("userWeight", 70.0f);
    currentUser.height = preferences.getFloat("userHeight", 175.0f);
    currentUser.gender = preferences.getInt("userGender", 0);
    Serial.println("✓ User profile loaded");
    createScreen_Splash();
    Serial.println("✅ Setup complete - System ready");
}
// ============================================================================
// LOOP
// ============================================================================
void loop()
{
    lv_timer_handler();
    delay(5);
    if (currentScreen == SCREEN_DASHBOARD) {
        static uint32_t last_update = 0;
        uint32_t now = millis();
        if (now - last_update > 1000) {
            vo2Data.vo2  += 0.5f;
            vo2Data.vco2 += 0.4f;
            vo2Data.hr   += 1.0f;
            vo2Data.ve   += 0.3f;
            vo2Data.rer   = vo2Data.vco2 / (vo2Data.vo2 + 0.1f);
            vo2Data.time  = (now - vo2Data.lastUpdate) / 1000.0f;
            createScreen_Dashboard();
            last_update = now;
        }
    }
}
#endif // USE_DUAL_CORE_UI
