/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>
#include <stdio.h>
//#include "RefreshLeds.h"
/**********MACROS************/
#define BOARDSIZE 4
#define USBUART_BUFFER_SIZE (2u)
#define USBFS_DEVICE    (0u)
#define MOVERIGHT 0x1D
#define MOVELEFT 0x1C
#define MOVEUP 0x1E
#define MOVEDOWN 0x1F
#define ENTER 0x0D
#define HOME 0x01
#define S_KEY 0x73

#define DIRECTION_UP 4
#define DIRECTION_DOWN 3
#define DIRECTION_LEFT 5
#define DIRECTION_RIGHT 6
#define DIRECTION_LRC 7 //upper right corner
#define DIRECTION_LLC 8 //lower left corner
#define DIRECTION_URC 9
#define DIRECTION_ULC 10
#define ADV 0x5D

#define ERROR 50
#define START_BIT0 0x55
#define START_BIT1 0xaa
#define PLAYERID 51
#define PRESEQ 52
#define SEQ 53

#define MASK 0b00001111
#define TRUE 1
#define FALSE 0


/*********structs***********/
typedef struct{
  int row;
  int col;
  uint8 preRow;
  uint8 preCol;
}Position_t;
Position_t currPos;

enum states {
   START_BIT00, //0
   START_BIT11, //1
   PLAYERIDS,  //2
   SEQ_STATE,  //3
   PASS,  //4
   ROW_POS, //5
   COL_POS,  //6
};
  enum states state = START_BIT00;
/**********prototypes**********/

CY_ISR_PROTO(RX_ISR);
CY_ISR_PROTO(TX_ISR);
CY_ISR_PROTO(LEDS);
void RefreshLeds(void);
void MoveLeds(void);

int IsEmptySlot(void);
int FindBracket(int direction,uint8 currPlayer[BOARDSIZE][BOARDSIZE], uint8 oppPlayer[BOARDSIZE][BOARDSIZE], int PlayerRow, int PlayerCol);
int CheckMovesValid(int direction, uint8 currPlayer[BOARDSIZE][BOARDSIZE],uint8 oppPlayer[BOARDSIZE][BOARDSIZE],  int PlayerRow, int PlayerCol);
//void Parse(void);
int Check(uint8 p1[BOARDSIZE][BOARDSIZE], uint8 p2[BOARDSIZE][BOARDSIZE],int r, int c);
int OPPCheck(uint8 p1[BOARDSIZE][BOARDSIZE], uint8 p2[BOARDSIZE][BOARDSIZE],int r, int c); 
void ReceivePacket(void);
void PacketUpdate(void);
void PlaceOppTile(void); 
int Sequence(void);
void Parse(uint8 parseData);

/********variables**********/
char array[19] = {"advertise wuyuan12\n"};
char connect[24] ={"connect 192.168.0.102\n"}; 
char *str = NULL;
uint16 count;
int playerFlag = 1; // flag = 1 means player1(red)   flag=0 means player2(blue)
int row = 0, col, passFlag = 1, validMoveFlag = 0;
int enterFlag = 0;
uint8 counter = 0,seq = 0;
char playerID[8] = "wuyuan12";
uint8 dataFormat[19] = {0}, parseData =0, rece20 = 0;
uint8 IdBuffer[8]={}, passBuffer = 0, seqNum = 0, seqBuffer[3] = {};;
int dataIdx = 0, receIdx = 0;
uint8 status = 0, recePkt[19]={0};
int oppRow =0, oppCol = 0, MYTURN = TRUE;
int c1, c2, c3, c4, c5, c6, c7, c8 = 0;
int c11, c22, c33, c44, c55, c66, c77, c88 = 0;
int validPositionFlag = 0;
int idIdx, count1, sCount;
int preSeq,oppSeq1,oppSeq2,oppSeq0,seqPassFlag, oppSeqNum = 0;
uint8 rece55 = 0, receAA = 0;
uint8 currRow, currCol = 0;
uint8 colorBits;
uint8 oppPacket = 0;
int checkPlayerID[8] = {0}, rowBuffer[2] = {0}, colBuffer[2]={0};
int k=0, n=0, m=0, idx = 0;
uint8 parse55, parseAA = 0;
uint8 parseReceivedFlag = 0;
uint8 oppData[19] = {0};


uint8 LMR[BOARDSIZE][BOARDSIZE];
uint8 LMG[BOARDSIZE][BOARDSIZE];
uint8 LMB[BOARDSIZE][BOARDSIZE];
uint8 buffer[USBUART_BUFFER_SIZE];



/************ISR_Functions**********************/
CY_ISR(FIVE00) {
   //  PacketUpdate();
        Pin_1_Write(1);
     TimeControl_Write(1);
     Timer1_ReadStatusRegister();
}

CY_ISR(TX_ISR) {
       //   Pin_1_Write(1);
        UART_WriteTxData(dataFormat[counter]);
     
        counter++;
      
        if(counter >= 19) {
       
            counter = 0;   
            TimeControl_Write(0); 
     
        }
    
}
uint8 rData[19] = {};
int count4=0;
CY_ISR(RX_ISR) {
    // update the packet every 500ms
    PacketUpdate();
  
    if(UART_ReadRxStatus() & UART_RX_STS_FIFO_NOTEMPTY) {
          parseData = UART_ReadRxData();
          rData[count4++] = parseData;
     
          Parse(parseData);
          if(count4 >= 19) {
              rData[count4] = '\0';
              USBUART_PutData(rData, 19);
              count4 = 0;   
       
     
        }
          
    }
}

CY_ISR(LEDS) {
    RefreshLeds();
    Timer_ReadStatusRegister();
}

int main()
{
    
   int i,j;
   for(i=0; i<BOARDSIZE; i++) {
   for(j=0; j<BOARDSIZE; j++) {
       LMB[i][j] = 0;
       LMR[i][j] = 0;
       LMG[i][j] = 0;
   }
}



    currPos.row = 0;
    currPos.col = 0;
      


  
    Timer_Start();
    Timer1_Start();
    LCD_Start();
    UART_Start();
    isr_StartEx(LEDS);
    tx_isr_StartEx(TX_ISR);
    rx_isr_StartEx(RX_ISR);
    five00_isr_StartEx(FIVE00);
  
   
    
    CyGlobalIntEnable; /* Enable global interrupts. */
     
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);
    
     
    //initialize board
    LMB[(BOARDSIZE/2)-1][(BOARDSIZE/2)-1]=1;
    LMR[BOARDSIZE/2 -1][(BOARDSIZE/2 -1)+1]=1;
    LMR[(BOARDSIZE/2 -1)+1][(BOARDSIZE/2) -1]=1;
    LMB[(BOARDSIZE/2 -1)+1][(BOARDSIZE/2 -1)+1]=1;
    for(;;) {
    LCD_Position(1,1);
    LCD_PrintInt8(oppRow);
   // CyDelay(300);
    LCD_PrintInt8(oppCol);
   // CyDelay(300);
    MoveLeds();
   
    }
}


/********************************************* different functions calls ******************************************************************/
/**********************  ********************************************************************************************************************/
/*********************** *******************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

//checks for the sequence, making sure that the opponent's current sequence is euqal to the previous + 1
int Sequence(void) {
    seqPassFlag = 0;
    if(oppSeqNum == 1) {
        preSeq = oppSeqNum; 
        seqPassFlag = 1;
       return 1;
       
    }else if(oppSeqNum == preSeq +1) {
           preSeq = oppSeqNum;
           seqPassFlag = 1;
           return 1;
        
          
        
    } else {
         return 0;
    }
    
}

/*======================setting up data foramt=========================*/
//my packet format
void PacketUpdate(void) {
 dataFormat[0] = 0x55;
    dataFormat[1] = 0xaa;
    dataFormat[2] = playerID[0];
    dataFormat[3] = playerID[1];
    dataFormat[4] = playerID[2];
    dataFormat[5] = playerID[3];
    dataFormat[6] = playerID[4];
    dataFormat[7] = playerID[5];
    dataFormat[8] = playerID[6];
    dataFormat[9] = playerID[7];
    dataFormat[10] = 0x20;
    
    dataFormat[13] = 0x30 + (seq / 100 % 10);
    dataFormat[12] = 0x30 + (seq / 10 % 10);
    dataFormat[11] = 0x30 + (seq % 10);       

    dataFormat[14] = 0x30;

    dataFormat[15] = 0x30 + (currRow / 10) ;
    dataFormat[16] = (0x30 + ((currRow+1) % 10));
    dataFormat[17] = 0x30 + (currCol / 10);
    dataFormat[18] = (0x30 + ((currCol+1) % 10));


}
/*======================setting up data foramt=========================*/
//this is where I place my opponent's tiles
void PlaceOppTile(void) {
    LCD_PrintString("=");
    LCD_PrintInt8(playerFlag);
   //   LCD_PrintInt8(OPPCheck(LMB, LMR) );
      LCD_PrintInt8(MYTURN);
if(playerFlag ==0 &&  OPPCheck(LMB, LMR, oppRow, oppCol) && !MYTURN){
     FindBracket(DIRECTION_DOWN, LMB, LMR, oppRow, oppCol);
     FindBracket(DIRECTION_UP, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_RIGHT, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_LEFT, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_LRC, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_LLC, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_URC, LMB, LMR,oppRow, oppCol);
     FindBracket(DIRECTION_ULC, LMB, LMR,oppRow, oppCol);
     LMB[oppRow][oppCol] = 0x010;  
    playerFlag = playerFlag ^1;
    MYTURN = TRUE;

    }   
}
/*******************************Parse Data format*********************************************/

//I parse the packets that I received, and store it into differnt buffers 
int i, j;
void Parse(uint8 parseData) {
    /***************************statemachine******************/
     switch (state) {
      
        LCD_Position(1,1);
        LCD_PrintInt8(state);
       
        case START_BIT00 : {
         
            if(parseData == 0x55) {
                 
                 state = START_BIT11;
               

            }else {
                
                state = START_BIT00;
            }
          

            break;
        }
        case START_BIT11 : {
            if(parseData == 0xaa) {
               // parseAA = parseData;
                state = PLAYERIDS;   
            }else {
                state = START_BIT00; 
            }
                   
            break;
        }
        case PLAYERIDS : {
            
            if(parseData != 0x20) {      
                checkPlayerID[k] = parseData;
                k++;
                
                state = PLAYERIDS;
            }else {

                k=0;
                state = SEQ_STATE;   
                 

                }

         break;   
        }
        case SEQ_STATE : { 
          //  if(j <3) {
                 
                seqBuffer[j] = parseData;
                j++; 
                if(j == 2){
                   j=0;
                  oppSeq0 = (seqBuffer[0] & MASK);
                   oppSeq1 = (seqBuffer[1] & MASK);
                   oppSeq2 = (seqBuffer[2]& MASK); 
                   oppSeqNum = (oppSeq0*100) + (oppSeq1*10) + (oppSeq2);
                   sprintf(str, "%d", oppSeqNum);
                USBUART_PutChar(*str);
                   state = PASS; 
                }
                 state = SEQ_STATE;
                 
               // }  
             break;   
        }
        case PASS: {
                passBuffer = parseData;
                state = ROW_POS;

            break;
        }
        case ROW_POS : {
           //  LCD_PrintInt8(parseData);

          //  if(n <2) {
                rowBuffer[n] = parseData;

                
                n++;
                
                if(n == 2) {
                    n=0;   
               
                    oppRow = (((rowBuffer[0] & MASK) * 10) + (((rowBuffer[1]) & MASK)-1));
                    
                    LCD_PrintString("r");
                    LCD_PrintNumber(oppRow);
                     
                    state = COL_POS;  
                }
                state = ROW_POS;
                
          //  }

                                
                         
            break;
        }
        case COL_POS : {

         //    if(m <2) {
                colBuffer[m] = parseData;
                m++;
               
                if(m==2) {
                   oppCol = (((colBuffer[0] & MASK) * 10) + (((colBuffer[1]) & MASK)-1));
                    LCD_PrintString("c");
                 LCD_PrintNumber(oppCol);
                   m=0;
                   parseReceivedFlag = 1;
                   if(Sequence()) {
                         PlaceOppTile();
               
                   }
                  state = START_BIT00; 
                }
                 state = COL_POS;
           
            // }
        
            break;
        }
        
        
        

            
    
        
    }
    
   
    
}
/******check for valid move and change opp's tiles**********/
//I flip my opponent's tile if their move is valid
int FindBracket(int direction, uint8 currPlayer[BOARDSIZE][BOARDSIZE], uint8 oppPlayer[BOARDSIZE][BOARDSIZE], int PlayerRow, int PlayerCol) {
    int bracket = 0, bracket1 = 0;
    //counter for opponent's tile. 
    int oppIndex = 0;
    int tmpRow = 0, tmpCol = 0;
    tmpRow = PlayerRow;
    tmpCol = PlayerCol;

                

    //check down direction to turn other player's tile
    if(direction == DIRECTION_DOWN) {
        bracket = tmpRow+1;
        if(currPlayer[bracket][PlayerCol]==0 &&  oppPlayer[bracket][PlayerCol]) {
            while(oppPlayer[bracket][PlayerCol] && bracket<BOARDSIZE) {
                bracket++;
            }
            if(currPlayer[bracket][PlayerCol] && bracket<BOARDSIZE) {  
                oppIndex = bracket - PlayerRow;
                //LCD_Position(1,0);

                while(oppIndex > 0) {
                    oppPlayer[tmpRow][PlayerCol] = 0; 
                    currPlayer[tmpRow][PlayerCol] = 1;  
                    tmpRow++;
                    oppIndex--;
                }
            }
        }
    }           
         
    //check up direction to turn other player's tile
    else if(direction == DIRECTION_UP) {
        bracket = tmpRow-1;
        if((currPlayer[bracket][PlayerCol]==0) &&  (oppPlayer[bracket][PlayerCol])) {
            while(oppPlayer[bracket][PlayerCol]&& bracket>0) {
                bracket--;

            }

            if(currPlayer[bracket][PlayerCol]&& bracket>=0) {    
              //  validMoveFlag = 1;
                oppIndex =(bracket - PlayerRow) *-1;
                while(oppIndex >= 0) {
                    oppPlayer[tmpRow][PlayerCol] = 0; 
                    currPlayer[tmpRow][PlayerCol] = 1;  
                    tmpRow--;
                    oppIndex--;
                    
                }
            }
        }
    }else if(direction == DIRECTION_RIGHT) {       
        bracket = tmpCol+1; 
        if(currPlayer[PlayerRow][bracket]==0 &&  oppPlayer[PlayerRow][bracket]) {
            while(oppPlayer[PlayerRow][bracket]&& bracket<BOARDSIZE) {
                bracket++;

            }
            if(currPlayer[PlayerRow][bracket]&& bracket<BOARDSIZE) {  
                oppIndex = bracket - PlayerCol;

                while(oppIndex > 0) {
                    oppPlayer[PlayerRow][tmpCol] = 0; 
                    currPlayer[PlayerRow][tmpCol] = 1;  
                    tmpCol++;
                    oppIndex--;
                  
                }
            }
        }
    } else if(direction == DIRECTION_LEFT) {       
        bracket = tmpCol-1;
        if(currPlayer[PlayerRow][bracket]==0 &&  oppPlayer[PlayerRow][bracket]) {
            while(oppPlayer[PlayerRow][bracket]&& bracket>0) {
                bracket--;

            }
            if(currPlayer[PlayerRow][bracket]&& bracket>=0) {  
              
                oppIndex = (bracket - PlayerCol) * -1;
             

                while(oppIndex > 0) {
                    oppPlayer[PlayerRow][tmpCol] = 0; 
                    currPlayer[PlayerRow][tmpCol] = 1;  
                    tmpCol--;
                    oppIndex--;
                  //   Pin_1_Write(1);
                }
            }
        }
    }else if(direction == DIRECTION_LRC) {       
        bracket = tmpCol+1;
        bracket1 = tmpRow+1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1<BOARDSIZE) {
                bracket++;
                bracket1++;

            }
            if(currPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1<BOARDSIZE) {      
                oppIndex = bracket - PlayerCol;
                while(oppIndex > 0) {
                    oppPlayer[tmpRow][tmpCol] = 0; 
                    currPlayer[tmpRow][tmpCol] = 1;  
                    tmpCol++;
                    tmpRow++;
                    oppIndex--;
                  //   Pin_1_Write(1);
                }
            }
        }
    }else if(direction == DIRECTION_LLC) {       
        bracket = tmpCol-1;
        bracket1 = tmpRow+1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket>0 && bracket1<BOARDSIZE) {
                bracket--;
                bracket1++;

            }
            if(currPlayer[bracket1][bracket]&& bracket>=0 && bracket1<BOARDSIZE) {  
               
                oppIndex = (bracket - PlayerCol) * -1;
              

                while(oppIndex > 0) {
                    oppPlayer[tmpRow][tmpCol] = 0; 
                    currPlayer[tmpRow][tmpCol] = 1;  
                    tmpCol--;
                    tmpRow++;
                    oppIndex--;
                  
                }
            }
        }
    }else if(direction == DIRECTION_URC) {     

                  
        bracket = tmpCol+1;
        bracket1 = tmpRow-1;
 
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1>0) {
                bracket++;
                bracket1--;

            }


            if(currPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1>=0) {  
               
                oppIndex = bracket - PlayerCol;

                while(oppIndex > 0) {
                    oppPlayer[tmpRow][tmpCol] = 0; 
                    currPlayer[tmpRow][tmpCol] = 1;  
                    tmpCol++;
                    tmpRow--;
                    oppIndex--;
                  
                }
            }
        }
    } else if(direction == DIRECTION_ULC) {       
        bracket = tmpCol-1;
        bracket1 = tmpRow-1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket>0 && bracket1>0) {
                bracket--;
                bracket1--;

            }

            if(currPlayer[bracket1][bracket]&& bracket>=0 && bracket1>=0) {  
               
                oppIndex = (bracket - PlayerCol) * -1;
              

                while(oppIndex > 0) {

                    oppPlayer[tmpRow][tmpCol] = 0; 
                    currPlayer[tmpRow][tmpCol] = 1;  
                    tmpCol--;
                    tmpRow--;
                    oppIndex--;
                  //   Pin_1_Write(1);
                }
            }
        }
    }
    
    
    
    
    return 1;
}
/***************Check valid moves*********/
//Look at all directions of the title and iterate through the tiles to check if it is encapsolated by 
//current palyer's title, if so, then validmoveflag is high, which means it is a valid move to make.
int CheckMovesValid(int direction, uint8 currPlayer[BOARDSIZE][BOARDSIZE], uint8 oppPlayer[BOARDSIZE][BOARDSIZE], int PlayerRow, int PlayerCol) {
    int bracket = 0, bracket1 = 0;
    int tmpRow = 0, tmpCol = 0;
    tmpRow = PlayerRow;
    tmpCol = PlayerCol;
    validPositionFlag = 0;
    
    //check down direction to turn other player's tile
    if(direction == DIRECTION_DOWN) {
        bracket = tmpRow+1;
        if(currPlayer[bracket][PlayerCol]==0 &&  oppPlayer[bracket][PlayerCol]) {
            while(oppPlayer[bracket][PlayerCol]&& bracket<BOARDSIZE) {
                bracket++;

            }

            if(currPlayer[bracket][PlayerCol]&& bracket<BOARDSIZE) {  
                 validPositionFlag = 1;
            
            }
        }
    }           
         
    //check up direction to turn other player's tile
    else if(direction == DIRECTION_UP) {
           bracket = tmpRow-1;
        if((currPlayer[bracket][PlayerCol]==0) &&  (oppPlayer[bracket][PlayerCol])) {
       //   LCD_Position(0,6);
          
            while(oppPlayer[bracket][PlayerCol]&& bracket>0) {
                bracket--;
            }
            
            if((currPlayer[bracket][currPos.col])&& bracket>=0) {    
                validPositionFlag = 1;
            }
        }
    } else if(direction == DIRECTION_RIGHT) {
        bracket = tmpCol+1;
        if(currPlayer[PlayerRow][bracket]==0 &&  oppPlayer[PlayerRow][bracket]) {
            while(oppPlayer[PlayerRow][bracket]&& bracket<BOARDSIZE) {
                bracket++;

            }
            if(currPlayer[PlayerRow][bracket]&& bracket<BOARDSIZE) {  
                 validPositionFlag = 1;
            }
        }
    } else if(direction == DIRECTION_LEFT) {       
        bracket = tmpCol-1;
        if(currPlayer[PlayerRow][bracket]==0 &&  oppPlayer[PlayerRow][bracket]) {
            while(oppPlayer[PlayerRow][bracket]&& bracket>0) {
                bracket--;

            }
            if(currPlayer[PlayerRow][bracket]&& bracket>=0) {  
                validPositionFlag =1;
               
            }
        }
    } else if(direction == DIRECTION_LRC) {       
        bracket = tmpCol+1;
        bracket1 = tmpRow+1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket<BOARDSIZE) {
            while(oppPlayer[bracket1][bracket]) {
                bracket++;
                bracket1++;

            }
            if(currPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket<BOARDSIZE) {  
                validPositionFlag = 1;
            }
        }
    } else if(direction == DIRECTION_LLC) {       
        bracket = tmpCol-1;
        bracket1 = tmpRow+1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket>0 && bracket1<BOARDSIZE) {
                bracket--;
                bracket1++;

            }
            if(currPlayer[bracket1][bracket]&& bracket>=0 && bracket1<BOARDSIZE) {      
                 validPositionFlag = 1;
            }
        }
    }else if(direction == DIRECTION_URC) {       
        bracket = tmpCol+1;
        bracket1 = tmpRow-1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1>0) {
            while(oppPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1>0) {
                bracket++;
                bracket1--;

            }
            if(currPlayer[bracket1][bracket]&& bracket<BOARDSIZE && bracket1>=0) {  
                validPositionFlag = 1;
            }
        }
    }else if(direction == DIRECTION_ULC) {       
        bracket = tmpCol-1;
        bracket1 = tmpRow-1;
        if(currPlayer[bracket1][bracket]==0 &&  oppPlayer[bracket1][bracket]&&  oppPlayer[bracket1][bracket]) {
            while(oppPlayer[bracket1][bracket]&& bracket>0 && bracket1 >0) {
                bracket--;
                bracket1--;

            }

            if(currPlayer[bracket1][bracket]&& bracket>=0 && bracket1 >=0) {  
                 validPositionFlag = 1;
            }
        }
    }else {
        validPositionFlag = 0;
    }
    return validPositionFlag;
}


/****it's empty slot, not over lapping other tiles***/
int IsEmptySlot(void) {
    //check for the boundries
    if(currPos.row<0 || currPos.row > BOARDSIZE-1 
        || currPos.col<0 || currPos.col > BOARDSIZE-1) {
  
         return 0;   
    }
        
    //check to see if you can place a tile on the board
    if(LMR[currPos.row][currPos.col] == 0 && LMB[currPos.row][currPos.col] == 0)  {
//        if(playerFlag) {
//        validMoveFlag = Check(LMR, LMB);
//        }else{
//         validMoveFlag = Check(LMB, LMR);
//        }                  
        return 1;  
    }else{
                   
        return 0;
    }
}

int Check(uint8 p1[BOARDSIZE][BOARDSIZE], uint8 p2[BOARDSIZE][BOARDSIZE],int r, int c) {
         c1=   CheckMovesValid(DIRECTION_DOWN, p1, p2, r, c);
         c2=   CheckMovesValid(DIRECTION_UP, p1, p2, r, c);
         c3=   CheckMovesValid(DIRECTION_RIGHT, p1, p2, r, c);
         c4=   CheckMovesValid(DIRECTION_LEFT, p1, p2, r, c);
         c5=   CheckMovesValid(DIRECTION_LRC, p1, p2, r, c);
         c6=   CheckMovesValid(DIRECTION_LLC, p1, p2, r, c);
         c7=   CheckMovesValid(DIRECTION_URC, p1, p2, r, c);
         c8=   CheckMovesValid(DIRECTION_ULC, p1, p2, r, c);

        return c1 | c2 | c3 | c4 | c5 | c6 | c7 | c8;
}

int OPPCheck(uint8 p1[BOARDSIZE][BOARDSIZE], uint8 p2[BOARDSIZE][BOARDSIZE], int r, int c) {
         c11=   CheckMovesValid(DIRECTION_DOWN, p1, p2, r, c);
         c22=   CheckMovesValid(DIRECTION_UP, p1, p2, r, c);
         c33=   CheckMovesValid(DIRECTION_RIGHT, p1, p2, r, c);
         c44=   CheckMovesValid(DIRECTION_LEFT, p1, p2, r, c);
         c55=   CheckMovesValid(DIRECTION_LRC, p1, p2, r, c);
         c66=   CheckMovesValid(DIRECTION_LLC, p1, p2, r, c);
         c77=   CheckMovesValid(DIRECTION_URC, p1, p2, r, c);
         c88=   CheckMovesValid(DIRECTION_ULC, p1, p2, r, c);

        return c11 | c22 | c33 | c44 | c55 | c66 | c77 | c88;
}
/********refresh leds, basically lab5******/
void RefreshLeds(void) {
      OE_Write(1);
      LAT_Write(0);
      CLK_Write(0);
      RowSelect_Write(row);
      for(col=0; col <32; col++) {
        colorBits = 0;
        if(col <BOARDSIZE){                     
            //Turn on LED according to the coordinates
            if(LMR[row][col]) { 
                   colorBits = colorBits | 0b001; //turn on red  
            }
            if(LMB[row][col]) {
                    colorBits = colorBits | 0b010; //turn on blue 
            } 
            //creating the currsor and toggle currs
            if((col == currPos.col) && (row == currPos.row)) {
                if(playerFlag) {
                    colorBits = 0b001;   
                }else{
                    colorBits = 0b010; 
                }
            }
        }
        // choose wich section to use, row 0-8 or 9-16
        if(row < 8 ) {
            ColorSelect_Write(colorBits);
        }else{
            ColorSelect2_Write(colorBits);
        }
        CLK_Write(1);
        CLK_Write(0);
     }

     LAT_Write(1);//pulse latch once
     LAT_Write(0);
     OE_Write(0);//set OE low, enable output
     row++;
     //reset row count
     if(row == BOARDSIZE) {
           row = 0; 
     }
}

void MoveLeds(void) {
   // for(;;) {
          
    
            /* Host can send double SET_INTERFACE request. */
        if (0u != USBUART_IsConfigurationChanged())
        {
            /* Initialize IN endpoints when device is configured. */
            if (0u != USBUART_GetConfiguration())
            {
                /* Enumeration is done, enable OUT endpoint to receive data 
                 * from host. */
                USBUART_CDC_Init();
            }
        }

        /* Service USB CDC when device is configured. */
        if (0u != USBUART_GetConfiguration())
        {
            /* Check for input data from host. */
            if (0u != USBUART_DataIsReady())
            {
                /* Read received data and re-enable OUT endpoint. */
                count = USBUART_GetAll(buffer);

                if (0u != count)
                {
                    /* Wait until component is ready to send data to host. */
                    while (0u == USBUART_CDCIsReady())
                    {
                    }

                    /* Send data back to host. */
              //      USBUART_PutData(buffer, count);
                    
                    //moves LED using arrow keys
                 //   MoveLeds();

                        switch((int)buffer[0]) {
                            case 'p' : {
                                UART_PutString(connect);   
                            }
                            case ADV : {
                                UART_PutString(array);
                             break;   
                            }
                           case MOVERIGHT : { 
                               
                               currPos.col++;

                              
                               break;
                           }
                        
                           case MOVELEFT : {
                               currPos.col--;

                               break;
                           }
                           
                           case MOVEUP : {
                               currPos.row--;

                               break;
                           }
                           
                           case MOVEDOWN : {
                               currPos.row++;

                               break;
                           }
                         
                           case HOME: {
                               currPos.row = 0;
                               currPos.col = 0;

                               break;  
                          }
                        
                           case S_KEY: {
                            //pass this round to opponent
                                if(playerFlag) {                    
                                    playerFlag = 0;
                                }else{
                                    playerFlag = 1;
                                }

                               
                               break;
                          }
                        
                           case ENTER : {
                               //if it is a valid move
                                   
                              currRow = currPos.row;
                              currCol = currPos.col;
                              LCD_PrintString("*");
                               if(IsEmptySlot() && MYTURN){
                                
                                //  seq++;
                                  //flag=1(player1/red) then turn opp's tiles to red, else to blue
                                  if(playerFlag && Check(LMR, LMB, currRow, currCol)){
                                     seq++;
                                     FindBracket(DIRECTION_UP, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_DOWN, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_RIGHT, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_LEFT, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_LRC, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_LLC, LMR, LMB,currRow, currCol);
                                     FindBracket(DIRECTION_URC, LMR, LMB, currRow, currCol);
                                     FindBracket(DIRECTION_ULC, LMR, LMB, currRow, currCol);
                                     LMR[currRow][currCol] = 0x001;
                                    playerFlag = playerFlag ^1;
                                    MYTURN = FALSE;
                                  
                                  }
                            
                            }
                               break;
                        } 
                        
                     
                        }
                        if((currPos.col < 0) ) {
                            currPos.col = 0;   
                        }else if (currPos.col >= BOARDSIZE-1  ) {
                            currPos.col = BOARDSIZE-1;   
                        }
                        
                        if(currPos.row < 0) {
                            currPos.row = 0;
                        }else if(currPos.row >= BOARDSIZE-1){
                            currPos.row = BOARDSIZE-1;
                        }

                
            }


      
        }
    
        } 

 //}//end of for loop
}
 
/* [] END OF FILE */
