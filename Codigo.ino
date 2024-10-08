#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

const byte ROWS = 4;
const byte COLS = 3;
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {3, 4, 5, 6};
byte colPins[COLS] = {7, 8, 9};
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27,16,2);
unsigned int pVoto[300][2];
int voto = 0;
unsigned int branco = 0;
String pad;
char keypressed;
const PROGMEM unsigned long senha[7] = {"senhas"};
enum palavras {partido, quantidade};

void displayVoto(unsigned int time) {
  delay(time);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(F("insira seu voto"));
}

void escreverNaEEPROM(int address, unsigned int number) {
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

void salvar() {
  int address = 4;
  escreverNaEEPROM(0, voto);
  escreverNaEEPROM(2, branco);
  for (int i = 0; i < voto; i++) {
    if (i > 250) { break; }
    escreverNaEEPROM(address, pVoto[i][partido]);
    address += 2;
    escreverNaEEPROM(address, pVoto[i][quantidade]);
    address += 2;
  }
}

unsigned int lerDaEEPROM(int address) {
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

bool somar() {
  int tamanho = lerDaEEPROM(0);
  int repeticao = 0;
  int address = 4;
  for (int i = 0; i < tamanho; i++) {
    for (int j = 0; j < voto; j++) {
      if (pVoto[j][partido] == lerDaEEPROM(address)) {
        ++repeticao;
        break;
      }
    }
    address += 4;
  }
  if ((tamanho - repeticao + voto) > 301) {
    return false;
  }
  branco += lerDaEEPROM(2);
  address = 4;
  for (int i = 0; i < tamanho; i++) {
    for (int j = 0; j < voto + 1; j++) {
      if (pVoto[j][partido] == lerDaEEPROM(address)) {
        pVoto[j][quantidade] += lerDaEEPROM(address + 2);
        break;
      }
      else if (j == voto) {
        pVoto[j][partido] = lerDaEEPROM(address);
        pVoto[j][quantidade] = lerDaEEPROM(address + 2);
        ++voto;
        break;
      }
    }
    address += 4;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print(F("inicializando"));
  displayVoto(2000);
}

void loop() {
  if (Serial.available() != 0) {
    long input = Serial.parseInt();
    while (Serial.available() != 0 ) {
      Serial.read();
    }
    if (input == pgm_read_dword(&senha[0])) {
      if (voto == 0) {
        Serial.println(F("sem votos registrados"));
      }
      unsigned int maior = 0;
      for (int i = 1; i < voto; i++) {
        if (pVoto[i][quantidade] > pVoto[maior][quantidade]) {
          maior = i;
        }
      }
      pVoto[maior][quantidade] += branco;
      for (int i = 0; i < voto; i++) {
        Serial.print(pVoto[i][partido]);
        Serial.print(F(" foi votado "));
        Serial.print(pVoto[i][quantidade]);
        Serial.println(F(" vezes"));
      } 
      pVoto[maior][quantidade] -= branco;
    }
    else if (input == pgm_read_dword(&senha[1])) {
      for (int i = 0; i <= 300; i++) {
        pVoto[i][quantidade] = 0;
      }
      voto = 0;
      branco = 0;
      Serial.println(F("resultados da votacao limpos"));
    }
    else if (input == pgm_read_dword(&senha[2])) {      
      Serial.println(F("armazenando na memoria"));
      salvar();
      Serial.println(F("armazenamento feito com sucesso"));
    }
    else if (input == pgm_read_dword(&senha[3])) {
      Serial.println(F("lendo da memoria"));
      int address = 8;
      int memoria = lerDaEEPROM(0);
      unsigned int maior = 4;
      for (int i = 1; i < memoria; i++) {
        if (lerDaEEPROM(address + 2) > lerDaEEPROM(maior + 2)) {
          maior = address;
        }
        address += 4;
      }
      address = 4;
      for (int i = 0; i < memoria; i++) {
        Serial.print(lerDaEEPROM(address));
        Serial.print(F(" foi votado "));
        if (address == maior) {
          address += 2;
          Serial.print(lerDaEEPROM(address) + lerDaEEPROM(2));
        }
        else {
          address += 2;
          Serial.print(lerDaEEPROM(address));
        }
        address += 2;
        Serial.println(F(" vezes"));
      }
      Serial.println(F("concluido"));
    }
    else if (input == pgm_read_dword(&senha[4])) {
      Serial.println(F("somando"));
      if (somar()) {
        Serial.println(F("concluido"));
      }
      else {
        Serial.println(F("sem memoria suficiente"));
      }
    }
    else {
      Serial.print(F("sem comandos registrados com o numero "));
      Serial.println(input);
    }
  }
  readKeypad();
  if (keypressed == '#') {
    unsigned int input = pad.toInt();
    unsigned long iSenha = pad.toInt();
    if (iSenha == pgm_read_dword(&senha[5])) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("armazenando"));
      salvar();
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("concluido"));
      pad = "";
      displayVoto(2000);
    }
    else if (iSenha == pgm_read_dword(&senha[6])) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("somando"));
      delay(1000);
      if (somar()) {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(F("concluido"));
      }
      else {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(F("sem memoria"));
      }
      pad = "";
      displayVoto(2000);
    }
    else if (input == 0) {
      pad = "";
      ++branco;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("voto branco"));
      displayVoto(1000);
    }
    else if (pad.toInt() > 65535) {
      pad = "";
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("nao computado"));
      displayVoto(1000);
    }
    else if (voto > 300) {
      pad = "";
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("espaco cheio"));
    }
    else {
      for (int i = 0; i <= voto; i++) {
        if (i == voto) {
          pVoto[i][partido] = input;
          ++pVoto[i][quantidade];
          ++voto;
          break;
        }
        else if (pVoto[i][partido] == input) {
          ++pVoto[i][quantidade];
          break;
        }
      }
      pad = "";
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("voto confirmado"));
      displayVoto(1000);
    }
  }
  else if (keypressed == '*') {
    pad = "";
    displayVoto(0);
  }
  lcd.setCursor(0, 0);
  lcd.print(pad);
  delay(100);
}

void readKeypad() {
  keypressed = customKeypad.getKey();
  if (keypressed != '#') {
    String konv = String(keypressed);
    pad += konv;
  }
}
