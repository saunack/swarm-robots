const float OUTER_RANGE = 80;
const float INNER_RANGE = 30;
const float CONFIRM_RANGE = 75;
float rcv[6] = {1, 1, 1, 1, 1, 1};

int minval[6], maxval[6];
int mode, tail_pin, head_pin;
//4 modes:
//  0 -> Idle
//  1 -> Following
//  2 -> Transmitting
//  3 -> Idle, waiting for back transmission

int right(int pin) {
  return (pin + 1) % 6;
}
int left(int pin) {
  return (pin + 5) % 6;
}

float calib(int reading, int pin) {
  if (reading > maxval[pin])  maxval[pin] = reading;
  if (reading < minval[pin])  minval[pin] = reading;

  if (maxval[pin] - minval[pin] < 300) {
    Serial.print("Pin: "); Serial.print(pin); Serial.println(" uncalibrated");
    return 0;
  }
  return (100.0 * ((float) (reading - minval[pin]) / (float) (maxval[pin] - minval[pin])));
}

int Update() {
  Serial.print(mode); Serial.print(": ");
  Serial.print("Input - ");
  for (int i = 0; i < 6; i++) {
    int buff = analogRead(i);
    rcv[i] = calib(buff, i);
    Serial.print(rcv[i]); Serial.print("  ");
    //Serial.print(buff); Serial.print("  ");
  }
  Serial.println();

  int x = 0;
  for (int i = 0; i < 6; i++)  if (rcv[i] < rcv[x]) x = i;
  return x;
}

void changeMode(int newmode) {
  mode = newmode;
  Serial.print("Mode changed to "); Serial.println(mode);
  switch (mode) {
    case 0: digitalWrite(2, HIGH); digitalWrite(3, HIGH); break;
    case 1: digitalWrite(2, LOW); digitalWrite(3, LOW); break;
    case 2: digitalWrite(2, LOW); digitalWrite(3, HIGH); break;
    case 3: digitalWrite(2, HIGH); digitalWrite(3, LOW); break;
  }
  digitalWrite(4, LOW);   digitalWrite(5, LOW);    digitalWrite(6, LOW);    digitalWrite(7, LOW);
  delay(1000);
}

void follow() {
  if (rcv[head_pin] > OUTER_RANGE) {
    digitalWrite(4, LOW);    digitalWrite(5, LOW);    digitalWrite(6, LOW);    digitalWrite(7, LOW);
    return;
  }
  /*  else if ((head_pin == 0 || head_pin == 3) && rcv[head_pin] > (INNER_RANGE + OUTER_RANGE)/2){
  	if(rcv[right(head_pin)] < rcv[left(head_pin)])
  		{digitalWrite(4,HIGH);          digitalWrite(5,LOW);          digitalWrite(6,HIGH);          digitalWrite(7,LOW);}
  	else
  		{digitalWrite(4,LOW);          digitalWrite(5,HIGH);          digitalWrite(6,LOW);          digitalWrite(7,HIGH);}
  	delay(250);
    }
    else*/
  switch (head_pin) {
    //pins 4,5 are for the right motor and 6,7 for the left.
    //sensor 0 is the forward orientation and numbering is CC.
    //                     0
    // 6             5           1                4
    // 7             4           2                5
    //                     3
    case 0:
      digitalWrite(4, HIGH);          digitalWrite(5, LOW);          digitalWrite(6, LOW);          digitalWrite(7, HIGH);
      delay(500);
      break;
    case 1:
      digitalWrite(4, HIGH);          digitalWrite(5, LOW);          digitalWrite(6, HIGH);          digitalWrite(7, LOW);
      delay(250);
      break;
    case 2:
      digitalWrite(4, LOW);          digitalWrite(5, HIGH);          digitalWrite(6, LOW);          digitalWrite(7, HIGH);
      delay(250);
      break;
    case 3:
      digitalWrite(4, LOW);          digitalWrite(5, HIGH);          digitalWrite(6, HIGH);          digitalWrite(7, LOW);
      delay(500);
      break;
    case 4:
      digitalWrite(4, HIGH);          digitalWrite(5, LOW);          digitalWrite(6, HIGH);          digitalWrite(7, LOW);
      delay(250);
      break;
    case 5:
      digitalWrite(4, LOW);          digitalWrite(5, HIGH);          digitalWrite(6, LOW);          digitalWrite(7, HIGH);
      delay(250);
      break;
  }
  digitalWrite(4, LOW);    digitalWrite(5, LOW);    digitalWrite(6, LOW);    digitalWrite(7, LOW);
  return;
}

bool confirm(int pin) {
  int count = 0;
  float reading;

  Serial.println("Confirmation signal");

  if (mode == 0 || mode == 1) {
    long unsigned int t = millis();

    digitalWrite(pin + 8, HIGH);
    delay(100);

    for (count = 0, t = millis(); millis() - t < 100;) {
      if (count == 10)  continue;
      reading = calib(analogRead(pin), pin);

      Serial.print(reading); Serial.print("  ");
      if (reading > CONFIRM_RANGE) count++;
      else  (count > 0 ? count-- : 0);
    }
    Serial.println();
    if (count < 10) {
      Serial.println("Exited at 1st");
      digitalWrite(pin + 8, LOW);
      return false;
    }

    digitalWrite(pin + 8, LOW);
    delay(100);

    for (count = 0, t = millis(); millis() - t < 100;) {
      if (count == 10)  continue;
      reading = calib(analogRead(pin), pin);

      Serial.print(reading); Serial.print("  ");
      if (reading < OUTER_RANGE) count++;
      else  (count > 0 ? count-- : 0);
    }
    Serial.println();
    if (count < 10) {
      Serial.println("exited at 2");
      return false;
    }

    digitalWrite(pin + 8, HIGH);
    delay(100);
    digitalWrite(pin + 8, LOW);
    return true;
  }

  else if (mode == 2) {
    long unsigned int t = millis();

    digitalWrite(pin + 8, LOW);
    delay(100);

    for (count = 0, t = millis(); millis() - t < 100;) {
      if (count == 10)  continue;
      reading = calib(analogRead(pin), pin);

      Serial.print(reading); Serial.print("  ");
      if (reading > CONFIRM_RANGE) count++;
      else  (count > 0 ? count-- : 0);
    }
    Serial.println();
    if (count < 10) {
      Serial.println("Exited at 1st");
      return false;
    }

    digitalWrite(pin + 8, HIGH);
    delay(100);

    for (t = millis(); millis() - t < 100;) {
      if (count == 10)  continue;
      reading = calib(analogRead(pin), pin);

      Serial.print(reading); Serial.print("  ");
      if (reading < OUTER_RANGE) count++;
      else  (count > 0 ? count-- : 0);
    }
    Serial.println();
    if (count < 10) {
      Serial.println("Exited at 2nd");
      return false;
    }
    else          return true;
  }
}

bool isComm(int pin) {
  int count = 0, negcount = 0;

  Serial.print(mode); Serial.print(":  ");  Serial.print("Checking for comm link on "); Serial.print(pin);  Serial.print("- ");

  if (mode == 2) {
    digitalWrite(pin + 8, HIGH);
    delay(100);
    float reading;
    for (long unsigned int t = millis(); millis() - t < 100;) {
      reading = calib(analogRead(pin), pin);
      Serial.print(reading); Serial.print("  ");

      if (reading < OUTER_RANGE) {
        if (count < 10)  count++;
        else if (confirm(pin)) {
          digitalWrite(pin + 8, LOW);
          Serial.println("PASS");
          return true;
        }
        else break;
      }

      else {
        if (count > 1) count--;
        else  count = 0;
      }
    }

    digitalWrite(pin + 8, LOW);
    Serial.println("FAIL");
    return false;
  }

  else if (mode == 0 || mode == 1) {
    float reading;
    for (long unsigned int t = millis(); millis() - t < 250;) {
      reading = calib(analogRead(pin), pin);
      Serial.print(reading); Serial.print("  ");

      if (reading < OUTER_RANGE) {
        if (count < 10)  count++;
        else if (confirm(pin)) {
          digitalWrite(pin, LOW);
          Serial.println("PASS");
          return true;
        }
        else break;
      }

      else {
        if (count > 1) count--;
        else  count = 0;
      }
    }

    Serial.println("FAIL");
    return false;
  }
}

void setup() {
  DDRB = B111111;
  PORTB = B000000;
  pinMode(2, OUTPUT); digitalWrite(2, LOW);
  pinMode(3, OUTPUT); digitalWrite(3, LOW);
  pinMode(4, OUTPUT);  pinMode(5, OUTPUT);  pinMode(6, OUTPUT);  pinMode(7, OUTPUT);
  digitalWrite(4, LOW);  digitalWrite(5, LOW);  digitalWrite(6, LOW);  digitalWrite(7, LOW);
  Serial.begin(115200);
  maxval[0] = maxval[1] = maxval[2] = maxval[3] = maxval[4] = maxval[5] = 0;
  minval[0] = minval[1] = minval[2] = minval[3] = minval[4] = minval[5] = 1000;

  //maxval[0] = maxval[1] = maxval[2] = maxval[3] = maxval[4] = maxval[5] = 1000;
  minval[0] = minval[1] = minval[2] = minval[3] = minval[4] = minval[5] = 50;
  while (rcv[0] + rcv[1] + rcv[2] + rcv[3] + rcv[4] + rcv[5] < 95 * 6)
    Update();
  changeMode(0);

  Serial.println("Initialised");
}

void loop() {
  int i = 0;
  
  if (mode == 0) {
    int count = 0;
    while (count < 20) {
      head_pin = Update();
      count++;
      if (isComm(head_pin)) {
        changeMode(1);
        return;
      }
    }
    Serial.println("Moving");

    digitalWrite(4, HIGH);          digitalWrite(5, LOW);          digitalWrite(6, HIGH);          digitalWrite(7, LOW);
    delay(250);
    digitalWrite(4, LOW);          digitalWrite(5, LOW);          digitalWrite(6, LOW);          digitalWrite(7, LOW);
  }

  else if (mode == 1) {
    int count = 0, negcount = 0, history = Update();
    while (true) {
      head_pin = Update();
      int last_move = head_pin;
      while (rcv[head_pin] <= INNER_RANGE) {
        head_pin = Update();
        if (count < 4)  count++;
        else if (isComm(head_pin)) {
          digitalWrite(head_pin + 8, HIGH);
          changeMode(2);
          digitalWrite(head_pin + 8, LOW);
          return;
        }
        /*else if((history + 3) % 6 == head_pin || (head_pin + 3) % 6 == history){
          head_pin = history;
          digitalWrite(head_pin + 8, HIGH);
          changeMode(2);
          digitalWrite(head_pin + 8, LOW);
          return;
        }*/
        else  follow();
        history = head_pin;
      }
      count = 0;

   /*   if((history + 3) % 6 == head_pin || (head_pin + 3) % 6 == history){
          head_pin = history;
          digitalWrite(head_pin + 8, HIGH);
          changeMode(2);
          digitalWrite(head_pin + 8, LOW);
          return;
        }
*/
      if (rcv[head_pin] <= OUTER_RANGE) {
        last_move = rcv[head_pin];
        history = head_pin;
        follow();
        negcount = 0;
        continue;
      }
      else negcount++;

      rcv[head_pin] = last_move;
      follow();

      if (negcount == 10000) {
        changeMode(0);
        return;
      }
    }
  }

  else if (mode == 2) {
    Serial.print("Minimum: "), Serial.println(head_pin);
    for (int count = 0; count < 4; count++)  for (i = right(head_pin); i != head_pin; i = right(i)) {

        Serial.print("Check starting on ");  Serial.println(i); 
        Serial.println();Serial.println();Serial.println();Serial.println();Serial.println();
        if (isComm(i)) {
          tail_pin = i;
          rcv[tail_pin] = calib(analogRead(tail_pin), tail_pin);
          digitalWrite(tail_pin + 8, HIGH);

          for (int negcount = 0; negcount < 1000; negcount++) {
            if (rcv[tail_pin] > INNER_RANGE)	Update();
            else if (isComm(tail_pin)){
              digitalWrite(tail_pin + 8, LOW);
              changeMode(3);
              return;
            }
          }
          digitalWrite(tail_pin + 8, LOW);
        }
      }
    if (i == head_pin) {                                                    //if i = 6, should be the last bot. start sending the signals back along the line
      digitalWrite(head_pin + 8, HIGH);
      changeMode(0);
      digitalWrite(head_pin + 8, LOW);
      return;
    }
  }

  else if (mode == 3) {                                              //the first bot will presumably have a different code for this part becuase it has no bot in front of it.
    float Read;                                                      //Alt : you can set the value of minm_led to negative to identify the first bot
    for (int negcount = 0; negcount < 100000; negcount++) {                                                   //is CLOSE ENOUGH too strong a check? due to hardware imprefections, the value may change to outside CLOSE ENOUGH
      Read = calib(analogRead(tail_pin), tail_pin);
      if (Read < (INNER_RANGE + OUTER_RANGE)/2) {
        digitalWrite(head_pin + 8, HIGH);
        changeMode(0);
        digitalWrite(head_pin + 8, LOW);
        head_pin = tail_pin = 0;
        return;
      }
    }
    digitalWrite(head_pin + 8, HIGH);
    changeMode(0);
    digitalWrite(head_pin + 8, LOW);
    head_pin = tail_pin = 0;
  }
  else  Serial.print("Error");
}
