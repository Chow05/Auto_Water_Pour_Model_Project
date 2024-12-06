#include <LiquidCrystal.h>

const int B_Start = A0;
const int B_Stop = A1;
const int Trig = 5;
const int Echo = 4;
const int CB_LuuLuong = 3;
const int ENA = 6;
const int IN1 = 0;
const int IN2 = 2;
LiquidCrystal LCD(7, 8, 9, 10, 11, 12);

const float KC_CB_toi_Coc = 3.55;   // Đơn vị cm, giá trị x10 so với thực tế để mô phỏng
const float ChieuCaoCoc = 10;  // Đơn vị cm, giá trị x10 so với thực tế để mô phỏng

int TrangThaiHoatDong = 0;
float MucNuoc = 0.0;     // Đơn vị %
volatile int XungVaoLL;  // Đo số xung của YF-S401
float LuuLuong = 0.0;
uint32_t T0;
uint32_t T1;
int CS_Bom = 0;

////////////////////////////////////////////////////////////////////////////////////////////
void HamDelay(uint32_t TimeNow, uint32_t TimeDelay) {
  uint32_t Time = millis();
  while (Time - TimeNow < TimeDelay)
    Time = millis();
}

void DoMucNuoc() {
  uint32_t ThoiGian;
  float KhoangCach;
  float VT_AmThanh = (343.2 * 100) / (1 * 1000000);  // 343.2 m/s => 0.03432 cm/us

  digitalWrite(Trig, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);  // xung có độ dài 5 microSeconds
  digitalWrite(Trig, LOW);

  ThoiGian = pulseIn(Echo, HIGH);              // Đo độ rộng xung HIGH ở chân Echo
  KhoangCach = (ThoiGian / 2.0) * VT_AmThanh;  // Tính khoảng cách đến vật
  MucNuoc = 100 - 100.0 * (KhoangCach - KC_CB_toi_Coc) / ChieuCaoCoc;

  Serial.print("MucNuoc: ");
  Serial.print(MucNuoc, 1);
  Serial.println("%");
}

void Xung() {
  XungVaoLL++;
}
void DoLuuLuong() {
  T0 = millis();
  if (T0 >= (T1 + 1000.0)) {
    T1 = T0;
    if (XungVaoLL != 0) {
      LuuLuong = XungVaoLL / 98.0;  // F = 98Q, Q (L/Min)
      XungVaoLL = 0;

      Serial.print("LuuLuong: ");
      Serial.print(LuuLuong, 1);
      Serial.println(" L/M");
    }
  }
}

void DieuKhienBom(float Set_LL) {
  if (LuuLuong < Set_LL) {
    if (CS_Bom < 255)
      analogWrite(ENA, CS_Bom += 5);
    else
      analogWrite(ENA, CS_Bom = 255);
  } else if (LuuLuong > Set_LL) {
    if (CS_Bom > 0)
      analogWrite(ENA, CS_Bom -= 5);
    else
      analogWrite(ENA, CS_Bom = 0);
  } else
    analogWrite(ENA, CS_Bom);
}

void XuatLCD() {
  LCD.setCursor(0, 0);
  LCD.print("FLOW:");
  LCD.setCursor(5, 0);
  LCD.print("   ");
  LCD.setCursor(5, 0);
  LCD.print(LuuLuong, 1);
  LCD.setCursor(8, 0);
  LCD.print("L/M |PWM");
  LCD.setCursor(0, 1);
  LCD.print("LEVEL:");
  LCD.setCursor(6, 1);
  LCD.print("    ");
  LCD.setCursor(6, 1);
  LCD.print(MucNuoc, 1);
  LCD.setCursor(10, 1);
  LCD.print("% |");
  LCD.setCursor(13, 1);
  LCD.print("   ");
  LCD.setCursor(13, 1);
  LCD.print(CS_Bom);
  HamDelay(millis(), 300);
}

void setup() {

  Serial.begin(9600);

  pinMode(B_Start, INPUT_PULLUP);
  pinMode(B_Stop, INPUT_PULLUP);
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(CB_LuuLuong, INPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(CB_LuuLuong, HIGH);
  attachInterrupt(digitalPinToInterrupt(CB_LuuLuong), Xung, RISING);  // Cài đặt ngắt ngoài
  T0 = millis();
  T1 = T0;

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);

  LCD.begin(16, 2);
  LCD.clear();
  XuatLCD();
}

void loop() {

  if (digitalRead(B_Start) == 0)
    TrangThaiHoatDong = 1;
  if (digitalRead(B_Stop) == 0)
    TrangThaiHoatDong = 0;
  Serial.println(TrangThaiHoatDong);

  if (TrangThaiHoatDong == 1) {
    CS_Bom = 125;
    while (MucNuoc < 80.0 && TrangThaiHoatDong == 1) {
      if (digitalRead(B_Stop) == 0)
        TrangThaiHoatDong = 0;
      digitalWrite(IN1, HIGH);
      DoMucNuoc();
      DoLuuLuong();
      XuatLCD();
      if (MucNuoc < 50.0)
        DieuKhienBom(0.4);
      if (MucNuoc >= 50.0 && MucNuoc < 60.0)
        DieuKhienBom(0.3);
      if (MucNuoc >= 60.0 && MucNuoc < 70.0)
        DieuKhienBom(0.2);
      if (MucNuoc >= 70.0 && MucNuoc < 80.0)
        DieuKhienBom(0.1);
    }
    if (MucNuoc >= 80.0) {
      CS_Bom = 0;
      digitalWrite(IN1, LOW);
      analogWrite(ENA, 0);
      TrangThaiHoatDong = 0;
    }
  }

  if (TrangThaiHoatDong == 0) {
    CS_Bom = 0;
    MucNuoc = 0.0;
    LuuLuong = 0.0;

    digitalWrite(IN1, LOW);
    analogWrite(ENA, CS_Bom);
    DoMucNuoc();
    DoLuuLuong();
    XuatLCD();
  }
}