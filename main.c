#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INFECTED_DURATION 4
#define IMMUNE_DURATION 1

#define TOTAL_ARGUMENT_COUNT 4

// #define DEBUG

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
void personScan(FILE *file, Person *person, SimulationData *simulation);
void personPrintToFile(FILE *file, Person *person, SimulationData *simulation);
void personPrintToConsole(Person *person, SimulationData *simulation);
void updateLocation(Person *person, SimulationData *simulation);
void computeNextStatus(Person *person, int index, SimulationData *simulation);
void updateStatus(Person *person);
void simulateSerial(Person *person, SimulationData *simulation);

int main(int argc, const char *argv[]) {
    if(argc != TOTAL_ARGUMENT_COUNT) {
        Usage();
    }

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
    
    // allocate memory for Person array
    Person *person = malloc(simulation.numberOfPersons * sizeof(Person));
    if(!person) {
        printf("Eroare la alocare array Person\n");
        exit(-1);
    }

    personScan(inputFile, person, &simulation);

    if(fclose(inputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    struct timespec start, finish;
    double time = 0;
    
    printf("Measuring Serial...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);

    simulateSerial(person, &simulation);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    time = (finish.tv_sec - start.tv_sec);
    time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Time: %lf\n", time);

    FILE *outputFile = fopen(outputPath, "w");
    if(!outputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    personPrintToFile(outputFile, person, &simulation);

    if(fclose(outputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    return 0;
}

/*-----------------------------------------------------------------
 * Function:  Simulate Serial
 * Purpose:   Simulates the serial version of the algorithm
 * In args:   person, simulation
 */
void simulateSerial(Person *person, SimulationData *simulation) {
    // each time step
    for(int time=0;time<simulation->simulationTime;time++) {
        #ifdef DEBUG
            personPrintToConsole(person, simulation);
            printf("\n");
        #endif

        // update locations
        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateLocation(&(person[i]), simulation);
        }

        for(int i=0;i<simulation->numberOfPersons;i++) {
            computeNextStatus(person, i, simulation);
        }

        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateStatus(&(person[i]));
        }

    }
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
 * Purpose:   Input data into person array from file with pathname given by path parameter. Also updates the status of the person read in order to initialize both status and nextStatus
 * In args:   path, person
 */
void personScan(FILE *file, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->numberOfPersons;i++) {
        fscanf(file, "%d", &(person[i].personID));
        fscanf(file, "%d %d", &(person[i].coord.x), &(person[i].coord.y));
        fscanf(file, "%d", &(person[i].nextStatus));
        fscanf(file, "%d", &(person[i].movementDirection));
        fscanf(file, "%d", &(person[i].movementAmplitude));
        person[i].infectionCounter = 0;
        person[i].statusDuration = 0;

        updateStatus(&(person[i]));
        if(person->statusDuration != 0) {
            person->statusDuration++;
        }
    }
}

/*-----------------------------------------------------------------
 * Function:  Person Print To File
 * Purpose:   Output data for all persons into a file
 * In args:   file, path, person
 */
void personPrintToFile(FILE *file, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->numberOfPersons;i++) {
        fprintf(file, "id:%d ", person[i].personID);
        fprintf(file, "x:%d y:%d ", person[i].coord.x, person[i].coord.y);
        fprintf(file, "st:%d ", person[i].status);
        fprintf(file, "mD:%d ", person[i].movementDirection);
        fprintf(file, "mA:%d ", person[i].movementAmplitude);
        fprintf(file, "iC:%d ", person[i].infectionCounter);
        fprintf(file, "sD:%d\n", person[i].statusDuration);
    }
}

/*-----------------------------------------------------------------
 * Function:  Person Print To Console
 * Purpose:   Output data for all persons to console
 * In args:   path, person
 */
void personPrintToConsole(Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->numberOfPersons;i++) {
        printf("id:%d ", person[i].personID);
        printf("x:%d y:%d ", person[i].coord.x, person[i].coord.y);
        printf("st:%d ", person[i].status);
        printf("mD:%d ", person[i].movementDirection);
        printf("mA:%d ", person[i].movementAmplitude);
        printf("iC:%d ", person[i].infectionCounter);
        printf("sD:%d\n", person[i].statusDuration);
    }
}

/*-----------------------------------------------------------------
 * Function:  Update Location
 * Purpose:   Update the location of a person and take care of out of test area
 * In args:   person
 */
void updateLocation(Person *person, SimulationData *simulation) {
    switch (person->movementDirection) {
        case NORTH:
            person->coord.x += person->movementAmplitude;
            if (person->coord.x >= simulation->maxXCoord) {
                person->coord.x = simulation->maxXCoord - 1;
                person->movementDirection ^= 1;
            }
            break;
        case SOUTH:
            person->coord.x -= person->movementAmplitude;
            if (person->coord.x < 0) {
                person->coord.x = 0;
                person->movementDirection ^= 1;
            }
            break;
        case EAST:
            person->coord.y += person->movementAmplitude;
            if (person->coord.y >= simulation->maxYCoord) {
                person->coord.y = simulation->maxYCoord - 1;
                person->movementDirection ^= 1;
            }
            break;
        case WEST:
            person->coord.y -= person->movementAmplitude;
            if (person->coord.y < 0) {
                person->coord.y = 0;
                person->movementDirection ^= 1;
            }
            break;
        default:
    }
}

/*-----------------------------------------------------------------
 * Function:  Compute NextStatus
 * Purpose:   Compute the next status for a person
 * In args:   person
 */
void computeNextStatus(Person *person, int index, SimulationData *simulation) {
    switch (person[index].status) {
        case INFECTED:
            for (int i=0; i<simulation->numberOfPersons; i++) {
                if (i != index) {
                    if (person[i].status == SUSCEPTIBLE) {
                        if (person[i].coord.x == person[index].coord.x) {
                            if (person[i].coord.y == person[index].coord.y) {
                                person[i].nextStatus = INFECTED;
                            }
                        }
                    }
                }
            }
            if (person[index].statusDuration == 0) {
                person[index].nextStatus = IMMUNE;
            }
            break;
        case IMMUNE:
            if (person[index].statusDuration == 0) {
                person[index].nextStatus = SUSCEPTIBLE;
            }
            break;
        case SUSCEPTIBLE:
            break;
        default:
    }
}

/*-----------------------------------------------------------------
 * Function:  Update Status
 * Purpose:   Update the status to next status of a persons
 * In args:   person
 */
void updateStatus(Person *person) {
    person->status = person->nextStatus;
    switch (person->status) {
        case INFECTED:
            if (person->statusDuration == 0) {
                person->statusDuration = INFECTED_DURATION - 1;
                person->infectionCounter++;
            }
            else {
                person->statusDuration--;
            }
            break;
        case SUSCEPTIBLE:
            break;
        case IMMUNE:
            if (person->statusDuration == 0) {
                person->statusDuration = IMMUNE_DURATION - 1;
            }
            else {
                person->statusDuration--;
            }
            break;
        default:
    }
}