#include <WiFi.h>
#include <esp_now.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Keypad.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

#define PIN_PARO 23 
bool sistemaBloqueado = false;
bool paroRemoto = false;

#define VRX 34 
#define VRY 35 
#define SW_PIN 5 
int centroVRX = 0, centroVRY = 0; 

#define LED_JOYSTICK 15 
#define LED_ANGULOS   2 
#define LED_PUNTOS    4 

const byte FILAS = 4;
const byte COLS  = 4;
char teclas[FILAS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pinesFilas[FILAS]    = {27, 14, 12, 13};
byte pinesColumnas[COLS]  = {32, 33, 25, 26};
Keypad keypad = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLS);

uint8_t slaveMAC[] = {0x88, 0x57, 0x21, 0x8B, 0x3D, 0x58};
typedef struct { char msg[24]; } Packet;
Packet packet;

bool enControlJoystick = false;     
bool enSeleccionMotor = false;    
bool enDigitacionAngulo = false; 
int menuActual = 0;
int cursorMenu = 0;
int motorSeleccionado = 1;

const int NUM_MOTORES = 5;
String nombresMotores[NUM_MOTORES] = {"Base", "Hombro", "Codo", "Muneca", "Garra"};
int cursorMotor = 0;
String estadoMotor = "STOP";
String ultimoEstado = "";
int puntosGuardadosCount = 0;
String inputAngulo = ""; 

unsigned long lastButtonPress = 0;
const int doubleClickTime = 500;
bool lastButtonState = HIGH;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&packet, data, sizeof(packet));
  String msg = String(packet.msg); msg.trim();
  if (msg == "EMERGENCIA_ON") paroRemoto = true;
  else if (msg == "EMERGENCIA_OFF") paroRemoto = false;
}

// 🔥 NUEVA FUNCIÓN: Saludo de inicio
void saludoInicial() {
  oled.setFont(u8g2_font_ncenB14_tr);
  
  oled.clearBuffer();
  int w1 = oled.getStrWidth("Hello!");
  oled.drawStr((128 - w1) / 2, 40, "Hello!");
  oled.sendBuffer();
  delay(1200);

  oled.clearBuffer();
  int w2 = oled.getStrWidth("Hola!");
  oled.drawStr((128 - w2) / 2, 40, "Hola!");
  oled.sendBuffer();
  delay(1200);

  oled.clearBuffer();
  int w3 = oled.getStrWidth("Ni hao!");
  oled.drawStr((128 - w3) / 2, 40, "Ni hao!");
  oled.sendBuffer();
  delay(1200);
}

void pantallaBloqueo() {
  static bool parpadeo = false;
  oled.clearBuffer();
  if (parpadeo) oled.drawFrame(0, 0, 128, 64);
  oled.drawFrame(2, 2, 124, 60);
  oled.setFont(u8g2_font_ncenB10_tr);
  oled.drawStr(30, 28, "BLOQUEO");
  oled.setFont(u8g2_font_6x10_tr);
  if (paroRemoto) oled.drawStr(20, 48, "PARO ESCLAVO");
  else oled.drawStr(20, 48, "PARO MAESTRO");
  oled.sendBuffer();
  parpadeo = !parpadeo;
}

void pantallaLiberacion() {
  oled.clearBuffer(); oled.drawBox(4, 12, 120, 40); oled.setDrawColor(0);
  oled.setFont(u8g2_font_ncenB08_tr); oled.drawStr(20, 30, "SISTEMA LIBRE");
  oled.setFont(u8g2_font_5x8_tf); oled.drawStr(15, 45, "LISTO PARA OPERAR");
  oled.setDrawColor(1); oled.sendBuffer(); delay(1500);
}

void apagarLeds() {
  digitalWrite(LED_JOYSTICK, LOW); digitalWrite(LED_ANGULOS, LOW); digitalWrite(LED_PUNTOS, LOW);
}

void enviarESPNow(String txt) {
  txt.toCharArray(packet.msg, sizeof(packet.msg));
  esp_now_send(slaveMAC, (uint8_t*)&packet, sizeof(packet));
}

void drawIcon(uint8_t type, int x, int y) {
  switch (type) {
    case 0: oled.drawCircle(x+4,y,2); oled.drawLine(x+4,y-4,x+4,y+4); oled.drawLine(x,y,x+8,y); break;
    case 1: oled.drawCircle(x+4,y,1); oled.drawArc(x+4,y,6,0,90); break;
    case 2: oled.drawCircle(x+4,y+2,3); oled.drawLine(x+4,y-4,x+4,y+2); break;
  }
}

void drawItem(uint8_t pos, uint8_t index, const char* text) {
  int y = 4 + pos * 20;
  if (cursorMenu == index) oled.drawFrame(0, y, 128, 18);
  drawIcon(index, 6, y + 9); oled.drawStr(24, y + 13, text);
}

void mostrarMenuPrincipal() {
  apagarLeds(); oled.clearBuffer(); oled.setFont(u8g2_font_5x8_tf);
  drawItem(0, 0, "Prog. Puntos"); drawItem(1, 1, "Prog. Angulos"); drawItem(2, 2, "Prog. Joystick");
  oled.sendBuffer();
}

void mostrarMenuMotores() {
  oled.clearBuffer(); oled.setFont(u8g2_font_5x8_tf);
  if(menuActual == 1) oled.drawStr(0, 8, "MODO PUNTOS");
  else if(menuActual == 2) oled.drawStr(0, 8, "MODO ANGULOS");
  else oled.drawStr(0, 8, "MODO JOYSTICK");
  
  for (int i = 0; i < NUM_MOTORES; i++) {
    int y = 20 + i * 10;
    if (i == cursorMotor) oled.drawStr(0, y, ">"); 
    oled.drawStr(10, y, ("M" + String(i+1) + ": " + nombresMotores[i]).c_str());
  }
  oled.sendBuffer();
}

void mostrarIngresoAngulo() {
  oled.clearBuffer(); oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 12, ("ANG: " + nombresMotores[motorSeleccionado-1]).c_str());
  oled.setFont(u8g2_font_ncenB14_tr); oled.drawStr(10, 40, inputAngulo.c_str());
  oled.setFont(u8g2_font_5x8_tf); oled.drawStr(0, 63, "*: +/-  D:Back  #:Go");
  oled.sendBuffer();
}

void mostrarEstadoMotor(String motor, String estado) {
  oled.clearBuffer(); oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(0, 10, ("MOTOR: " + motor).c_str());
  
  if (menuActual == 3) { 
    int cx = 64; int cy = 40;
    if (estado == "NEGATIVE") oled.drawBox(cx-12, cy-4, 24, 6);
    else if (estado == "POSITIVE") { oled.drawBox(cx-12, cy-4, 24, 6); oled.drawBox(cx-3, cy-13, 6, 24); } 
    else oled.drawDisc(cx, cy, 6); 
    oled.drawStr(0, 62, estado.c_str());
  } 
  else if (menuActual == 1) { 
    // 🔥 AUTO-CENTRADO para que encaje perfecto "POSITIVE" o "FREE"
    oled.setFont(u8g2_font_ncenB10_tr);
    int textWidth = oled.getStrWidth(estado.c_str());
    oled.drawStr((128 - textWidth) / 2, 35, estado.c_str()); 
    
    oled.setFont(u8g2_font_5x8_tf);
    oled.drawStr(0, 53, "*:Lib #:Fij   Pts:");
    oled.setCursor(95, 53); oled.print(puntosGuardadosCount);
    oled.drawStr(0, 63, "1:LibTodos 2:FijTodos");
  }
  
  oled.sendBuffer();
}

void mostrarTexto(String l1, String l2 = "") {
  oled.clearBuffer(); oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(0, 22, l1.c_str()); oled.drawStr(0, 40, l2.c_str());
  oled.sendBuffer();
}

void controlJoystickLoop() {
  int x = analogRead(VRX), y = analogRead(VRY);
  const int DZ = 700; 
  int diffX = abs(x - centroVRX), diffY = abs(y - centroVRY);
  if (diffX > diffY && diffX > DZ) estadoMotor = (x < centroVRX) ? "NEGATIVE" : "POSITIVE"; 
  else if (diffY > diffX && diffY > DZ) estadoMotor = (y < centroVRY) ? "NEGATIVE" : "POSITIVE"; 
  else estadoMotor = "STOP";

  if (estadoMotor != ultimoEstado) {
    ultimoEstado = estadoMotor;
    enviarESPNow("M" + String(motorSeleccionado) + "_" + estadoMotor);
    mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], estadoMotor);
  }
}

void setup() {
  pinMode(PIN_PARO, INPUT_PULLUP);
  pinMode(LED_JOYSTICK, OUTPUT); pinMode(LED_ANGULOS, OUTPUT); pinMode(LED_PUNTOS, OUTPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  apagarLeds();
  
  Wire.begin(21, 22); 
  oled.begin();

  saludoInicial();

  WiFi.mode(WIFI_STA); esp_now_init(); esp_now_register_recv_cb(onDataRecv);
  esp_now_peer_info_t peerInfo = {}; memcpy(peerInfo.peer_addr, slaveMAC, 6); esp_now_add_peer(&peerInfo);
  long sumX = 0, sumY = 0;
  for (int i = 0; i < 20; i++) { sumX += analogRead(VRX); sumY += analogRead(VRY); delay(10); }
  centroVRX = sumX / 20; centroVRY = sumY / 20;
  
  mostrarMenuPrincipal();
}

void loop() {
  if (digitalRead(PIN_PARO) == HIGH || paroRemoto == true) { 
    if (!sistemaBloqueado) { 
      enviarESPNow("EMERGENCIA_ON"); 
      enviarESPNow("STOP");          
      sistemaBloqueado = true; 
    }
    pantallaBloqueo(); delay(200); return;
  } 
  if (sistemaBloqueado && digitalRead(PIN_PARO) == LOW && paroRemoto == false) {
    enviarESPNow("EMERGENCIA_OFF");  
    pantallaLiberacion(); sistemaBloqueado = false; mostrarMenuPrincipal(); return;
  }

  bool btn = digitalRead(SW_PIN);
  if (btn == LOW && lastButtonState == HIGH) {
    unsigned long now = millis();
    if (now - lastButtonPress < doubleClickTime) {
      enviarESPNow("M1_HOME");
      mostrarTexto("CALIBRANDO...", "Base y Nanos");
      delay(1200);
      if (enControlJoystick) mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP");
      else if (enDigitacionAngulo) mostrarIngresoAngulo();
      else mostrarMenuPrincipal();
    }
    lastButtonPress = now;
  }
  lastButtonState = btn;

  char t = keypad.getKey();

  if (!t) { 
    if (enControlJoystick && (menuActual == 3 || menuActual == 1)) controlJoystickLoop(); 
    return; 
  }

  if (menuActual == 0) {
    if (t == '*') { cursorMenu = (cursorMenu + 1) % 3; mostrarMenuPrincipal(); }
    if (t == '#') { 
      menuActual = cursorMenu + 1; enSeleccionMotor = true; apagarLeds();
      if(menuActual == 1) digitalWrite(LED_PUNTOS, HIGH);
      if(menuActual == 2) digitalWrite(LED_ANGULOS, HIGH);
      if(menuActual == 3) digitalWrite(LED_JOYSTICK, HIGH);
      mostrarMenuMotores();
    }
  } 
  else {
    if (enSeleccionMotor) {
      if (t == '*') { cursorMotor = (cursorMotor + 1) % NUM_MOTORES; mostrarMenuMotores(); }
      if (t == 'D') { menuActual = 0; mostrarMenuPrincipal(); }
      if (t == '#') { 
        motorSeleccionado = cursorMotor + 1; enSeleccionMotor = false;
        if(menuActual == 2) { enDigitacionAngulo = true; inputAngulo = ""; mostrarIngresoAngulo(); }
        else { enControlJoystick = true; ultimoEstado = ""; mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
      }
    } 
    else if (enControlJoystick) {
      if (menuActual == 1) { 
        if (t == '*') { enviarESPNow("M" + String(motorSeleccionado) + "_FREE"); mostrarTexto("LIBERADO (Mano)", "M" + String(motorSeleccionado)); delay(600); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "FREE"); }
        if (t == '#') { enviarESPNow("M" + String(motorSeleccionado) + "_LOCK"); mostrarTexto("BLOQUEADO", "Posicion Fija"); delay(600); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
        
        if (t == '1') { enviarESPNow("FREE_ALL"); mostrarTexto("LIBERANDO...", "Todos los ejes"); delay(800); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "FREE"); }
        if (t == '2') { enviarESPNow("LOCK_ALL"); mostrarTexto("BLOQUEANDO...", "Todos los ejes"); delay(800); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
        
        if (t == 'A') { enviarESPNow("SAVE"); if(puntosGuardadosCount < 20) puntosGuardadosCount++; mostrarTexto("Punto Guardado", "Ruta"); delay(600); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
        if (t == 'B') { enviarESPNow("DELETE"); if(puntosGuardadosCount > 0) puntosGuardadosCount--; mostrarTexto("Borrado", "Ultimo Punto"); delay(600); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
        if (t == 'C') { enviarESPNow("PLAY"); mostrarTexto("Reproduciendo...", "En Orden"); delay(1200); mostrarEstadoMotor(nombresMotores[motorSeleccionado-1], "STOP"); }
      }
      if (t == 'D') { enControlJoystick = false; enSeleccionMotor = true; enviarESPNow("STOP"); mostrarMenuMotores(); }
    }
    else if (enDigitacionAngulo) { 
      if (t >= '0' && t <= '9') { if (inputAngulo.length() < 7) inputAngulo += t; mostrarIngresoAngulo(); }
      if (t == '*') { if (inputAngulo.startsWith("-")) inputAngulo.remove(0, 1); else inputAngulo = "-" + inputAngulo; mostrarIngresoAngulo(); }
      if (t == 'D') { enDigitacionAngulo = false; enSeleccionMotor = true; mostrarMenuMotores(); }
      if (t == '#') {
        if (inputAngulo.length() > 0) {
          enviarESPNow("GOTO_M" + String(motorSeleccionado) + ":" + inputAngulo);
          mostrarTexto("ENVIADO!", inputAngulo + " Grados"); delay(800);
        }
        enDigitacionAngulo = false; enSeleccionMotor = true; mostrarMenuMotores();
      }
    }
  }
}