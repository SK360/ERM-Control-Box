# ERM-Control-Box
Code for Arduino to Start/Stop Escape Room Master

See attached diagram image for wiring example. The LED's would need to run through a resistor if connecting directly.  I am running my wires to trigger a relay board since I am using large 12v LED's 

Instead of a win button I am using a sensor on the final prop to send ground to the win pin to auto complete the room when the group finds the correct item.  Could easily be tied to a keypad or exit button as well.  This is why I had to add the Bounce library to debounce the input due to some false positives we were getting.

See my ERM settings screenshot for how I have this setup in ERM Automation

Make sure you add Bounce2 Library to the Arduino Sketch
https://github.com/thomasfredericks/Bounce2

When the board is powered on it is "ready", the green light will illuminate. ERM will poll /started for "Not Started" to change to "Started" and start the room timer.  Upon room start in ERM it will call /confirmstart which will illuiminate the Red LED to confirm to gamemaster that room has indeed started.

When time expires in ERM it will call /gamefailed which will blink the RED led confirming to gamemaster that the room has run out of time and failed.

When Win button is pressed, "Not Won" on /won will change to "Won" and ERM will complete the room.  Yellow LED Illuminates.  ERM is also setup to call /roomwon on room completion so that pressing complete room from the web page will also illuminate yellow LED.

When game reset is called from ERM, it polls /reset which resets all variables on the prop and should illuminate the Green LED again.
