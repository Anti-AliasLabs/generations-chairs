void setupGraphs() {

  status = 128; // 128 is an unused value from the Arduino input
  lastStatus = 128;  

  filteredGraph = new int[numGraphPoints];
  baselineGraph = new int[numGraphPoints];
  touchGraph =    new int[numGraphPoints];
  releaseGraph =  new int[numGraphPoints];
  statusGraph =  new int[numGraphPoints];
}

int updateArray(String s) {
  int array;
  print("----");
  println(splitString[1]);
  try {
    array = Integer.parseInt(s.trim());
    //println(array);
  } 
  catch (NumberFormatException e) {
    array = -128;
    println("format exception");
  }
  return array;
}

void updateGraphs() {

  int lastGraphPtr = globalGraphPtr-1;
  if (lastGraphPtr < 0) lastGraphPtr = numGraphPoints-1;

  filteredGraph[globalGraphPtr] = filteredData;
  baselineGraph[globalGraphPtr] = baselineVals; 
  touchGraph[globalGraphPtr] = baselineVals - touchThresholds;
  releaseGraph[globalGraphPtr] = baselineVals - releaseThresholds; 
  if (lastStatus==0 && status!=0x00) {
    // touched
    statusGraph[globalGraphPtr] = 1;
  } else if (lastStatus!=0x00 && status==0x00) {
    // released
    statusGraph[globalGraphPtr] = -1;
  } else {
    statusGraph[globalGraphPtr] = 0;
  }

  lastStatus = status;

  if (++globalGraphPtr >= numGraphPoints) globalGraphPtr = 0;
}

void drawGraphs(int[] graph, int electrode, int graphColour) {
  int scratchColor =g.strokeColor;
  int scratchFill = g.fillColor;
  float scratchWeight = g.strokeWeight;
  stroke(graphColour);
  strokeWeight(2);
  fill(0, 0, 0, 0);

  int localGraphPtr = globalGraphPtr;
  int numPointsDrawn = 0;

  int thisX = -1;
  int thisY = -1;

  beginShape();

  while (numPointsDrawn < numGraphPoints) {
    thisX = (int)(graphsLeft+(numPointsDrawn*graphsWidth/numGraphPoints));
    thisY = (int)graphsTop+ (int)(graphsHeight*(1-((float)graph[localGraphPtr] / (float)tenBits)));

    vertex(thisX, thisY);

    if (++localGraphPtr>=numGraphPoints) localGraphPtr = 0;
    numPointsDrawn++;
  }

  endShape();

  stroke(scratchColor);
  strokeWeight(scratchWeight);
  fill(scratchFill);
}

void drawLevels(int[] arrayToDraw) {
  for (int i=0; i<arrayToDraw.length; i++) {
    rect(40+75*i, 295-arrayToDraw[i], 50, 10);
  }
}

void drawStatus(int electrode) {
  int scratchColor =g.strokeColor;
  float scratchWeight = g.strokeWeight;
  strokeWeight(2);

  int thisX;

  int localGraphPtr = globalGraphPtr;
  int numPointsDrawn = 0;

  while (numPointsDrawn < numGraphPoints) {
    thisX = (int)(graphsLeft+(numPointsDrawn*graphsWidth/numGraphPoints));

    if (statusGraph[localGraphPtr] == 1) {
      //println("beep");
      stroke(touchColour);
      line(thisX, graphsTop, thisX, graphsTop+graphsHeight);
    } else if (statusGraph[localGraphPtr] == -1) {
      //println("beep");
      stroke(releaseColour);
      line(thisX, graphsTop, thisX, graphsTop+graphsHeight);
    }

    if (++localGraphPtr>=numGraphPoints) localGraphPtr = 0;
    numPointsDrawn++;
  }

  stroke(scratchColor);
  strokeWeight(scratchWeight);
}

void drawGrid() {
  int scratchColor =g.strokeColor;
  float scratchWeight = g.strokeWeight;

  stroke(textColour);
  strokeWeight(1);

  for (int i=0; i<=numVerticalDivisions; i++) {
    line(graphsLeft, graphsTop+i*(graphsHeight/numVerticalDivisions), graphsLeft+graphsWidth, graphsTop+i*(graphsHeight/numVerticalDivisions));
  }

  stroke(scratchColor);
  strokeWeight(scratchWeight);
}

void drawText(int[] arrayToDraw) {
  fill(0);
  for (int i=0; i<arrayToDraw.length; i++) {
    text(arrayToDraw[i], 20, 50+20*i);
  }
}

