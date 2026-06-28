class Semaforo {
  const int estados = 3; // quantidade de estados
  int estado = 0; // estado atual
  const int ledsQuant = 3; // quantidade de leds
  int leds[3]; // porta dos leds
public:
  void setPortas(int portas[3]) {
    // configura as portas dos leds para o semaforo
    // portas: [0] = verde; [1] = amarelo; [2] = vermelho
    for (int i = 0; i < ledsQuant; i++) {
      leds[i] = portas[i];
      pinMode(leds[i], OUTPUT);
    }
  }
  void setEstado(int novoEstado) {
    // seleciona um novo estado para o semaforo. 0 = desligado; 1 = verde; 2 = amarelo; 3 = vermelho
    estado = 0;
    if (novoEstado > estados) {
      novoEstado = 1;
    }
    for (int i = 0; i < novoEstado; i++) {
      proxEstado();
    }
  }
    int getEstado() {
      return estado;
    }
  void desligarLeds() {
    // desliga todos os leds
    for (int i = 0; i < ledsQuant; i++) {
      digitalWrite(leds[i], LOW);
    }
  }
    void proxEstado() {
    // passa o semaforo para o proximo estado
    estado++;
    if (estado > estados) {
      estado = 1;
    }
    desligarLeds();
    digitalWrite(leds[estado-1], HIGH);
  }
};
 
class SensorUltrassonico {
public:
  int trigger, echo;
  void setPortas(int trigger, int echo) {
    this -> trigger = trigger;
    this -> echo = echo;
    pinMode(trigger, OUTPUT);
    pinMode(echo, INPUT);
  }
  void ativarPulso() {
    digitalWrite(trigger, LOW); 
    delayMicroseconds(5);
    digitalWrite(trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);   
  }
  double getDistancia() {
    ativarPulso();
    return (pulseIn(echo, HIGH) * 0.0171); // retorna em centimetros
  }
};

class Cruzamento {
  Semaforo semaforos[2];
  SensorUltrassonico sensores[2];
  const int estados = 6; // quantidade de estados
  int estado = 0;
public:
  void setSemaforos(int portasSemaforo1[3], int portasSemaforo2[3]) {
    semaforos[0].setPortas(portasSemaforo1);
    semaforos[1].setPortas(portasSemaforo2);
  }
  void setSensores(int portasSensor1[2], int portasSensor2[2]) {
    sensores[0].setPortas(portasSensor1[0], portasSensor1[1]);
    sensores[1].setPortas(portasSensor2[0], portasSensor2[1]);
  }
  SensorUltrassonico* getSensores() {
    return sensores;
  }
  void setEstado(int novoEstado) {
    /*
    seleciona um novo estado para o cruzamento
    
    estado = 0 | desligado
    estado = 1 | verde-vermelho (1-3) - vermelho geral
    estado = 2 | amarelo-vermelho (2-3) - entreverdes (amarelo)
    estado = 3 | vermelho-vermelho (3-3) - entreverdes (vermelho)
    estado = 4 | vermelho-verde (3-1) - vermelho geral
    estado = 5 | vermelho-amarelo (3-2) - entreverdes (amarelo)
    estado = 6 | vermelho-vermelho (3-3) - entreverdes (vermelho)
    
    estado 1-3 = estagio 1
    estado 4-6 = estagio 2
    */
    estado = 1;
    if (novoEstado > estados) {
      novoEstado = 1;
    }
    if (novoEstado <= 0) {
      semaforos[0].desligarLeds();
      semaforos[1].desligarLeds();
            return;
    }
    semaforos[0].setEstado(1);
    semaforos[1].setEstado(3);
    for (int i = 0; i < novoEstado - 1; i++) {
      proxEstado();
    }
  }
  int getEstado() {
    return estado;
  }
  void proxEstado() {
    // passa para o proximo estado do cruzamento      
        int estado1 = semaforos[0].getEstado();
    int estado2 = semaforos[1].getEstado();
    if (estado1 !=  3 || estado2 == 3 && estado == 6) {
      semaforos[0].proxEstado();
    }
    if (estado2 !=  3 || estado1 == 3 && estado == 3) {
      semaforos[1].proxEstado();
    }
        estado++;
    if (estado > estados) {
      estado = 1;
    }
  }
  void iniciarCicloFixo(int temporizador[6]) {
    /*
    temporizador[0] = vermelho geral 1
    temporizador[1] = entreverdes (amarelo) 1
    temporizador[2] = entreverdes (vermelho) 1
    temporizador[3] = vermelho geral 2
    temporizador[4] = entreverdes (amarelo) 2
    temporizador[5] = entreverdes (vermelho) 2
    */
    setEstado(1);
    for (int i = 0; i < estados; i++) {
            delay(temporizador[i]);
      proxEstado();
    }
  }
  void iniciarCicloUltrassonico(int vias[2][7]) { // algum problema desconhecido
    /*
    vias[semaforo][x]
        x = 0 = tempo(milisec) de verde minimo
        x = 1 = tempo(milisec) de verde maximo
        x = 2 = tempo(milisec) de amarelo
        x = 3 = tempo(milisec) de vermelho
        x = 4 = distancia(cm) entre o sensor e o comeco da calcada
        x = 5 = distancia (cm) entre o sensor e o fim da calcada
        x = 6 = tempo(milisec) sem carros para se considerar ociosa
    */
    
    for (int semaforo = 0; semaforo < 2; semaforo++) {
      unsigned long timerInicio = millis();
      unsigned long timerSensor = millis(); // ultima vez que um carro passou
      unsigned long tempoAberto; // tempo de sinal verde

      setEstado(1 + semaforo * 3);

      double sensor;
      do {
        sensor = sensores[semaforo].getDistancia();
        if (vias[semaforo][4] < sensor && sensor < vias[semaforo][5]) {
          timerSensor = millis();
          Serial.print(vias[semaforo][4]);
          Serial.print(" < ");
          Serial.print(sensor);
          Serial.print(" < ");
          Serial.println(vias[semaforo][5]);
        };
        
        tempoAberto = millis() - timerInicio;
      } while ((unsigned long)tempoAberto < vias[semaforo][0] || (unsigned long)tempoAberto < vias[semaforo][1] && (unsigned long)(millis() - timerSensor) < vias[semaforo][6]);

      Serial.println(tempoAberto);
      
      proxEstado();
      delay(vias[semaforo][2]); // tempo amarelo
      proxEstado();
      delay(vias[semaforo][3]); // tempo vermelho
    }
  }
};


Cruzamento cruzamento;

void setup() {
  Serial.begin(9600);
    
  int semaforo1[3] = {2, 3, 4};
  int semaforo2[3] = {5, 6, 7};
  cruzamento.setSemaforos(semaforo1, semaforo2);

  int sensor1[2] = {8, 9};
  int sensor2[2] = {10, 11};
  cruzamento.setSensores(sensor1, sensor2);
}

int temporizadores[6] = {1000, 1000, 1000, 1000, 1000, 1000};
int configuracoes[2][7] =   {
  {3000, 15000, 1000, 1000, 0, 15, 1500},
  {3000, 15000, 1000, 1000, 0, 15, 1500}
};

void loop() {
  cruzamento.iniciarCicloUltrassonico(configuracoes);
}