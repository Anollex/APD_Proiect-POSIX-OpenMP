@ECHO OFF
ECHO Starting simulation batch run...
ECHO.

REM --- Define Test Variables ---
SET SIM_TIMES=50 100 150 200 500
SET INPUT_FILES=epidemics10K.txt epidemics20K.txt epidemics50K.txt epidemics100K.txt epidemics500K.txt
SET THREAD_COUNTS=4 8 12 16
SET LOG_FILE=simulation_results.txt
REM --- End of Variables ---

REM Delete the old log file if it exists, so this run is fresh.
IF EXIST "%LOG_FILE%" (
    ECHO Deleting old log file: %LOG_FILE%
    DEL "%LOG_FILE%"
)

ECHO All console output from prog.exe will be redirected to %LOG_FILE%
ECHO You will still see progress updates in this window.
ECHO.

REM Loop through all simulation times
FOR %%t IN (%SIM_TIMES%) DO (

    REM Loop through all input files
    FOR %%f IN (%INPUT_FILES%) DO (
    
        REM Loop through all thread counts
        FOR %%c IN (%THREAD_COUNTS%) DO (
        
            REM Display the command in the console (so you see progress)
            ECHO Running: prog.exe %%t %%f %%c
            
            REM Write a header for this run into the log file
            @REM ECHO -------------------------------------------------- >> %LOG_FILE%
            @REM ECHO Running: prog.exe %%t %%f %%c >> %LOG_FILE%
            @REM ECHO -------------------------------------------------- >> %LOG_FILE%
            
            REM Execute the command and append its stdout (>>) to the log file
            prog.exe %%t %%f %%c >> %LOG_FILE%
            
            REM Add a blank line in the log file for readability
            ECHO. >> %LOG_FILE%
        )
    )
)

ECHO All simulations complete. Results are in %LOG_FILE%
PAUSE