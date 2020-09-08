double iterator;
double freq1 = 0.05;
double freq2 = 0.1;
double amp1 = 25;
double amp2 = 15;
double register_period = 500.0; // in ms

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  iterator = 0;

}

void loop() {
  int i, res_length;
  int transmission_length = 10;
  float x;
  float frq_mod;
  String res;

  x = amp1 * sin(iterator * freq1 * 2 * 3.1415) +  amp2 * cos(iterator * freq2 * 2 * 3.1415);

  iterator = iterator + (register_period / 1000.0);

  res = String(x, 3);
  res_length = res.length();

  for (i = 1; i <= transmission_length - res_length; i++) {
    res.concat(" ");    
  }
  int bytes_sent = Serial.print(res);  
  
  delay(register_period);
}
