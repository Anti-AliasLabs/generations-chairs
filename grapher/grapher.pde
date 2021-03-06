/*******************************************************************************
 Grapher
 by Becky Stewart, 2015
 
 Displays debugging information from chair sensor
 ---------------------------------------------------------------------
 Adapted from:
 Bare Conductive MPR121 output grapher / debug plotter for Touch Board
 Bare Conductive code written by Stefan Dzisiewski-Smith.
 ---------------------------------------------------------------------
 
 requires controlp5 to be in your processing libraries folder: 
 http://www.sojamo.de/libraries/controlP5/
 
 requires datastream on the Touch Board: 
 https://github.com/BareConductive/mpr121/tree/public/Examples/DataStream

 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*******************************************************************************/

import processing.serial.*; 
import controlP5.*;

final int baudRate = 9600;
 
final int numElectrodes = 1; // includes proximity electrode 
final int numGraphPoints = 300;
final int tenBits = 1024;

final int graphsLeft = 20;
final int graphsTop = 20;
final int graphsWidth = 984;
final int graphsHeight = 540;
final int numVerticalDivisions = 8;

final int filteredColour = color(255,0,0,200);
final int baselineColour = color(0,0,255,200);
final int touchedColour = color(255,128,0,200);
final int releasedColour = color(0,128,128,200);
final int textColour = color(60);
final int touchColour = color(255,0,255,200);
final int releaseColour = color(255, 255, 255, 200);

final int graphFooterLeft = 20;
final int graphFooterTop = graphsTop + graphsHeight + 20;

final int numFooterLabels = 6;

boolean serialSelected = false;
boolean firstRead = true;
boolean paused = false;

ControlP5 cp5;
DropdownList electrodeSelector, serialSelector;
Textlabel labels[], serialPrompt, pauseInstructions;
 
Serial inPort;        // The serial port
String[] serialList;
String inString;      // Input string from serial port
String[] splitString; // Input string array after splitting 
int lf = 10;          // ASCII linefeed 
int filteredData, baselineVals, diffs, touchThresholds, releaseThresholds, status, lastStatus; 
int lastFilteredData = 600;
int lastBaselineVals = 600; 
int[] filteredGraph, baselineGraph, touchGraph, releaseGraph, statusGraph;
int globalGraphPtr = 0;

int electrodeNumber = 0;
int serialNumber = 4;

void setup(){
  size(1024, 600);
  
  setupGraphs();
  
  serialList = Serial.list();
  println(serialList); 
  
  setupSerialPrompt();
}

void draw(){ 
  background(200); 
  stroke(255);
  if(serialSelected){
    drawGrid();
    drawGraphs(filteredGraph,electrodeNumber, filteredColour);
    drawGraphs(baselineGraph,electrodeNumber, baselineColour);
    //drawGraphs(touchGraph,electrodeNumber, touchedColour);
    //drawGraphs(releaseGraph,electrodeNumber, releasedColour);
    //drawStatus(electrodeNumber);
  }
}


void serialEvent(Serial p) {
 
  if(serialSelected && !paused){ 
  
    int[] dataToUpdate;
    
    inString = p.readString(); 
    splitString = splitTokens(inString, ": ");
    
    if(firstRead && splitString[0].equals("DIFF")){
      firstRead = false;
    } else {
      if(splitString[0].equals("TOUCH")){
        status = updateArray( splitString[1]); 
      } else if(splitString[0].equals("TTHS")){
        touchThresholds = updateArray( splitString[1]); 
      } else if(splitString[0].equals("RTHS")){
        releaseThresholds = updateArray( splitString[1]);
      } else if(splitString[0].equals("FDAT")){
        filteredData = updateArray(splitString[1]); 
        // don't update if formatting error or wildly off from previous
        if(filteredData == -128 || abs(filteredData-lastFilteredData)>400)
          filteredData = lastFilteredData;
        lastFilteredData = filteredData;
        print("FDAT: ");
        println(filteredData);
      } else if(splitString[0].equals("BVAL")){
        baselineVals = updateArray(splitString[1]);
        // don't update if formatting error or wildly off from previous
        if(baselineVals == -128 || abs(baselineVals-lastBaselineVals)>400)
          baselineVals = lastBaselineVals;
        lastBaselineVals = baselineVals;
        print("BVAL: ");
        println(baselineVals);
        updateGraphs(); // update graphs when we get a BVAL line
                        // as this is the last of our dataset
      } else if(splitString[0].equals("DIFF")){
        diffs = updateArray( splitString[1]);
        updateGraphs(); // update graphs when we get a DIFF line
                        // as this is the last of our dataset
      } else if(splitString[0].equals("STATUS")){
        print(splitString[1]);
      }
    }
  }
} 

void controlEvent(ControlEvent theEvent) {
  // DropdownList is of type ControlGroup.
  // A controlEvent will be triggered from inside the ControlGroup class.
  // therefore you need to check the originator of the Event with
  // if (theEvent.isGroup())
  // to avoid an error message thrown by controlP5.

  if (theEvent.isGroup()) {
    // check if the Event was triggered from a ControlGroup
    //println("event from group : "+theEvent.getGroup().getValue()+" from "+theEvent.getGroup());
    if(theEvent.getGroup().getName().contains("electrodeSel")){
      electrodeNumber = (int)theEvent.getGroup().getValue();
    } else if(theEvent.getGroup().getName().contains("serialSel")) {
      serialNumber = (int)theEvent.getGroup().getValue();
      inPort = new Serial(this, Serial.list()[serialNumber], baudRate);
      inPort.bufferUntil(lf);
      
      disableSerialPrompt();
      setupRunGUI();
      setupLabels();
      serialSelected = true;
    }
  } 
  else if (theEvent.isController()) {
  }
}

void keyPressed() {
  if (key == 'p' || key == 'P') {
    paused = !paused;
  }
}
