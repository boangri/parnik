int EN = 2;
byte pass1[] = {1,1,1,1,1,1};
byte pass2[] = {2,2,2,2,2,2};
byte myaddr = 33;
const int BSIZE=128;
byte buf[BSIZE];
byte obuf[BSIZE];
boolean sessionOpen = false;
unsigned long time = 0;
struct {
  float temp1;
  float temp2;
  float hum1;
  float temp_lo;
  float temp_hi;
  float temp_pump;
  float volt;
  float sigma;
  float vol;
  float dist;
  int fans;
  int pump;
} typedef Parnik;

void RS485_setup(Parnik *pp)
{
  Serial1.begin(9600);
  pinMode(EN,OUTPUT);
  digitalWrite(EN,LOW); 
}  
  
void RS485(Parnik *pp)
{
  int i; /* received bytes counter */
  unsigned short crc;
  byte *pass;
  boolean passOk = true;
  unsigned long t, lastReceived; /* when last byte was received - for 5 msec timeout*/
  byte * bp;
  float var;
  int ivar;
  
  i = 0;
  do {
    while (Serial1.available() > 0) {
      if (i >= sizeof(buf)) {
        Serial.println("Buffer overflow");
        return; /* buffer overflow */
      }  
      buf[i++] = Serial1.read();
      lastReceived = millis();
    }  
    delay(1);
  } while (millis() - lastReceived <= 5);
  
  if (buf[0] != myaddr) {
    //Serial.println("Not for me");
    return;
  }
  /* calculate CRC */
  crc = crc16.crc(buf, i);
  if (crc != 0) {
    Serial.println("Invalid CRC");
    return; /* CRC error */
  }
  

  i = 0;
  obuf[i++] = myaddr;
  switch (buf[1]) {
    case 0:
      obuf[i++] = 0x00;
      break;
    case 1:
      if (buf[2] == 1) {
        pass = pass1;
      } else if (buf[2] == 2) {
        pass = pass2;
      } else {
        Serial.println("Invalid level");
        return;
      }  
      for (int j = 0; j < 6; j++) {
        if (pass[j] != buf[j+3]) {
           Serial.println ("Invalid password");
           return;
        }
      }
      if (passOk) {
         sessionOpen = true;
         time = millis();
         obuf[i++] = 0;
      }
      break;   
    case 2:
      sessionOpen = false;
      obuf[i++] = 0;
      break;  
    case 4: /* read parameters */
      if (!sessionOpen || (millis() - time > 20000)) { 
        Serial.println("Session closed");
        return; 
      }
      time = millis(); /* restart timer */
      
      switch (buf[2]) {
      case 1: /* temperatures*/
        var = (pp->temp1 + 30.)*100.;
        ivar = (int)var;  
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        var = (pp->temp2 + 30.)*100.;
        ivar = (int)var;  
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        var = (pp->temp_hi + 30.)*100.;
        ivar = (int)var;  
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        var = (pp->temp_pump + 30.)*100.;
        ivar = (int)var;  
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        break;
        /*
      case 2: //* humidities 
        ;
        break;
        */
      case 3: //* motor states 
        obuf[i++] = pp->fans;
        obuf[i++] = pp->pump;
        break;
      case 4: //* power 
        pp->volt = voltage.getAverage();
        var = (pp->volt)*100.;
        ivar = (int)var;
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        break;     
      case 5: //* water 
        var = (pp->vol)*100.;
        ivar = (int)var;
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        var = (pp->dist)*100.;
        ivar = (int)var;
        bp = (byte *)&ivar;
        for (int j = 0; j < 2; j++) {
          obuf[i++] = *bp++;
        }  
        break;    
      }  
      break; 
    default:
      obuf[i++] = 0x00;
      break;  
  }
  
  crc = crc16.crc(obuf, i);
  obuf[i++] = crc & 0xff;
  obuf[i++] = (crc >> 8) & 0xff;
  
  while ((t = millis()) - lastReceived <= 6) { // make sure a gap at least 6 msec
    delay(1);
  }  
  digitalWrite(EN,HIGH);
  Serial1.write(obuf, i);
  Serial1.flush();
  digitalWrite(EN,LOW);  
  //Serial.println(t - lastReceived, DEC); // how much it took to answer
} 

