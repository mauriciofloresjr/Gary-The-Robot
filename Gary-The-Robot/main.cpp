#include "mbed.h"
#include "nRF24L01P.h"
#include <cstdio>

nRF24L01P my_nrf24l01p(D11, D12, D13, D2, D9, D10);  // mosi, miso, sck, csn, ce, irq
int TrCounter=0;

DigitalOut lwb(D4);
DigitalOut lwf(D5);
DigitalOut rwb(D6);
DigitalOut rwf(D7);

PwmOut RW(D3); //D3
PwmOut LW(D8);

Timer moveTimer;

bool finished = false;

float RWSpeed = .331; //.331
float LWSpeed = .3; //.3

float timeToMove = 1.5; //Time it takes to move between squares in seconds
float timeToTurn = 0.65; //Time it takes to turn 90 degrees in seconds

int startPos[2] = {0,0}; //Starting [X,Y] Position on grid
int currPos[2] = {0,0}; //Current [X,Y] Position on grid
int goalPos[2] = {8,8}; //Goal [X,Y] Position on grid
int route[2] = {0,0}; //Rise and Run parameters to reach goal

void move_stop() {
    lwb = 0;
    lwf = 0;
    rwb = 0;
    rwf = 0;
}

void move_backward() {
    RW = 0.5;
    RW.period(0.25);
    LW = 0.5;
    LW.period(0.25);

    lwb = 1;
    lwf = 0;
    rwb = 1;
    rwf = 0;

    moveTimer.start();

    while(!finished){
        if(moveTimer.read() >= timeToMove) {
            moveTimer.stop();
            moveTimer.reset();
            move_stop();
            finished = true;
        }   
    }
    finished = false;
}

void turn_left(int speed) {
    RW.period(.1);
    LW.period(.1);
    
    RW = abs(RWSpeed * speed);
    LW = abs(LWSpeed * speed);
    
    lwb = 1;
    lwf = 0;
    rwb = 0;
    rwf = 1;

    moveTimer.start();

    while(!finished){
        if(moveTimer.read() >= timeToTurn) {
            moveTimer.stop();
            moveTimer.reset();
            move_stop();
            finished = true;
        }   
    }
    finished = false;
}

void turn_right(int speed) {
    RW.period(.1);
    LW.period(.1);
    
    RW = abs(RWSpeed * speed);
    LW = abs(LWSpeed * speed);
    
    lwb = 0;
    lwf = 1;
    rwb = 1;
    rwf = 0;
    
    moveTimer.start();

    while(!finished){
        if(moveTimer.read() >= timeToTurn) {
            moveTimer.stop();
            moveTimer.reset();
            move_stop();
            finished = true;
        }   
    }
    finished = false;
}

void move_forward(int distance) {
    RW.period(0.1f);
    LW.period(0.1f);

    RW = RWSpeed;
    LW = LWSpeed;

    for (int i = 0; i < distance; i++) {
        lwb = 0;
        lwf = 1;
        rwb = 0;
        rwf = 1;

        moveTimer.start();

        while(!finished){
            if(moveTimer.read() >= timeToMove) {
                moveTimer.stop();
                moveTimer.reset();
                move_stop();
                finished = true;
            } 
        }
        finished = false;  
    }
}

// Calulates the required rise and run to go from starting position to goal position
void calculateRoute(int startPos[], int goalPos[]) {
    int routeX = goalPos[0] - startPos[0];
    int routeY = goalPos[1] - startPos[1];
    route[0] = routeX;
    route[1] = routeY;
}

void celebrate() {
    turn_right(2);
    turn_left(2);
    turn_right(2);
    turn_left(2);
    turn_right(2);
    turn_left(2);
}

int main() {
    #define TRANSFER_SIZE 4
    char rxData[TRANSFER_SIZE];
    int rxDataCnt = 0;
    my_nrf24l01p.powerUp();
    my_nrf24l01p.setRfOutputPower(-6);
    my_nrf24l01p.setRxAddress((0x0000000070),DEFAULT_NRF24L01P_ADDRESS_WIDTH); //0x1F22676D92
    my_nrf24l01p.setAirDataRate(2000);
    // Display the (default) setup of the nRF24L01+ chip
    printf( "nRF24L01+ Frequency    : %d MHz\r\n",  my_nrf24l01p.getRfFrequency() );
    printf( "nRF24L01+ Output power : %d dBm\r\n",  my_nrf24l01p.getRfOutputPower() );
    printf( "nRF24L01+ Data Rate    : %d kbps\r\n", my_nrf24l01p.getAirDataRate() );
    printf( "nRF24L01+ RX Address  : 0x%010llX\r\n", my_nrf24l01p.getRxAddress() );
    my_nrf24l01p.setTransferSize( TRANSFER_SIZE );
    my_nrf24l01p.setReceiveMode();
    my_nrf24l01p.enable();
     
    while (1) {
          // If we've received anything in the nRF24L01+...
        if (my_nrf24l01p.readable()) {
            // ...read the data into the receive buffer
            my_nrf24l01p.read( NRF24L01P_PIPE_P0, rxData, sizeof( rxData ) );
            
            startPos[0] = rxData[0]; // X Coord
            startPos[1] = rxData[1]; // Y Coord

            goalPos[0] = rxData[2]; // X Coord
            goalPos[1] = rxData[3]; // Y Coord

            calculateRoute(startPos, goalPos); //calculate route based on difference between goal and start

            move_forward(route[1]); //move forward for Y units of calculated route
            turn_right(1);
            move_forward(route[0]); //move forward for X units of calculated route
        }
    }
    
}
