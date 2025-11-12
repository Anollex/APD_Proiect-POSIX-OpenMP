#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFECTED_DURATION 3
#define IMMUNE_DURATION 5

#define TOTAL_ARGUMENT_COUNT 4

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

void Usage();
void simulationScan(FILE *file, SimulationData *simulation);
void personScan(FILE *file, Person *person, SimulationData simulation);
void personPrint(FILE *file, Person *person, SimulationData simulation);
void updateLocation(Person person);
void computeNextStatus(Person person);
void updateStatus(Person person);

int main(int argc, const char *argv[]) {
    if(argc != TOTAL_ARGUMENT_COUNT) {
        Usage();
    }

    Person person[10];
    SimulationData simulation;
    char *path = argv[2];
    char *outputPath = "file_serial_out.txt";

    FILE *inputFile = fopen(path, "r");
    if(!inputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    // init simulation and read person data
    simulation.simulationTime = atoi(argv[1]);
    simulationScan(inputFile, &simulation);
    personScan(inputFile, person, simulation);

    if(fclose(inputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    FILE *outputFile = fopen(outputPath, "w");
    if(!outputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    personPrint(outputFile, person, simulation);

    if(fclose(inputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    return 0;
}

/*-----------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Show and explain usage of the executable program and its command line arguments
 */
void Usage() {
    printf("Invalid arguments. Program call should be: ./program_name simulationTime inputFileName threadNumber\n");
    exit(-1);
}

/*-----------------------------------------------------------------
 * Function:  Simulation Scan
 * Purpose:   Read the fields of simulation given in the file parameter (maxXCoords, maxYCoords, numberOfPersons)
 * In args:   file, simulation
 */
void simulationScan(FILE *file, SimulationData *simulation) {
    fscanf(file, "%d %d", &simulation->maxXCoord, &simulation->maxYCoord);
    fscanf(file, "%d", &simulation->numberOfPersons);
}

/*-----------------------------------------------------------------
 * Function:  Person Scan
 * Purpose:   Input data into person array from file with pathname given by path parameter 
 * In args:   path, person
 */
void personScan(FILE *file, Person *person, SimulationData simulation) {
    for(int i=0;i<simulation.numberOfPersons;i++) {
        fscanf(file, "%d", &(person[i].personID));
        fscanf(file, "%d %d", &(person[i].coord.x), &(person[i].coord.y));
        fscanf(file, "%d", &(person[i].status));
        fscanf(file, "%d", &(person[i].movementDirection));
        fscanf(file, "%d", &(person[i].movementAmplitude));
    }
}

/*-----------------------------------------------------------------
 * Function:  Person Print
 * Purpose:   Output data for all persons into a file. This function also modifies the given path to construct
 * In args:   path, person
 */
void personPrint(FILE *file, Person *person, SimulationData simulation) {
    for(int i=0;i<simulation.numberOfPersons;i++) {
        fprintf(file, "%d ", person[i].personID);
        fprintf(file, "%d %d ", person[i].coord.x, person[i].coord.y);
        fprintf(file, "%d ", person[i].status);
        fprintf(file, "%d ", person[i].movementDirection);
        fprintf(file, "%d\n", person[i].movementAmplitude);
    }
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