#include <Wire.h>
#include <MCP342x.h>
#include <LiquidCrystal.h>

enum range {
  R_100uV,
  R_1mV,
  R_10mV,
  R_100mV,
  R_1V,
  R_10V,
  R_100V,
  R_1000V,
  R_100pA,
  R_1nA,
  R_10nA,
  R_100nA,
  R_1uA,
  R_10uA,
  R_100uA,
  R_1000uA,
  R_10mA,
  R_ERR,
};

static const char *range_names[] = {
  "100uV ",
  "1mV   ",
  "10mV  ",
  "100mV ",
  "1V    ",
  "10V   ",
  "100V  ",
  "1000V ",
  "100pA ",
  "1nA   ",
  "10nA  ",
  "100nA ",
  "1uA   ",
  "10uA  ",
  "100uA ",
  "1000uA",
  "10mA  ",
  "ERR",
};

static const char *range_units[] = {
  "uV",
  "mV",
  "mV",
  "mV",
  "V ",
  "V ",
  "V ",
  "V ",
  "pA",
  "nA",
  "nA",
  "nA",
  "uA",
  "uA",
  "uA",
  "uA",
  "mA",
  "ER",
};

static uint16_t range_mul[] = {
  100,
  1,
  10,
  100,
  1,
  10,
  100,
  1000,
  100,
  1,
  10,
  100,
  1,
  10,
  100,
  1000,
  10,
};

static double range_cal[] = {
  1.0000, /* 100uV */
  1.0000, /* 1mV */
  0.9994, /* 10mV */
  0.9993, /* 100mV */
  1.0011, /* 1V */
  1.0011, /* 10V */
  1.00119, /* 100V */
  1.0000, /* 1000V */
  1.0000, /* 100uA */
  1.0000, /* 1nA */
  1.0000, /* 10nA */
  1.0000, /* 100nA */
  1.0000, /* 1uA */
  1.0000, /* 10uA */
  1.0000, /* 100uA */
  1.0000, /* 1000uA */
  1.0000, /* 10mA */
};

uint8_t address = 0x68;
MCP342x adc = MCP342x(address);

const int rs = 13, en = 12, d4 = 17, d5 = 16, d6 = 15, d7 = 14;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

static void error(const char *msg)
{
  lcd.clear();
  lcd.print(msg);
}

void setup(void)
{
  Serial.begin(9600);
  Wire.begin();
  lcd.begin(16, 2);
  lcd.print("Initializing...");

  /* volt/amp */
  pinMode(2, INPUT);
  /* range */
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);

  // Reset devices
  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms

  // Check device present
  Wire.requestFrom(address, (uint8_t)1);
  error("Requested");
  if (!Wire.available()) {
    error("I2C ADC init failed");
    while (1)
      ;
  }
  error("Init done");
}

enum range read_range(void)
{
  uint8_t voltage = 0;

  if (digitalRead(2))
    voltage = 1;

  if (!digitalRead(3)) {
    if (voltage)
      return R_ERR;
    else
      return R_10mA;
  }

  if (!digitalRead(4)) {
    if (voltage)
      return R_1000V;
    else
      return R_1000uA;
  }

  if (!digitalRead(5)) {
    if (voltage)
      return R_100V;
    else
      return R_100uA;
  }

  if (!digitalRead(6)) {
    if (voltage)
      return R_10V;
    else
      return R_10uA;
  }

  if (!digitalRead(7)) {
    if (voltage)
      return R_1V;
    else
      return R_1uA;
  }

  if (!digitalRead(8)) {
    if (voltage)
      return R_100mV;
    else
      return R_100nA;
  }

  if (!digitalRead(9)) {
    if (voltage)
      return R_10mV;
    else
      return R_10nA;
  }

  if (!digitalRead(10)) {
    if (voltage)
      return R_1mV;
    else
      return R_1nA;
  }

  if (!digitalRead(11)) {
    if (voltage)
      return R_100uV;
    else
      return R_100pA;
  }

  return R_ERR;
}

/* Mecnanical switches sometimes need to settle down */
static enum range get_range(void)
{
  enum range range;
  int8_t retry = 100;

  for (;;) {
    range = read_range();

    if (range != R_ERR)
       return range;

    if (retry-- <= 0)
      return range;

    delay(10);
  }
}

static void print_value(double val, char *buf, enum range range, uint8_t digit)
{
  /* Over 2V -> overflow */
  if (val > 2.01) {
    strcpy(buf, "Over+range");
    return;
  }

  /* Under 2V -> overflow */
  if (val < -2.01) {
    strcpy(buf, "Over-range");
    return;
  }

  int32_t ival = 1000000 * val;

  if (ival < 0)
    ival -= (digit ? 5 : 500);
  else
    ival += (digit ? 5 : 500);

  if (ival < 0) {
    *(buf++) = '-';
    ival = -ival;
  } else if (val > 0) {
    *(buf++) = '+';
  } else {
    *(buf++) = ' ';
  }

  *(buf++) = '0' + ival/1000000;
  ival %= 1000000;

  if (range_mul[range] == 1)
    *(buf++) = '.';

  *(buf++) = '0' + ival/100000;
  ival %= 100000;

  if (range_mul[range] == 10)
    *(buf++) = '.';

  *(buf++) = '0' + ival/10000;
  ival %= 10000;

  if (range_mul[range] == 100)
    *(buf++) = '.';

  *(buf++) = '0' + ival/1000;
  ival %= 1000;

  if (range_mul[range] == 1000)
    *(buf++) = '.';

  if (digit) {
      *(buf++) = '0' + ival/100;
      ival %= 100;

     *(buf++) = '0' + ival/10;
   //  ival %= 10;
   // *(buf++) = '0' + ival;
  }

  *(buf++) = range_units[range][0];
  *(buf++) = range_units[range][1];

  *buf = 0;
}

static MCP342x::Gain gain = MCP342x::gain1;
static int gain_div = 1;

static double read_adc(void)
{
  MCP342x::Config status;
  long value;

  // Initiate a conversion; convertAndRead() will wait until it can be read
  uint8_t err = adc.convertAndRead(MCP342x::channel1, MCP342x::oneShot,
           MCP342x::resolution18, gain, 1000000, value, status);

  if (err) {
    error("I2C ADC Error");
    for (;;) {};
  }

  long aval = value < 0 ? -value : value;

  if (aval > 0 && aval < 60000) {
    if (gain == MCP342x::gain1) {
      gain = MCP342x::gain2;
      gain_div = 2;
    }
    if (gain == MCP342x::gain2) {
      gain = MCP342x::gain4;
      gain_div = 4;
    }
    if (gain == MCP342x::gain4) {
      gain = MCP342x::gain8;
      gain_div = 8;
    }
  }

  if (aval > 120000) {
    if (gain == MCP342x::gain2) {
      gain = MCP342x::gain1;
      gain_div = 1;
    }
    if (gain == MCP342x::gain4) {
      gain = MCP342x::gain2;
      gain_div = 2;
    }
    if (gain == MCP342x::gain8) {
      gain = MCP342x::gain4;
      gain_div = 4;
    }
  }

  return 2.048 * value / gain_div / 131072;
}

void loop(void)
{
  static double val_lp;
  static int8_t settling;
  static enum range old_range = R_ERR;
  char buf[32];
  enum range range = get_range();
  static double vals[4];


  if (range == R_ERR) {
    error("Invalid range");
    return;
  }

  vals[0] = vals[1];
  vals[1] = vals[2];
  vals[2] = vals[3];
  vals[3] = read_adc();

  double val = (vals[0] + vals[1] + vals[2] + vals[3]) * range_cal[range]/4;

  print_value(val, buf, range, 0);
  Serial.println(buf);

  lcd.setCursor(0,0);

  lcd.print(buf);
  lcd.print(" ");
  lcd.print(range_names[range]);

  if (range != old_range) {
    val_lp = val;
    old_range = range;
  } else {
    if (abs(val_lp - val) < 0.001)
      val_lp = 0.4 * val + 0.6 * val_lp;
    else if (abs(val_lp - val) < 0.01)
      val_lp = 0.4 * val + 0.6 * val_lp;
    else
      val_lp = 0.6 * val + 0.4 * val_lp;
  }

  print_value(val_lp, buf, range, 1);

  lcd.setCursor(0, 1);
  lcd.print(buf);
  lcd.print(" lp");

  if (gain == MCP342x::gain1)
    lcd.print(" G1 ");

  if (gain == MCP342x::gain2)
    lcd.print(" G2 ");

  if (gain == MCP342x::gain4)
    lcd.print(" G4 ");

  if (gain == MCP342x::gain8)
    lcd.print(" G8 ");
}
