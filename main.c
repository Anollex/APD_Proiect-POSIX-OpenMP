#include <stdio.h>
#include <stdlib.h>

#define INFECTED_DURATION 3
#define IMMUNE_DURATION 5

// #define DEBUG 1

typedef struct {
    int maxXCoord;
    int maxYCoord;
    int numberOfPersons;
    int simulationTime;
}SimulationData;

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


void personScan(FILE *file, Person *person);
void updateLocation(Person person);
void computeNextStatus(Person person);
void updateStatus(Person person);


int main(int argc, const char *argv[]) {
    Person person[10];
    char* path = argv[2];

    FILE *inputFile = fopen(path, "r");
    if(!inputFile) {
        perror("File not found!\n");
        exit(-1);
    }

    

    if(fclose(inputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    return 0;
}

/*-----------------------------------------------------------------
 * Function:  Person Scan
 * Purpose:   Input data into Person array from file with pathname given by path parameter 
 * In args:   path, person
 */
void personScan(FILE *file, Person *person) {

}

/*-----------------------------------------------------------------
 * Function:  Simulation Init
 * Purpose:   Initializes the simulation data
 * In args:   simulation, simulationTime
 */
void simulationInit(SimulationData simulation, int simulationTime) {
    // read data

}

/*-----------------------------------------------------------------
 * Function:  Person Print
 * Purpose:   Output the data  of all persons
 * In args:   path, person
 */
void personPrint(char *path, Person *person) {

}

/*-----------------------------------------------------------------
 * Function:  Update Location
 * Purpose:   Update the location of a person and take care of out of test area
 * In args:   person
 */
void updateLocation(Person person) {

}

/*-----------------------------------------------------------------
 * Function:  Compute NextStatus
 * Purpose:   Compute the next status for a person
 * In args:   person
 */
void computeNextStatus(Person person) {

}

/*-----------------------------------------------------------------
 * Function:  Update Status
 * Purpose:   Update the status to next status of a persons
 * In args:   person
 */
void updateStatus(Person person) {
}