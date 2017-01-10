#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include <stdlib.h>
#include <Wire.h> 
#include <string.h>
#include <stdarg.h>

#define LINE1 0
#define LINE2 10
#define LINE3 20

#define CHAR_WIDTH 8

#define UI_STR_WIDTH 6

#define LINE_START 5
#define LINE_END 117
#define LINE_MAXCHAR 15

#define BTN_CHECK_DELAY 200
#define ROOM_RAND_CHANCE 75

//accelerometer
#define THRESHOLD 250

void handleButtonStates();
int selectWasPressed();
int startWasPressed();
void handleInput();
void handleOutput();
void setControlRowOled(char **options, int selected, int width, int str_width);
struct room *createRoom(struct room *roomFrom, int directionFrom);
int winning = 0;
int winningCount = 0;

//wire utility functions @Rollen  D'Souza
void WireInit();
void WireRequestArray(int address, uint8_t* buffer, uint8_t amount);
void WireWriteRegister(int address, uint8_t reg, uint8_t value);
void WireWriteByte(int address, uint8_t value);
//accelerometer functions
void initializeAccelerometer();
int successfulShake();
int gameOver = 0;


static TwoWire orbitComm(0);

//defaults
char defaultUiOptions[6][UI_STR_WIDTH] = { " LIV ", " NOR ", " EAS ", " SOT ", " WES ", " EXA " };

struct room 
{
  char *desc;
  int descLength;
  char **uiOptions;
  struct room *exits[4];
  int nExits;
  int x, y;
  int winRoom;
};

struct menu
{
  char *desc;
  int lives;
};

int inMenu = 0;
int inBattle = 0;
int examining = 0;
struct menu *HPmenu;

struct room *tRoom;
struct room *tRoom2;
struct room *currRoom;

int screenChanged;
int instBtnStateSel;
int btnStateSel;
int btnStatePrevSel;
int instBtnStateStr;
int btnStateStr;
int btnStatePrevStr;
unsigned long int timeCheck;
unsigned long int drawCheck;
int screenSelect;
char *temp;
String temp1;
//String items[8] = {"SWORD", "AXE", "POTION", "KEY", "MAP", "ROBE", "RING", "STONE"};

void setup() {
  orbitComm.begin();

  delay(100);

  pinMode(PD_2, INPUT);
  pinMode(PE_0, INPUT);

  screenChanged = 1;
  screenSelect = 0;
  btnStateSel = LOW;
  instBtnStateSel = LOW;
  btnStatePrevSel = LOW;
  btnStateStr = LOW;
  instBtnStateStr = LOW;
  btnStatePrevStr = LOW;
  timeCheck = millis();
  drawCheck = millis();
  temp = new char[1];
    
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
  OrbitOledSetDrawMode(modOledSet);

  tRoom = createRoom(0, 0);
  generateDungeon(5, tRoom);
  currRoom = tRoom;

  HPmenu = (struct menu *)malloc(sizeof(struct menu)); 
  HPmenu->desc = (char *)malloc(sizeof(char) * 14);
  HPmenu->lives = 3;
 
  //accelerometer
  WireInit();
  initializeAccelerometer();
  randomSeed(analogRead(A1));
}

void loop() 
{
  if(!gameOver)
  {
    handleInput();
    handleOutput();
  }
  handleGameOver();
}


void getInstButtonStates()
{
  instBtnStateSel = !(digitalRead(PD_2));
  instBtnStateStr = !(digitalRead(PE_0));
}

void resetButtonStates()
{
  btnStateSel = instBtnStateSel;
  btnStateStr = instBtnStateStr;
}

int selectWasPressed()
{
  return instBtnStateSel != btnStateSel && instBtnStateSel;
}

int startWasPressed()
{
    return instBtnStateStr != btnStateStr && instBtnStateStr;
}

void handleInput()
{
  getInstButtonStates();
  
  if (selectWasPressed())  
  {
    if(!inMenu && !inBattle){
      OrbitOledClear();
      screenChanged = 1;
      screenSelect++;
    }
  }
  
  if (startWasPressed())  
  {
    if(inBattle){
        inBattle = 0;
        screenSelect = 1;
        screenChanged = 1;
    }
    else if(inMenu){
        inMenu = 0;
        screenSelect = 0;
        screenChanged = 1;
    }
    /*else if(examining){
        examining = 0;
        screenSelect = 0;
        screenChanged = 1;
    }*/
    else{
      int num = random(3);
      switch (screenSelect)
      {
        case 0:
          inMenu = 1;
          screenChanged = 1;
          OrbitOledClear();
          break;
        case 1:
          if(currRoom->exits[0])
          {
            if(num == 2)
              inBattle = 1;
            currRoom = currRoom->exits[0];
            screenChanged = 1;
            if(currRoom->winRoom)
              gameOver = 1;
            OrbitOledClear();
          }
        break;
        case 2:
          if(currRoom->exits[1])
          {
            if(num == 2)
              inBattle = 1;
            currRoom = currRoom->exits[1];
            screenChanged = 1;
            if(currRoom->winRoom)
              gameOver = 1;
            OrbitOledClear();
          }
        break;
        case 3:
          if(currRoom->exits[2])
          {
            if(num == 2)
            {
              inBattle = 1;
            }
            currRoom = currRoom->exits[2];
            screenChanged = 1;
            if(currRoom->winRoom)
              gameOver = 1;
            OrbitOledClear();
          }
        break;
        case 4:
          if(currRoom->exits[3])
          {
            if(num == 2)
            {
              inBattle = 1;
              OrbitOledClear();
            }
            currRoom = currRoom->exits[3];
            screenChanged = 1;
            if(currRoom->winRoom)
              gameOver = 1;
            OrbitOledClear();
          }
          break;
        /*case 5:
          examining = 1;
          screenChanged = 1;
          OrbitOledClear();
          break;*/
      }
    }  
  }
  resetButtonStates();
  if (screenSelect > 5)
    screenSelect = 0;
}

void handleOutput()
{
  if(!inBattle){
    OrbitOledMoveTo(0, LINE1);
    OrbitOledDrawString(currRoom->desc);
  }
  if (screenChanged){
    if(inMenu)
      drawMenuOled();
    else if(inBattle)
      battle();
    //else if(examining)
    //  examine();
    else
      setControlRowOled(currRoom->uiOptions, screenSelect, 3, 5);  
  }

  
  if (millis() - drawCheck > 30)
  {
    if (screenChanged)
    {
      OrbitOledUpdate();
      screenChanged = 0;
    }
    else
      OrbitOledUpdate();
    drawCheck = millis();
  }
}

void handleGameOver(){
  if (gameOver < 3 && gameOver > 0)
  {
    OrbitOledClear();
    switch(gameOver){
      case 1:
        OrbitOledMoveTo( (CHAR_WIDTH * 3), LINE2);
        OrbitOledDrawString("YOU WIN!");
        break;
      case 2:
        OrbitOledMoveTo( (CHAR_WIDTH * 3), LINE2);
        OrbitOledDrawString("YOU LOSE!");
        break;
    }
    OrbitOledUpdate();
    gameOver = 3;
  }
}

void battle(){
    OrbitOledClear();
    OrbitOledMoveTo( (CHAR_WIDTH * 1), LINE1);  
    OrbitOledDrawString("MONSTER FOUND");
    OrbitOledMoveTo( (CHAR_WIDTH * 2), LINE2);
    OrbitOledDrawString("SHAKE!");
    OrbitOledUpdate();
    delay(2000);
    if(successfulShake()){
      OrbitOledClear();
      OrbitOledMoveTo( (CHAR_WIDTH * 4), LINE1);
      OrbitOledDrawString("MONSTER");
      OrbitOledMoveTo( (CHAR_WIDTH * 3), LINE2);
      OrbitOledDrawString("DEFEATED!");
      return;
    }
    else{
      HPmenu->lives --;
      if(HPmenu->lives == 0){
        gameOver = 2;
        return;
      }
      OrbitOledClear();
      OrbitOledMoveTo( (CHAR_WIDTH * 3), LINE1);
      OrbitOledDrawString("YOU LOST");
      OrbitOledMoveTo( (CHAR_WIDTH * 4), LINE2);
      OrbitOledDrawString("A LIFE!");
    }
}

void setControlRowOled(char **options, int selected, int width, int str_width) 
{
  OrbitOledClear();
  for (int i = 0; i < width * 2; i++)
    options[i][0] = ' ';
  
  options[selected][0] = '#';
  
  for (int i = 0; i < 3; i++)
  {
    OrbitOledMoveTo((i * CHAR_WIDTH * 5), LINE2);
    OrbitOledDrawString(options[i]);
  }
  
  OrbitOledMoveTo(LINE_START, LINE3);
  
  for (int i = 3; i < 6; i++)
  {
    OrbitOledMoveTo(((i-3) * CHAR_WIDTH * 5), LINE3);
    OrbitOledDrawString(options[i]);
  }
    
}

void drawMenuOled(){
    char *HPmenuDesc;
    HPmenuDesc = (char *)malloc(sizeof(char) * 14);
    sprintf(HPmenuDesc, "LIVES LEFT: %d", HPmenu->lives);
    HPmenu->desc = HPmenuDesc;
    
    OrbitOledMoveTo( (CHAR_WIDTH * 0), LINE2);
    OrbitOledDrawString(HPmenu->desc);
    OrbitOledMoveTo( (CHAR_WIDTH * 2), LINE3);
    OrbitOledDrawString("PRESS START");
    return;
}

/*void examine(){
    int numItems = random(2);
    for(int i = 0; 
  
}*/

struct room *createRoom(struct room *roomFrom, int roomDirection)
{
  struct room *newRoom = (struct room *)malloc(sizeof(struct room)); 
  newRoom->descLength = 10;
  char *newRoomDesc; 
  char **newUiOptions = (char **) calloc(6, sizeof(char*));

  for(int i = 0; i < 4; i++)
  {
    newRoom->exits[i] = 0;
  }

  if (roomFrom)
  {
    newRoom->nExits = 1;
    roomFrom->nExits += 1;

    if (roomDirection == 0) //new room is north of the previous, so the south exit of the this room will lead to the room it was generated from
    {
      newRoom->x = roomFrom->x;
      newRoom->y = roomFrom->y + 1;
      (roomFrom->exits)[0] = newRoom;
      (newRoom->exits)[2] = roomFrom;
    }
    if (roomDirection == 1) //east
    {
      newRoom->x = roomFrom->x + 1;
      newRoom->y = roomFrom->y;
      (roomFrom->exits)[1] = newRoom;
      (newRoom->exits)[3] = roomFrom;
    }
    if (roomDirection == 2) //south
    {
      newRoom->x = roomFrom->x;
      newRoom->y = roomFrom->y - 1;
      (roomFrom->exits)[2] = newRoom;
      (newRoom->exits)[0] = roomFrom;
    }
    if (roomDirection == 3) //west
    {
      newRoom->x = roomFrom->x - 1;
      newRoom->y = roomFrom->y;
      (roomFrom->exits)[3] = newRoom;
      (newRoom->exits)[1] = roomFrom;
    }
  }
  
  else //this room must be at the center
  {
    (newRoom->exits)[0] = 0;
    newRoom->nExits = 0;
    newRoom->x = 0;
    newRoom->y = 0;
  }

  if (newRoom->x < 0)
    newRoom->descLength += 1;
  if(newRoom->y < 0)
    newRoom->descLength += 1;

  newRoomDesc = (char *)malloc(sizeof(char) * (newRoom->descLength + 1));

  for (int i = 0; i < 6; i++)
  {
    newUiOptions[i] = (char *) calloc(UI_STR_WIDTH + 1, sizeof(char));
  }
  
  strcpy(newUiOptions[0], (const char *) defaultUiOptions[0]);
  strcpy(newUiOptions[1], (const char *) defaultUiOptions[1]);
  strcpy(newUiOptions[2], (const char *) defaultUiOptions[2]);
  strcpy(newUiOptions[3], (const char *) defaultUiOptions[3]);
  strcpy(newUiOptions[4], (const char *) defaultUiOptions[4]);
  strcpy(newUiOptions[5], (const char *) defaultUiOptions[5]);

  sprintf(newRoomDesc, "ROOM (%d,%d)", newRoom->x, newRoom->y);

  newRoom->uiOptions = newUiOptions;
  newRoom->desc = newRoomDesc;
  newRoom->winRoom = 0;
  return newRoom;
}

void generateDungeon(int depth, struct room *bRoom)
{
  int **grid = (int **)malloc((depth * 2 + 1) * sizeof(*grid));

  for (int i=0; i < depth * 2 + 1; i++)
  {
    grid[i] = (int *)malloc((depth * 2 + 1) * sizeof *grid[i]);
  }
  
  for (int i = 0; i < depth*2 + 1; i++)
  {
    for(int j = 0; j < depth*2 + 1; j++)
    {
      grid[i][j] = 0;
    } 
  }

  grid[depth][depth] = 1;
  
  generateDungeonHelper(grid, depth, depth, depth, depth, bRoom);
}

void generateDungeonHelper(int **grid, int n, int x, int y, int depth, struct room *cRoom)
{
  if(n == 0)
  {
    if(!winning)
    {
      randomSeed(analogRead(A1));
      if (random(5) == 1 || winningCount == 5)
      {
        cRoom->winRoom = 1;
        winning = 1;
      }
      else
        winningCount++;
    }
    return;
  }

  
  
  int randx = random(100);
  if (randx < ROOM_RAND_CHANCE)
  {
    if (x < (depth * 2 + 1) && y+1 < (depth * 2 + 1))
    {
      if (!grid[x][y + 1])
      {
        grid[x][y + 1]++;
        generateDungeonHelper(grid, n-1, x, y+1, depth, createRoom(cRoom, 0));
      }
    }
  } 

  randx = random(100);
  if (randx < ROOM_RAND_CHANCE)
  {
    if (x+1 < (depth * 2 + 1) && y < (depth * 2 + 1))
    {
      if (!grid[x + 1][y])
      {
        grid[x + 1][y]++;
          generateDungeonHelper(grid, n-1, x+1, y, depth, createRoom(cRoom, 1));
      }
    }
  }
  randx = random(100);
  if (randx < ROOM_RAND_CHANCE)
  {
    if (x < (depth * 2 + 1) && y-1 < (depth * 2 + 1))
    {
      if (!grid[x][y-1])
      {
          grid[x][y - 1]++;
          generateDungeonHelper(grid, n-1, x, y-1, depth, createRoom(cRoom, 2));
      }
    }
  }

  randx = random(100);
  if (randx < ROOM_RAND_CHANCE)
  {
    if (x-1 < (depth * 2 + 1) && y < (depth * 2 + 1))
    {
      if (!grid[x - 1][y])
      {
          grid[x - 1][y]++;
          generateDungeonHelper(grid, n-1, x-1, y, depth, createRoom(cRoom, 3));
      }
    }
  }
}

void setDefaultWinRoom(struct room *cRoom, int direction)
{
  if (!(cRoom->exits[direction]))
  {
    cRoom->winRoom = 1;
    Serial.println(cRoom->x);
      Serial.println(cRoom->y);
    return;
  }
  setDefaultWinRoom(cRoom->exits[direction], direction);
}

//*****ACCELEROMETER FUNCTIONS*****

void initializeAccelerometer(){
    WireWriteRegister(0x1D, 0x2D, 0x08);  //measurement mode
    WireWriteRegister(0x1D, 0x31, 0xB);  //+-16g data format

    WireWriteRegister(0x1D, 0x1E, 0x00);  //X offset
    WireWriteRegister(0x1D, 0x1F, 0x00);  //Y offset
    WireWriteRegister(0x1D, 0x20, 0x00);  //Z offset
}

int successfulShake(){
    uint8_t data[6] = {0};                //accelerometer gives 6 bytes of data
    WireWriteByte(0x1D, 0x32);
    //WireRequestArray(0x1D, data, 6);    //get data from accelerometer
    orbitComm.requestFrom(0x1D, 6);
    int i = 0;
    int amount = 6;
    do 
    {
      while(!orbitComm.available());
      data[i] = orbitComm.read();
      i++;
    } while(--amount > 0);

    int16_t xaxis = (data[1] << 8) | data[0];  //put data from two bytes of 
    int16_t yaxis = (data[3] << 8) | data[2];  //each axis into one 16bit int
    int16_t zaxis = (data[5] << 8) | data[4];

    float xg = xaxis + 1;
    float yg = yaxis + 16;
    float zg = zaxis - 232;

    float vector = sqrt(xg*xg + yg*yg + zg*zg);

    if(vector > THRESHOLD) return 1;
    else return 0;
    
}

//wire utility functions
void WireInit(){
  orbitComm.begin();
}
void WireWriteByte(int address, uint8_t value){
  orbitComm.beginTransmission(address);
  orbitComm.write(value);
  orbitComm.endTransmission();
}
void WireWriteRegister(int address, uint8_t reg, uint8_t value){
  orbitComm.beginTransmission(address);
  orbitComm.write(reg);
  orbitComm.write(value);
  orbitComm.endTransmission();
}
void WireRequestArray(int address, uint32_t* buffer, uint8_t amount){
  orbitComm.requestFrom(address, amount);
  do 
  {
    while(!orbitComm.available());
    *(buffer++) = orbitComm.read();
  } while(--amount > 0);
}


