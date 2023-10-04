#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <GyverEncoder.h>


const int SCREEN_WIDTH=128; // OLED display width, in pixels
const int SCREEN_HEIGHT=64; // OLED display height, in pixels
const int first_string=12;  //first string on LCD
const int second_string=28;  //second string on LCD
const int third_string=44;  //third string on LCD
const int fourth_string=62;  //fourth string on LCD
const int enc_a_pin=2; //encoder A pin
const int enc_b_pin=3; //encoder B pin
const int enc_btn_pin=7; //encoder Button pin
const int i_meas_pin=14;
const int u_meas_pin=15;
const int u_gate_pin=5;
const int u_meas_thr=160;//reduce I to 1,2А if U>20V
const float i_set_thr=1.2;//reduce I to 1,2А if U>20V
const float hyst=0.02; //hysteresis for setting I
const float u_corr_coef=7.1;
const float inc_coef=1000;

//i_meas=41,I=0.5A; i_meas=81,I=1A; i_meas=109,I=1.2A; i_meas=130,I=1.5A; i_meas=180,I=2A; i_meas=225,I=2.5A;
//u_meas=160,U=20V; u_meas=197,U=25V; u_meas=92,U=12V; u_meas=373,U=48V; 
const int u_gate_min=160; //3v Vgate //for SPW47N60C3
const int u_meas_min=69; //69=9V

float i_set=0;
float u_gate=u_gate_min;
int print_iter=0;

int i_meas=0;
int u_meas=0;

float inc=0;//current increment Ugate
int load_on=1; //1-on -1-off
int charging=0;


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Encoder enc1(enc_b_pin, enc_a_pin, enc_btn_pin, TYPE2);  // для работы c кнопкой и сразу выбираем тип


// //debug
//void show(){
//  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
//  display.setCursor(0,first_string);             
//  display.print("I set: ");
//  display.print(i_set);
//  display.setCursor(0,second_string);   
//  display.print("I meas: ");
//  display.print(i_meas);  
//  display.setCursor(0,third_string);   
//  display.print("U meas: ");
//  display.print(u_meas);    
//  display.display();    
//}


void show(){
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  display.setCursor(0,first_string);             
  display.print("I set: ");  
  display.print(i_set);   
  display.print(" A");
  display.setCursor(0,second_string);   
  display.print("I meas: ");    
  display.print(i_dac2si(i_meas));    
  display.print(" A");
  display.setCursor(0,third_string);   
  display.print("U meas: ");
  display.print(int(float(u_meas)/u_corr_coef));
  //display.print(float(u_meas));//debug
  display.print(" V");    
  display.display();    
}



void setup() {
  // put your setup code here, to run once:
i_meas=analogRead(i_meas_pin);//init
u_meas=analogRead(u_meas_pin);//init


analogReference(INTERNAL);


if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
  Serial.println("SSD1306 allocation failed");
  for(;;);
}
  display.setFont(&FreeSerif9pt7b);
  display.clearDisplay();
  display.setTextSize(1);             
  display.setTextColor(WHITE);        
  display.setCursor(0,first_string);             
  display.println("Loading...");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  show();
}

void msg(String s){
    display.fillRect(0, third_string+1,SCREEN_WIDTH , fourth_string-third_string+1, BLACK);
    display.setCursor(0,fourth_string);             
    display.print(s); 
    display.display();  
}

float i_dac2si(float i){
  if(i<100){
    return(float(i)/86); //setup
  } else return(float(i)/90);  //setup
}

void loop() {
  // put your main code here, to run repeatedly:
  //delay(1);
  i_meas=analogRead(i_meas_pin);
  u_meas=analogRead(u_meas_pin);

  // обязательная функция отработки. Должна постоянно опрашиваться
  enc1.tick();
  
  if (enc1.isTurn()) {     // если был совершён поворот (индикатор поворота в любую сторону)
    // ваш код
  }
  
  if (enc1.isRight()) {  // если был поворот
    //Serial.println("Right");
    if(i_set<2.5)i_set+=0.01;
    show();
  }
  if (enc1.isLeft()) {
    //Serial.println("Left");
    if(i_set>0)i_set-=0.01;
    show();    
  }
  
  if (enc1.isRightH()){  // если было удержание + поворот
    if(i_set<=2.4)i_set+=0.1;
    show();
  }
  if (enc1.isLeftH()){
    if(i_set>=0.1){
      i_set-=0.1;
    } else i_set=0;
    show();      
  }
//  
//  if (enc1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
//  if (enc1.isClick()) Serial.println("Click");         // отпускание кнопки (+ дебаунс)
//  //if (enc1.isRelease()) Serial.println("Release");     // то же самое, что isClick
//  
  if (enc1.isHolded()){  // если была удержана и энк не поворачивался
    load_on=load_on*-1;      
  }
//  //if (enc1.isHold()) Serial.println("Hold");         // возвращает состояние кнопки



  if(u_meas<u_meas_min)u_gate=u_gate_min;//load off in no voltage!
  if(u_meas>=u_meas_thr and i_set>i_set_thr){
    i_set=i_set_thr;//reduce I to 1,2А if U>20V    
    msg("OVERLOAD");
  }
  

  print_iter++;
  if(print_iter>1000){
    show();
    if(charging==1)msg("CHARGING...");

    if(load_on==-1){    
      msg("LOAD OFF");
    }  
    print_iter=0;    
  }


  inc=float(abs(i_set-i_dac2si(i_meas)))/inc_coef;

  charging=0;
  if((i_dac2si(i_meas)<(i_set-hyst)) and (load_on==1)){
    u_gate+=inc;
    charging=1;
    if(u_gate>255)u_gate=255;
  }
  if((i_dac2si(i_meas)>(i_set+hyst)) and (load_on==1)){
    u_gate-=inc;    
    charging=1;
    if(u_gate<u_gate_min)u_gate=u_gate_min;
  }

  if(load_on==-1){
    u_gate=u_gate_min;
  }

  analogWrite(u_gate_pin, u_gate); 
  

  

}
