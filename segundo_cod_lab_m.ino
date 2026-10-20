
#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

const int pinADC = 33;
const int pinDAC = 25;

const uint32_t FS_MAX = 10000;
const int N = 1000;

float fSenal = 0;

uint32_t fsA = 0;
uint32_t fsB = 0;
uint32_t fsC = 0;

uint32_t fsActual = 0;
uint32_t Ts = 0;
uint32_t tiempoMuestreo = 0;

uint16_t muestraADC = 0;
uint8_t muestraDAC = 0;

char casoActual = '-';
String comando = "";

struct Muestra {
  uint32_t tiempo;
  uint16_t adc;
  uint8_t dac;
};

Muestra datos[N];

bool grabando = false;
int contador = 0;
uint32_t inicioCaptura = 0;

void mostrarMenu() {
  Serial.println();
  Serial.println("LABORATORIO DE NYQUIST - ESP32");
  Serial.println("Ingrese la frecuencia en Hz.");
  Serial.println("A, B, C: seleccionar caso");
  Serial.println("G: guardar 1000 muestras");
  Serial.println("M: mostrar menu");
}

uint32_t calcularFs(float frecuencia, float factor) {
  double resultado = (double)frecuencia * factor;

  if (!isfinite(resultado) || resultado < 1.0 ||
      resultado > FS_MAX) {
    return 0;
  }

  return (uint32_t)round(resultado);
}

void imprimirFs(char caso, uint32_t fs) {
  Serial.print("Caso ");
  Serial.print(caso);
  Serial.print(" - Fs = ");

  if (fs == 0) {
    Serial.println("No disponible");
  } else {
    Serial.print(fs);
    Serial.println(" Hz");
  }
}

void calcularFrecuencias(float frecuencia) {

  if (!isfinite(frecuencia) || frecuencia <= 0) {
    Serial.println("Ingrese una frecuencia positiva.");
    return;
  }

  fSenal = frecuencia;

  fsA = calcularFs(fSenal, 10.0f);
  fsB = calcularFs(fSenal, 2.2f);
  fsC = calcularFs(fSenal, 1.5f);

  fsActual = 0;
  casoActual = '-';

  dacWrite(pinDAC, 128);

  Serial.println();
  Serial.println("FRECUENCIAS CALCULADAS");

  Serial.print("Frecuencia original: ");
  Serial.print(fSenal, 3);
  Serial.println(" Hz");

  imprimirFs('A', fsA);
  imprimirFs('B', fsB);
  imprimirFs('C', fsC);

  Serial.println("Seleccione A, B o C.");
}

void seleccionarCaso(char caso) {

  if (fSenal <= 0) {
    Serial.println("Primero ingrese una frecuencia.");
    return;
  }

  uint32_t fsSeleccionada = 0;

  if (caso == 'A') fsSeleccionada = fsA;
  if (caso == 'B') fsSeleccionada = fsB;
  if (caso == 'C') fsSeleccionada = fsC;

  if (fsSeleccionada == 0) {
    Serial.println("Frecuencia de muestreo no disponible.");
    return;
  }

  fsActual = fsSeleccionada;
  Ts = (uint32_t)round(1000000.0 / fsActual);

  casoActual = caso;
  tiempoMuestreo = micros() + Ts;

  Serial.println();
  Serial.print("CASO ");
  Serial.println(casoActual);

  Serial.print("Frecuencia de entrada: ");
  Serial.print(fSenal, 3);
  Serial.println(" Hz");

  Serial.print("Frecuencia de muestreo: ");
  Serial.print(fsActual);
  Serial.println(" Hz");

  Serial.print("Periodo de muestreo: ");
  Serial.print(Ts);
  Serial.println(" us");

  Serial.print("Frecuencia de Nyquist: ");
  Serial.print(fsActual / 2.0f, 3);
  Serial.println(" Hz");

  if (caso == 'A') {
    Serial.println("Muestreo adecuado.");
  }

  if (caso == 'B') {
    Serial.println("Cerca del limite de Nyquist.");
  }

  if (caso == 'C') {
    float fAlias = fabsf(fSenal - fsActual);

    Serial.println("Aliasing.");

    Serial.print("Frecuencia alias teorica: ");
    Serial.print(fAlias, 3);
    Serial.println(" Hz");
  }

  Serial.println("Escriba G para registrar las muestras.");
}

void iniciarCaptura() {

  if (fsActual == 0) {
    Serial.println("Primero seleccione A, B o C.");
    return;
  }

  contador = 0;
  grabando = true;

  inicioCaptura = micros();
  tiempoMuestreo = inicioCaptura + Ts;

  Serial.println();
  Serial.println("Capturando 1000 muestras...");
}

void exportarCSV() {

  Serial.print("CSV_BEGIN,");
  Serial.print(casoActual);
  Serial.print(",");
  Serial.print(fSenal, 3);
  Serial.print(",");
  Serial.println(fsActual);

  Serial.println(
    "Caso,Frecuencia_entrada_Hz,Fs_Hz,"
    "Tiempo_s,ADC,DAC,Voltaje_ADC_V"
  );

  for (int i = 0; i < N; i++) {

    Serial.print(casoActual);
    Serial.print(",");

    Serial.print(fSenal, 3);
    Serial.print(",");

    Serial.print(fsActual);
    Serial.print(",");

    Serial.print(datos[i].tiempo / 1000000.0f, 6);
    Serial.print(",");

    Serial.print(datos[i].adc);
    Serial.print(",");

    Serial.print(datos[i].dac);
    Serial.print(",");

    float voltaje = datos[i].adc * 3.3f / 4095.0f;
    Serial.println(voltaje, 4);
  }

  Serial.println("CSV_END");
  Serial.println("Captura finalizada.");
}

void procesarComando(String texto) {

  texto.trim();
  texto.toUpperCase();

  if (texto.length() == 0) return;

  if (texto == "M") {
    mostrarMenu();
    return;
  }

  if (texto == "G") {
    iniciarCaptura();
    return;
  }

  if (texto == "A" || texto == "B" || texto == "C") {
    seleccionarCaso(texto.charAt(0));
    return;
  }

  if (texto.startsWith("F=")) {
    texto.remove(0, 2);
    texto.trim();
  }

  char *fin;
  float frecuencia = strtof(texto.c_str(), &fin);

  if (fin == texto.c_str() || *fin != '\0' ||
      !isfinite(frecuencia)) {
    Serial.println("Comando invalido.");
    return;
  }

  calcularFrecuencias(frecuencia);
}

void leerSerial() {

  while (Serial.available() > 0) {

    char caracter = Serial.read();

    if (caracter == '\n' || caracter == '\r') {

      if (comando.length() > 0) {
        procesarComando(comando);
        comando = "";
      }

    } else if (comando.length() < 30) {
      comando += caracter;
    }
  }
}

void setup() {

  Serial.begin(115200);
  delay(1500);

  pinMode(pinADC, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(pinADC, ADC_11db);

  dacWrite(pinDAC, 128);

  comando.reserve(32);

  mostrarMenu();
}

void loop() {

  if (!grabando) {
    leerSerial();
  }

  if (fsActual > 0) {

    uint32_t ahora = micros();

    if ((int32_t)(ahora - tiempoMuestreo) >= 0) {

      tiempoMuestreo += Ts;

      if ((int32_t)(ahora - tiempoMuestreo) >= 0) {
        tiempoMuestreo = ahora + Ts;
      }

      uint32_t instante = micros();

      muestraADC = analogRead(pinADC);
      muestraDAC = muestraADC >> 4;

      dacWrite(pinDAC, muestraDAC);

      if (grabando && contador < N) {

        datos[contador].tiempo =
          instante - inicioCaptura;

        datos[contador].adc = muestraADC;
        datos[contador].dac = muestraDAC;

        contador++;

        if (contador >= N) {
          grabando = false;
          exportarCSV();

          tiempoMuestreo = micros() + Ts;
        }
      }
    }
  }
}
