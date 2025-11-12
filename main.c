#include <stdio.h>
#include <stdlib.h>

#define INFECTED_DURATION 3
#define IMMUNE_DURATION 5

// #define DEBUG 1

typedef struct {
    int x;
    int y;
}Coordinate;

typedef struct {
    int personID;
    Coordinate coord;
    int status;
    int nextStatus;
    int statusDuration;
    int movementDirection;
    int movementAmplitude;
    int infectionCounter;
}Person;

typedef enum {
    INFECTED,
    SUSCEPTIBLE,
    IMMUNE
}Status;

typedef enum {
    NORTH,  //00
    SOUTH,  //01
    EAST,   //10
    WEST    //11 ^ 0x0001 = 10
}Directions;

void updateLocation(Person person);

int main(int argc, const char * argv[]) {
    Person person[10];
    int maxXCoord = 0;
    int maxYCoord = 0;
    int numOfPersons = 0;
    int threadCount = atoi(argv[3]);
    int simulationTime = atoi(argv[1]);
    char* path = argv[2];


    return 0;
}

/*-----------------------------------------------------------------
 * Function:  Update Location
 * Purpose:   Print if the list are not the same and exit with -1 code
 * In args:   person
 */
void updateLocation(Person person) {
}