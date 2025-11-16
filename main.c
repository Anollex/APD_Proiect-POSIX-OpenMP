#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define INFECTED_DURATION 4
#define IMMUNE_DURATION 2

#define TOTAL_ARGUMENT_COUNT 4

#define SERIAL_PATH_SUFFIX "_serial_out.txt"
#define PARALLEL_PATH_SUFFIX "_parallel_out.txt"

// #define DEBUG
// #define DEBUG_GRID

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

typedef struct personNode{
    int personIndex;
    struct personNode *next;
}PersonNode;

typedef struct {
    int rank;
    int threadCount;
    PersonNode ***grid;
    Person *person;
    SimulationData *simulation;
}Parameters;

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

typedef enum {
    STANDARD_PRINT_FORMAT,
    ONLY_NUMBERS_PRINT_FORMAT
}PrintFormats;

void Usage();
void simulationScan(FILE *file, SimulationData *simulation);
void personScan(FILE *file, Person *person, SimulationData *simulation);
void personPrintToFile(FILE *file, Person *person, SimulationData *simulation, int format);
void personPrintToConsole(Person *person, SimulationData *simulation);
void updateLocation(Person *person, SimulationData *simulation);
// void computeNextStatus(Person *person, int index, SimulationData *simulation); // first version
void computeNextStatus(PersonNode ***grid, Person *person, SimulationData *simulation);
void updateStatus(Person *person);
void simulateSerial(PersonNode ***grid, Person *person, SimulationData *simulation);

void initGrid(PersonNode ***grid, Person *person, SimulationData *simulation);
void printPersonNode(PersonNode *node);
void appendPersonNode(int personIndex, PersonNode **gridCell);
void updateGrid(PersonNode ***grid, Person *person, SimulationData *simulation);

void printGrid(PersonNode ***grid, Person *person, SimulationData *simulation);
void printList(PersonNode* node, Person *person);

void freeGrid(PersonNode ***grid, SimulationData *simulation);
void freeList(PersonNode *node);

char *buildOutputPath(char *inputPath, char *suffix);
void checkSolution(char *serialFilePath, char *parallelFilePath);

void computeNextStatusParallel(PersonNode ***grid, Person *person, SimulationData *simulation, Parameters *parameters, pthread_t *threadHandles, int threadCount);
void *computeNextStatusParallel_PosixFunc(void *parameters);
void *updateLocationsParallel_PosixFunc(void *parameters);
void updateStatusParallel(Person *person, SimulationData *simulation, Parameters *parameters, pthread_t *threadHandles, int threadCount);

void simulateParallel(PersonNode ***grid, Person *person, SimulationData *simulation, int threadCount);

int main(int argc, const char *argv[]) {
    if(argc != TOTAL_ARGUMENT_COUNT) {
        Usage();
    }
    
    int threadCount = atoi(argv[3]);
    char *path = argv[2];
    // char *serialOutputPath = "file_serial_out.txt";
    char *serialOutputPath = buildOutputPath(path, SERIAL_PATH_SUFFIX);
    char *parallelOutputPath = buildOutputPath(path, PARALLEL_PATH_SUFFIX);

    FILE *inputFile = fopen(path, "r");
    if(!inputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    // init simulation and read person data
    SimulationData simulation;
    simulation.simulationTime = atoi(argv[1]);
    simulationScan(inputFile, &simulation);

    printf("simulationTime: %d\tnumberOfPersons: %d\tnumberOfThreads: %d\n", simulation.simulationTime, simulation.numberOfPersons, threadCount);

    PersonNode ***grid = malloc(simulation.maxXCoord * sizeof(PersonNode **));
    if(!grid) {
        printf("Eroare la alocare randuri grid\n");
        exit(-1);
    }
    for(int i=0;i<simulation.maxXCoord;i++) {
        grid[i] = malloc(simulation.maxYCoord * sizeof(PersonNode *));
        if(!grid[i]) {
            printf("Eroare la alocare coloana %d din grid\n", i);
            exit(-1);
        }
    }
    
    // allocate memory for Person array
    Person *person = malloc(simulation.numberOfPersons * sizeof(Person));
    Person *personCopy = malloc(simulation.numberOfPersons * sizeof(Person));
    if(!person || !personCopy) {
        printf("Eroare la alocare array Person\n");
        exit(-1);
    }

    personScan(inputFile, person, &simulation);
    memcpy(personCopy, person, simulation.numberOfPersons * sizeof(Person));

    if(fclose(inputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    initGrid(grid, person, &simulation);
    // printGrid(grid, &simulation);

    // Simuare seriala
    struct timespec start, finish;
    double serialTime = 0, parallelTime = 0;

    // printf("Measuring Serial...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);

    simulateSerial(grid, person, &simulation);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    serialTime = (finish.tv_sec - start.tv_sec);
    serialTime += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Serial Time: %lf\n", serialTime);

    FILE *outputFile = fopen(serialOutputPath, "w");
    if(!outputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    personPrintToFile(outputFile, person, &simulation, ONLY_NUMBERS_PRINT_FORMAT);

    if(fclose(outputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    // Simulare paralela
    // Trebuie restaurat person
    memcpy(person, personCopy, simulation.numberOfPersons * sizeof(Person));

    initGrid(grid, person, &simulation);

    // printf("Measuring Parallel...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);

    simulateParallel(grid, person, &simulation, threadCount);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    parallelTime = (finish.tv_sec - start.tv_sec);
    parallelTime += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Parallel Time: %lf\n", parallelTime);

    outputFile = fopen(parallelOutputPath, "w");
    if(!outputFile) {
        printf("File not found!\n");
        exit(-1);
    }

    personPrintToFile(outputFile, person, &simulation, ONLY_NUMBERS_PRINT_FORMAT);

    if(fclose(outputFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    double speedup = serialTime / parallelTime;

    printf("Speedup: %lf\n", speedup);

    checkSolution(serialOutputPath, parallelOutputPath);

    free(person);
    freeGrid(grid, &simulation);

    return 0;
}

/*-----------------------------------------------------------------
 * Function:  Simulate Serial
 * Purpose:   Simulates the serial version of the algorithm
 * In args:   person, simulation
 */
void simulateSerial(PersonNode ***grid, Person *person, SimulationData *simulation) {
    // each time step
    for(int time=0;time<simulation->simulationTime;time++) {
        #ifdef DEBUG
            personPrintToConsole(person, simulation);
            printf("\n");
        #endif

        updateGrid(grid, person, simulation);
    #ifdef DEBUG_GRID
        printGrid(grid, person, simulation);
        printf("\n");
    #endif

        // version 1 compute status
        // for(int i=0;i<simulation->numberOfPersons;i++) {
        //     computeNextStatus(person, i, simulation);
        // }

        computeNextStatus(grid, person, simulation);

        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateStatus(&(person[i]));
        }

        // update locations
        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateLocation(&(person[i]), simulation);
        }
    }
}

/*-----------------------------------------------------------------
 * Function:  Simulate Parallel
 * Purpose:   Simulates the parallel version of the algorithm
 * In args:   person, simulation
 */
void simulateParallel(PersonNode ***grid, Person *person, SimulationData *simulation, int threadCount) {
    // each time step
    for(int time=0;time<simulation->simulationTime;time++) {
        #ifdef DEBUG
            personPrintToConsole(person, simulation);
            printf("\n");
        #endif

        pthread_t *threadHandles = malloc(threadCount * sizeof(pthread_t));
        Parameters *parameters = malloc(threadCount * sizeof(Parameters));
        if (!parameters || !threadHandles) {
            printf("Eroare la alocare memorie pentru thread-uri\n");
            exit(-1);
        }

        updateGrid(grid, person, simulation);
    #ifdef DEBUG_GRID
        printGrid(grid, person, simulation);
        printf("\n");
    #endif

        // version 1 compute status
        // for(int i=0;i<simulation->numberOfPersons;i++) {
        //     computeNextStatus(person, i, simulation);
        // }

        computeNextStatusParallel(grid, person, simulation, parameters, threadHandles, threadCount);

        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateStatus(&(person[i]));
        }

        // update locations
        for(int i=0;i<simulation->numberOfPersons;i++) {
            updateLocation(&(person[i]), simulation);
        }

        free(threadHandles);
        free(parameters);
    }
}

/*-----------------------------------------------------------------
 * Function:  Init Grid
 * Purpose:   Go through all the cells of the grid and initialize them with NULL, then put all persons in the cells they belong to at the start based on current x and y coordinates
 * In args:   grid, person, simulation
 */
void initGrid(PersonNode ***grid, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->maxXCoord;i++) {
        for(int j=0;j<simulation->maxYCoord;j++) {
            grid[i][j] = NULL;
        }
    }
}

void appendPersonNode(int personIndex, PersonNode **gridCell) {
    PersonNode *node = malloc(sizeof(PersonNode));
    if(!node) {
        printf("Eroare la alocare dinamica nod pentru persoana cu index %d\n", personIndex);
        exit(-1);
    }

    node->personIndex = personIndex;
    node->next = *gridCell;
    *gridCell = node;
}

void printGrid(PersonNode ***grid, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->maxXCoord;i++) {
        for(int j=0;j<simulation->maxYCoord;j++) {
            // printPersonNode(grid[i][j]);
            printf("[%d][%d]: ", i, j);
            printList(grid[i][j], person);
        }
        printf("\n");
    }
}

void printList(PersonNode* node, Person *person) {
    PersonNode *traverser = node;
    while(traverser != NULL) {
        printf("(%d, %d) ", traverser->personIndex + 1, person[traverser->personIndex].status);
        traverser = traverser->next;
    }
    printf("\n");
}

void freeList(PersonNode *node) {
    if(node == NULL) return;

    PersonNode *traverser = node;
    while(traverser != NULL) {
        node = node->next;
        free(traverser);
        traverser = node;
    }
}

void printPersonNode(PersonNode *node) {
    if(node == NULL) {
        printf("NULL ");
    } else {
        printf("%d ", node->personIndex);
    }
}

/*-----------------------------------------------------------------
 * Function:  Update Grid
 * Purpose:   Go through all the cells of the grid and update the locations of the persons in each one;
            update consist of erasing the previous values for all cells, setting them to NULL and repopulating the cells by using x and y
 * In args:   grid, person, simulation
 */
void updateGrid(PersonNode ***grid, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->maxXCoord;i++) {
        for(int j=0;j<simulation->maxYCoord;j++) {
            freeList(grid[i][j]);
            grid[i][j] = NULL;
        }
    }

    // parcurg array-ul
    for(int i=0;i<simulation->numberOfPersons;i++) {
        int x = person[i].coord.x;
        int y = person[i].coord.y;
        // printf("%d %d\n", x, y);

        // preiau coordonatele x si y si adaug persoanele intr-o lista simplu inlantuita formata in celula x, y
        appendPersonNode(i, &grid[x][y]);
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
        if(person[i].status == INFECTED) {
            person[i].statusDuration++;
        }
    }
}

/*-----------------------------------------------------------------
 * Function:  Person Print To File
 * Purpose:   Output data for all persons into a file
 * In args:   file, path, person
 */
void personPrintToFile(FILE *file, Person *person, SimulationData *simulation, int format) {
    if(format == STANDARD_PRINT_FORMAT) {
        for(int i=0;i<simulation->numberOfPersons;i++) {
            fprintf(file, "id:%d ", person[i].personID);
            fprintf(file, "x:%d y:%d ", person[i].coord.x, person[i].coord.y);
            fprintf(file, "st:%d ", person[i].status);
            fprintf(file, "mD:%d ", person[i].movementDirection);
            fprintf(file, "mA:%d ", person[i].movementAmplitude);
            fprintf(file, "iC:%d ", person[i].infectionCounter);
            fprintf(file, "sD:%d\n", person[i].statusDuration);
        }
    } else if(format == ONLY_NUMBERS_PRINT_FORMAT) {
        for(int i=0;i<simulation->numberOfPersons;i++) {
            fprintf(file, "%d ", person[i].personID);
            fprintf(file, "%d %d ", person[i].coord.x, person[i].coord.y);
            fprintf(file, "%d ", person[i].status);
            fprintf(file, "%d ", person[i].movementDirection);
            fprintf(file, "%d ", person[i].movementAmplitude);
            fprintf(file, "%d ", person[i].infectionCounter);
            fprintf(file, "%d\n", person[i].statusDuration);
        }
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
 * In args:   grid, person, simulation
 */
void computeNextStatus(PersonNode ***grid, Person *person, SimulationData *simulation) {
    for(int i=0;i<simulation->maxXCoord;i++) {
        for(int j=0;j<simulation->maxYCoord;j++) {
            int infectedFound = 0;

            // verifica fiecare nod din lista
            PersonNode *traverser = grid[i][j];
            while(traverser != NULL) {
                int index = traverser->personIndex;
                switch(person[index].status) {
                    case INFECTED: // daca este infectat seteaza infectedFound si daca durata a ajuns la 0 seteaza urmatoarea stare pe immune
                        infectedFound = 1;
                        if(person[index].statusDuration == 0) {
                            person[index].nextStatus = IMMUNE;
                        }
                        break;
                    case IMMUNE: // daca durata a ajuns la 0 seteaza pe susceptible la urmatoarea stare
                        if(person[index].statusDuration == 0) {
                            person[index].nextStatus = SUSCEPTIBLE;
                        }
                        break;
                    case SUSCEPTIBLE: // nu trebuie facut nimic aici
                        break;
                }

                // update reference
                traverser = traverser->next;
            }
            
            if(infectedFound) { // daca o persoana este infectata vom infecta si celelalte persoane susceptibile din aceeasi celula
                traverser = grid[i][j];
                while(traverser != NULL) {
                    if(person[traverser->personIndex].status == SUSCEPTIBLE)
                        person[traverser->personIndex].nextStatus = INFECTED;

                    // update reference
                    traverser = traverser->next;
                }
            }
        }
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
                person->statusDuration = IMMUNE_DURATION;
            }
            else {
                person->statusDuration--;
            }
            break;
        default:
    }
}

void freeGrid(PersonNode ***grid, SimulationData *simulation) {
    for(int i=0;i<simulation->maxXCoord;i++) {
        free(grid[i]);
    }
    free(grid);
}

int getIndexForChar(char *string, char c) {
    int stringLength = strlen(string);

    for(int i=0;i<stringLength;i++) {
        if(string[i] == c) return i;
    }

    return -1;
}

char *buildOutputPath(char *inputPath, char *suffix) {
    int inputPathLength = strlen(inputPath);
    int suffixLength = strlen(suffix);

    char *ret = malloc((inputPathLength + suffixLength + 1) * sizeof(char));
    if(!ret) {
        printf("Eroare la alocare string de output\n");
        exit(-1);
    }

    int inputPathCutoff = getIndexForChar(inputPath, '.'); // vreau sa copiez fara .txt deci iau indexul punctului ca sa copiez doar pana acolo
    if(inputPathCutoff == -1) {
        printf("Fisierul nu are nume corespunzator - trebuie sa aiba o extensie cu '.'\n");
        exit(-1);
    }

    strncpy(ret, inputPath, inputPathCutoff);
    ret[inputPathCutoff] = '\0';
    strcat(ret, suffix);

    return ret;
}

#define MAX_LINE_BUFFER 100

void checkSolution(char *serialFilePath, char *parallelFilePath) {
    FILE *serialFile = fopen(serialFilePath, "r");
    if(!serialFile) {
        printf("File not found!\n");
        exit(-1);
    }

    FILE *parallelFile = fopen(parallelFilePath, "r");
    if(!parallelFile) {
        printf("File not found!\n");
        exit(-1);
    }

    char serialLine[MAX_LINE_BUFFER];
    char parallelLine[MAX_LINE_BUFFER];
    int line = 0;
    int ok = 1;

    while(1) {
        line++;

        char *serialCheck = fgets(serialLine, MAX_LINE_BUFFER, serialFile);
        char *parallelCheck = fgets(parallelLine, MAX_LINE_BUFFER, parallelFile);

        if(serialCheck == NULL && parallelCheck == NULL) {
            break;
        }

        if(serialCheck == NULL && parallelCheck == NULL) {
            printf("Fisierele nu au aceeasi lungime\n");
            ok = 0;
            break;
        }

        if(strcmp(serialLine, parallelLine) != 0) {
            printf("Liniile %d au lungimi diferite\n", line);
            printf("Serial  : %s", serialLine);
            printf("Parallel: %s", parallelLine);
            ok = 0;
            break;
        }
    }

    if(ok == 1) {
        printf("Rezultatele sunt egale\n");
    } else {
        printf("Rezultatele NU sunt egale\n");        
    }

    if(fclose(serialFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }

    if(fclose(parallelFile) != 0) {
        perror("File could not be closed\n");
        exit(-1);
    }
}

/*-----------------------------------------------------------------
 * Function:  Compute NextStatus
 * Purpose:   Compute the next status for a person
 * In args:   grid, person, simulation
 */
void *computeNextStatusParallel_PosixFunc(void *parameters) {
    Parameters *localParam = (Parameters *)parameters;
    
    PersonNode ***grid = localParam->grid;
    Person *person = localParam->person;
    SimulationData *simulation = localParam->simulation;
    int myRank = localParam->rank;
    int threadCount = localParam->threadCount;

    int rowsPerThread = simulation->maxXCoord / threadCount;
    int firstRow = myRank * rowsPerThread;
    int lastRow = (myRank + 1) * rowsPerThread - 1;
    if(myRank == threadCount - 1) { // suntem la ultima parte => luam si restul in calcul
        lastRow = simulation->maxXCoord - 1;
    }

    for(int i=firstRow;i<=lastRow;i++) {
        for(int j=0;j<simulation->maxYCoord;j++) {
            int infectedFound = 0;

            // verifica fiecare nod din lista
            PersonNode *traverser = grid[i][j];
            while(traverser != NULL) {
                int index = traverser->personIndex;
                switch(person[index].status) {
                    case INFECTED: // daca este infectat seteaza infectedFound si daca durata a ajuns la 0 seteaza urmatoarea stare pe immune
                        infectedFound = 1;
                        if(person[index].statusDuration == 0) {
                            person[index].nextStatus = IMMUNE;
                        }
                        break;
                    case IMMUNE: // daca durata a ajuns la 0 seteaza pe susceptible la urmatoarea stare
                        if(person[index].statusDuration == 0) {
                            person[index].nextStatus = SUSCEPTIBLE;
                        }
                        break;
                    case SUSCEPTIBLE: // nu trebuie facut nimic aici
                        break;
                }

                // update reference
                traverser = traverser->next;
            }
            
            if(infectedFound) { // daca o persoana este infectata vom infecta si celelalte persoane susceptibile din aceeasi celula
                traverser = grid[i][j];
                while(traverser != NULL) {
                    if(person[traverser->personIndex].status == SUSCEPTIBLE)
                        person[traverser->personIndex].nextStatus = INFECTED;

                    // update reference
                    traverser = traverser->next;
                }
            }
        }
    }

    return NULL;
}

void computeNextStatusParallel(PersonNode ***grid, Person *person, SimulationData *simulation, Parameters *parameters, pthread_t *threadHandles, int threadCount) {
    for(int thread=0;thread<threadCount;thread++) {
        parameters[thread].rank = thread;
        parameters[thread].threadCount = threadCount;

        parameters[thread].grid = grid;
        parameters[thread].person = person;
        parameters[thread].simulation = simulation;

        pthread_create(&threadHandles[thread], NULL, computeNextStatusParallel_PosixFunc, &parameters[thread]);
    }

    for (int thread=0;thread<threadCount;thread++) {
        pthread_join(threadHandles[thread], NULL);
    }
}

// for version 1
// void computeNextStatus(Person *person, int index, SimulationData *simulation) {
//     switch (person[index].status) {
//         case INFECTED:
//             for (int i=0; i<simulation->numberOfPersons; i++) {
//                 if (i != index) {
//                     if (person[i].status == SUSCEPTIBLE) {
//                         if (person[i].coord.x == person[index].coord.x) {
//                             if (person[i].coord.y == person[index].coord.y) {
//                                 person[i].nextStatus = INFECTED;
//                             }
//                         }
//                     }
//                 }
//             }
//             if (person[index].statusDuration == 0) {
//                 person[index].nextStatus = IMMUNE;
//             }
//             break;
//         case IMMUNE:
//             if (person[index].statusDuration == 0) {
//                 person[index].nextStatus = SUSCEPTIBLE;
//             }
//             break;
//         case SUSCEPTIBLE:
//             break;
//         default:
//     }
// }