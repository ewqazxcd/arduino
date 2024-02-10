//snakeDir="same direction" as the direction before so if the jostick goes to normal the last direction is shown
#include <LedControl.h>

#define UP           0
#define RIGHT        1
#define DOWN         2
#define LEFT         3
#define NO_DIRECTION 5
#define DIN         A0
#define CS          A1
#define CLK         A2
#define YAXIS       A3
#define XAXIS       A4
#define JOYPRESS    A5
#define DELAY       75

#define MAX_QUEUE_SIZE 64

struct Coordinate {char x; char y;};

struct Queue3 {Coordinate a; Coordinate b; Coordinate c;};

struct Queue3Arr {
  Coordinate buffer[3];
};

struct FlexQueue {
  int size;
  Coordinate* buffer;
};



////////// BEGIN Queue3
Queue3 newQueue3(Coordinate a, Coordinate b, Coordinate c) {
  return Queue3 {a, b, c};
}

void pushQueue(Queue3* q, Coordinate coord) {
  q->c = q->b;
  q->b = q->a;
  q->a = coord;
}

Coordinate firstInQueue(Queue3* q) {
  return q->c;
}

Coordinate lastInQueue(Queue3* q) {
  return q->a;
}
////////// END Queue3



////////// BEGIN Queue3Arr
Queue3Arr newQueue3Arr(Coordinate a, Coordinate b, Coordinate c) {
  return Queue3Arr {{a, b, c}};
}

void pushQueueReference(Queue3Arr* q, Coordinate coord) {
  q->buffer[2] = q->buffer[1]; 
  q->buffer[1] = q->buffer[0]; 
  q->buffer[0] = coord;
}

void pushQueue(Queue3Arr* q, Coordinate coord) {
  for(int i = 2; i > 0; i--) {
    q->buffer[i] = q->buffer[i-1];
  }
  q->buffer[0] = coord;
}

Coordinate firstInQueue(Queue3Arr* q) {
  return q->buffer[2]; 
}

Coordinate lastInQueue(Queue3Arr* q) {
  return q->buffer[0]; 
}
////////// END Queue3Arr



////////// BEGIN FlexQueue
FlexQueue newFlexQueue(Coordinate* buffer) {
  return FlexQueue {0, buffer};
}

void pushQueue(FlexQueue* q, Coordinate coord) {
  for(int i = q->size; i > 0; i--) {
    q->buffer[i] = q->buffer[i-1];
  }
  q->buffer[0] = coord;
  q->size++;
}

void popQueue(FlexQueue* q) {
  q->size--;
}

Coordinate firstInQueue(FlexQueue* q) {
  q->buffer[0];
}

Coordinate lastInQueue(FlexQueue* q) {
  q->buffer[(q->size) - 1];
}

void printQueue(FlexQueue* q) {
  Serial.println("Current queue");
  for(int i = q->size - 1; i >= 0; i--) {
    Serial.print(q->buffer[i].x, DEC);
    Serial.print(", ");
    Serial.println(q->buffer[i].y, DEC);
  }
}

////////// END FlexQueue



int xValue;
int yValue;
int buttonValue;
int rotation;
int prevButtonValue;
int joyDirection;
int rawJoyDirection;
int addLen;
int snakeDir;
bool headyPlus;
bool headxPlus;
bool headyMinus;
bool headxMinus;
LedControl lc = LedControl(DIN, CLK, CS, 0);

// Queue3 snake;
// Queue3Arr snake;

Coordinate snakeBuffer[MAX_QUEUE_SIZE];
FlexQueue snake;

void setup() {
  pinMode(XAXIS, INPUT);
  pinMode(YAXIS, INPUT);
  pinMode(JOYPRESS, INPUT);
  digitalWrite(JOYPRESS, HIGH);
  digitalWrite(13, HIGH);
  lc.shutdown(0, false);
  lc.setIntensity(0, 0);
  lc.clearDisplay(0);
  Serial.begin(9600);

  // snake = newQueue3({0, 0}, {0, 0}, {0, 0});
  // snake = newQueue3Arr({0, 0}, {0, 0}, {0, 0});
  snake = newFlexQueue(snakeBuffer);
  pushQueue(&snake, Coordinate {1, 2});
  pushQueue(&snake, Coordinate {3, 4});
  pushQueue(&snake, Coordinate {5, 6});

  printQueue(&snake);

  Serial.println("------------ setup done");
  Serial.println((int)snakeBuffer, HEX);
};

int convertJoyToRawDirection(int x, int y) {
  if      (y < 15  ) {return UP   ;}
  else if (x > 1015) {return RIGHT;} 
  else if (y > 1015) {return DOWN ;}
  else if (x < 15  ) {return LEFT ;}
  else               {return NO_DIRECTION;}
}

int convertDirection(char rawJoyDirection, char rot) {
  if (rawJoyDirection == NO_DIRECTION) { return NO_DIRECTION; }
  return (rawJoyDirection + rot) % 4;
}

void ledOff(Coordinate coord) {lc.setLed(0, coord.y, coord.x, false);}
void ledOn (Coordinate coord) {lc.setLed(0, coord.y, coord.x, true );}

// TODO Get rid of magic numbers
bool atTopBorder  (Coordinate coord) {return coord.y == 0;}
bool atRightBorder(Coordinate coord) {return coord.x == 7;}
bool atDownBorder (Coordinate coord) {return coord.y == 7;}
bool atLeftBorder (Coordinate coord) {return coord.x == 0;}

Coordinate head;
Coordinate tail;

void moveSnake() {
  head = lastInQueue (&snake);
  tail = firstInQueue(&snake);

  Serial.print("head ");
  Serial.print(head.x,DEC);
  Serial.print(", ");
  Serial.println(head.y,DEC);
  Serial.print("tail ");
  Serial.print(tail.x,DEC);
  Serial.print(", ");
  Serial.println(tail.y,DEC);

  if      (snakeDir==UP   ) {if (atTopBorder  (head)) {head.y++; snakeDir++      ;} head.y--;}
  else if (snakeDir==RIGHT) {if (atRightBorder(head)) {head.x--; snakeDir++      ;} head.x++;}
  else if (snakeDir==DOWN ) {if (atDownBorder (head)) {head.y--; snakeDir = LEFT ;} head.y++;}
  else if (snakeDir==LEFT ) {if (atLeftBorder (head)) {head.x++; snakeDir = UP   ;} head.x--;}

  pushQueue(&snake, head);
  popQueue(&snake);

  ledOff(tail);
  ledOn(head);
}

void loop() {
  Serial.println("*************");

  printQueue(&snake);

  xValue    = analogRead  (XAXIS   );
  yValue    = analogRead  (YAXIS   );
  buttonValue    = digitalRead (JOYPRESS);

  rawJoyDirection = convertJoyToRawDirection(xValue   , yValue    );
  joyDirection    = convertDirection   (rawJoyDirection, rotation);

  if (buttonValue == 0 && prevButtonValue == 1) { rotation++  ;}
  if (buttonValue == 1)               {prevButtonValue     = 1;}
  if (buttonValue != 1)               {prevButtonValue     = 0;}
  if (rotation > 3)                   {rotation            = 0;}
  if (joyDirection != NO_DIRECTION)   {snakeDir = joyDirection;}

  moveSnake();
  delay(DELAY*2*20);
}
