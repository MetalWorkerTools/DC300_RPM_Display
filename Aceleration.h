/*
=============================================================
=       Project: S curve
=      Language: Arduiino r12
=          Date: January 2008
=        Author: C. Eckert
=============================================================
*/
#include <Button.h>

// Givens
long ta = 3e6;     // acceleration time (microsec)
long td = 3e6;     // decelleration time (microsec)
long Vm = 3200;    // steady state velocity (pulse/sec)
long Pt = 12800;    // total number of pulses for move (1600 steps per rev)

// Other variables
long dly;           // stepper pulse delay (microsec)
long t = td/9;      // current time (microsec)  -  You need to seed the initial time with something > 0
                   //                             so you don't calculate to long of a delay
long t12;           // time during constant velocity (microsec)

int count = 0;      // count the number of pulses
int Perr = 0;       // error in position
//Button button = Button(12,PULLUP);
boolean state = false;
int buttonPin = 2;

// Arduino pins
#define dirPin 3
#define stepPin 8

void setup() {
 Serial.begin(9600);
 pinMode(dirPin, OUTPUT);
 pinMode(stepPin, OUTPUT);
 pinMode(buttonPin, INPUT);
 
 // Calculate the time at constant velocity
 t12 = (Pt/(Vm/1e6))-0.5*(ta+td);
 Serial.println(); Serial.println();

 Serial.println("Setup Done");
}

void loop()
{
  //check button pressed, if so enter program condition (inside if statement)
  //if(button.uniquePress()){ //functions based off of button pulling input pin LO
    if (digitalRead(buttonPin) == HIGH){
      state = true;
     
   
    if (state == true);
    {
      //put code you want to run on button press here.
     //if will run once and then when it's done it will exit the if statement and return to the top of the loop
     //function and continually checking to see if the button has been pressed, if it hasn't it will keep
     //checking, if it has it will run the program again and then once done continue to check after
 digitalWrite(dirPin, LOW);  // Set the stepper direction

 // Decide which part of the velocity curve your at
 if (t<ta) {                                       // Acceleration
   //Serial.println ("Acceleration Curve");
   dly = (ta)/(2*(Vm/1e6)*t);
 }
 else if (t>=ta && t<(ta+t12)){                    // Constant velocity
   //Serial.println ("Constant Velocity");
   dly = 1/(2*(Vm/1e6));
 }
 else if (t>=(ta+t12) && t<(ta+t12+td)){          // Deceleration
   //Serial.println ("Deceleration Curve");
   dly = 1/(2*((Vm/1e6)-(Vm/(1e6*td))*(t-ta-t12)));
 }
 
 t = t+2*dly; // update the current time
 //Serial.print("dly: "); Serial.print (dly); Serial.println(" microsec");
 //Serial.print ("Current time: "); Serial.print(t); Serial.println(" microsec");
 
 // Move stepper one pulse using delay just calculated
 digitalWrite(stepPin, HIGH);
 delayMicroseconds(dly);
 digitalWrite(stepPin, LOW);
 delayMicroseconds(dly);
 count ++;
   
 // The move is finished
 if (t>(ta+t12+td)){
   Serial.println ("Move Complete");
   Serial.print ("Total steps indexed: "); Serial.println (count);
   
   // Correct for any position error due to rounding
   Perr = Pt-count;
   if (Perr < 0) {
     digitalWrite(dirPin, 1^digitalRead(dirPin));  // reverse the stepper direction
     delay(50);
     Perr = -1*Perr;
   }
   for (;Perr>0;){
     digitalWrite(stepPin, HIGH);
     delayMicroseconds(dly);
     digitalWrite(stepPin, LOW);
     delayMicroseconds(dly);
     Perr--;
   }
   
   count=0;
   t=td/9;
   
   Serial.print ("Flat Line");
   delay (1000);
   state = false;
   
 }
    }
  }
  }



