
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Proyecto: VO2Smart UI - Firmware v8.0
// Placa: ESP32 con TFT (TFT_eSPI o similar)
// Autor: Claudio Abarca
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <TFT_eSPI.h>
#include <SPI.h>

//===================== CONFIGURACIÓN =====================
TFT_eSPI tft = TFT_eSPI();
#define BTN_NEXT 0   // GPIO botón siguiente pantalla
#define BTN_BACK 35  // GPIO botón pantalla anterior

int pantallaActual = 0;
const int totalPantallas = 22;

//===================== DECLARACIÓN DE PANTALLAS =====================
void drawPantalla0();  // Inicio sistema
void drawPantalla1();  // Selección sensores
void drawPantalla2();  // Configuración conexión
void drawPantalla3();  // Conexión establecida
void drawPantalla4();  // Sincronización datos
void drawPantalla5();  // Dashboard principal
void drawPantalla6();  // Métricas biomédicas
void drawPantalla7();  // Entrenamiento tiempo real
void drawPantalla8();  // Gráficos rendimiento
void drawPantalla9();  // Análisis metabólico
void drawPantalla10(); // Recuperación post-ejercicio
void drawPantalla11(); // Alertas y notificaciones
void drawPantalla12(); // Configuración sistema
void drawPantalla13(); // Perfiles usuario
void drawPantalla14(); // Modo offline
void drawPantalla15(); // Exportación datos
void drawPantalla16(); // Integración dispositivos
void drawPantalla17(); // Inteligencia artificial
void drawPantalla18(); // Historias clínicas
void drawPantalla19(); // Simulación pruebas
void drawPantalla20(); // Ajustes visualización
void drawPantalla21(); // Acerca de / créditos

//===================== SISTEMA DE PANTALLAS =====================
void mostrarPantalla() {
  switch (pantallaActual) {
    case 0: drawPantalla0(); break;
    case 1: drawPantalla1(); break;
    case 2: drawPantalla2(); break;
    case 3: drawPantalla3(); break;
    case 4: drawPantalla4(); break;
    case 5: drawPantalla5(); break;
    case 6: drawPantalla6(); break;
    case 7: drawPantalla7(); break;
    case 8: drawPantalla8(); break;
    case 9: drawPantalla9(); break;
    case 10: drawPantalla10(); break;
    case 11: drawPantalla11(); break;
    case 12: drawPantalla12(); break;
    case 13: drawPantalla13(); break;
    case 14: drawPantalla14(); break;
    case 15: drawPantalla15(); break;
    case 16: drawPantalla16(); break;
    case 17: drawPantalla17(); break;
    case 18: drawPantalla18(); break;
    case 19: drawPantalla19(); break;
    case 20: drawPantalla20(); break;
    case 21: drawPantalla21(); break;
    default: drawPantalla0(); break;
  }
}

//===================== SETUP =====================
void setup() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  mostrarPantalla();
}

//===================== LOOP =====================
void loop() {
  if (digitalRead(BTN_NEXT) == LOW) {
    pantallaActual++;
    if (pantallaActual >= totalPantallas) pantallaActual = 0;
    mostrarPantalla();
    delay(200);
  }
  if (digitalRead(BTN_BACK) == LOW) {
    pantallaActual--;
    if (pantallaActual < 0) pantallaActual = totalPantallas - 1;
    mostrarPantalla();
    delay(200);
  }
}

//===================== DEFINICIÓN DE PANTALLAS =====================

// Pantalla 0: Inicio
void drawPantalla0() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("VO2Smart", 160, 100, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Firmware v8.0", 160, 140, 2);
}

// Pantalla 1: Selección de sensores
void drawPantalla1() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Seleccionar Sensores", 10, 10, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("FC: Desconectado", 20, 50, 2);
  tft.drawString("VO2: Desconectado", 20, 80, 2);
  tft.fillRoundRect(40, 160, 160, 40, 8, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("Buscar y Conectar", 120, 180, 2);
}

// Pantalla 2: Configuración conexión
void drawPantalla2() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Configurando Conexion...", 20, 120, 2);
}

// Pantalla 3: Conexión establecida
void drawPantalla3() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Conexion Establecida", 60, 120, 2);
}

// Pantalla 4: Sincronización datos
void drawPantalla4() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Sincronizando Datos...", 40, 120, 2);
}

// Pantalla 5: Dashboard principal
void drawPantalla5() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Dashboard Principal", 10, 10, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("FC: 78 bpm", 20, 60, 2);
  tft.drawString("VO2: 36 ml/kg/min", 20, 90, 2);
  tft.drawString("Dist: 2.4 km", 20, 120, 2);
}

// Pantalla 6: Métricas biomédicas
void drawPantalla6() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Metricas Biomedicas", 10, 10, 2);
  tft.drawString("Glucosa: --", 20, 60, 2);
  tft.drawString("Lactato: --", 20, 90, 2);
  tft.drawString("Vitamina C: --", 20, 120, 2);
}

// Pantalla 7: Entrenamiento tiempo real
void drawPantalla7() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Entrenamiento Tiempo Real", 10, 10, 2);
}

// Pantalla 8: Graficos rendimiento
void drawPantalla8() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Graficos Rendimiento", 10, 10, 2);
}

// Pantalla 9: Analisis metabolico
void drawPantalla9() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Analisis Metabolico", 10, 10, 2);
}

// Pantalla 10: Recuperacion
void drawPantalla10() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Recuperacion Post-Ejercicio", 10, 10, 2);
}

// Pantalla 11: Alertas
void drawPantalla11() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Alertas y Notificaciones", 10, 10, 2);
}

// Pantalla 12: Configuración
void drawPantalla12() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Configuracion Sistema", 10, 10, 2);
}

// Pantalla 13: Perfiles
void drawPantalla13() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Perfiles de Usuario", 10, 10, 2);
}

// Pantalla 14: Modo offline
void drawPantalla14() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Modo Offline", 10, 10, 2);
}

// Pantalla 15: Exportación datos
void drawPantalla15() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Exportando Datos...", 10, 10, 2);
}

// Pantalla 16: Integración dispositivos
void drawPantalla16() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Integracion con Dispositivos", 10, 10, 2);
}

// Pantalla 17: Inteligencia artificial
void drawPantalla17() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Modulo IA", 10, 10, 2);
}

// Pantalla 18: Historias clínicas
void drawPantalla18() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Historias Clinicas", 10, 10, 2);
}

// Pantalla 19: Simulación pruebas
void drawPantalla19() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Simulacion de Pruebas", 10, 10, 2);
}

// Pantalla 20: Ajustes visualización
void drawPantalla20() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Ajustes de Visualizacion", 10, 10, 2);
}

// Pantalla 21: Créditos
void drawPantalla21() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Acerca de / Creditos", 10, 10, 2);
  tft.drawString("VO2Smart v8.0", 20, 60, 2);
  tft.drawString("Claudio Abarca", 20, 90, 2);
}

