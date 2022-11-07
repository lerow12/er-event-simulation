/* External definitions for emergency department using simlib. */

#include "simlib.h"             /* Required for use of simlib.c. */
#include <string.h>
#include <time.h>

#define EVENT_WALKIN_ARRIVAL          1  /* Event type walkin arrival */
#define EVENT_AMBULANCE_ARRIVAL       2  /* Event type ambulance arrival */
#define EVENT_TRIAGE_PATIENT          3  /* Event type triage patient */
#define EVENT_INITIAL_ASSESMENT       4  /* Event type intial assesment */
#define EVENT_RUN_TESTS               5  /* Event type run tests */
#define EVENT_FOLLOW_UP_ASSESSMENT    6  /* Event type follow-up assessment */
#define EVENT_PATIENT_ADMITTANCE      7  /* Event type patient admittance */
#define LIST_ACTIVE_PATIENTS          1  /* List number for tracking active patients */
#define AMBULANCE_SEVERITY         1.25  /* Ambulance severity multiplier */
#define MAX_NUM_PATIENTS	        100  /* Maximum number of patients in the ER */
#define FILENAME_LIMIT               50  /* Limit filename size */

/* Declare non-simlib global variables. */
int    RANDOM_STREAMS[7];
int    num_doctors, num_exam_rooms, num_nurses, num_labs, num_hospital_rooms, min_patients_simulated; 
float  mean_walkin_interarrival, mean_ambulance_interarrival, mean_triage_duration, 
       mean_initial_assessment_duration, mean_follow_up_assessment_duration, 
       mean_test_duration, mean_severity;
FILE   *outfile;
char   outfile_name[FILENAME_LIMIT];
time_t seconds;

/* Declare non-simlib functions. */
void try_input(float, char*);
void try_output(int);
void init_model(void);
void report(void);

int main(int argc, char** argv)  /* Main function. */
{
    /* Verify correct number of arguments. */
    if (argc != 14)
    {
        printf("USAGE ERROR: Usage %s [mean_walkin_arrival] [mean_ambulance_arrival] [mean_triage_duration]\n\ 
[mean_initial_assessment_duration] [mean_test_duration] [mean_follow_up_assessment_duration]\n\ 
[mean_severity] [num_doctors] [num_nurses] [num_exam_rooms] [num_labs] [num_hospital_rooms]\n\ 
[min_patients_simulated]\n", argv[0]);
        exit(1);
    }

    /* Read and validate input parameters. */
    try_input(mean_walkin_interarrival = atof(argv[1]), argv[1]);
    try_input(mean_ambulance_interarrival = atof(argv[2]), argv[2]);
    try_input(mean_triage_duration = atof(argv[3]), argv[3]);
    try_input(mean_initial_assessment_duration = atof(argv[4]), argv[4]);
    try_input(mean_test_duration = atof(argv[5]), argv[5]);
    try_input(mean_follow_up_assessment_duration = atof(argv[6]), argv[6]);
    try_input(mean_severity = atof(argv[7]), argv[7]);
    try_input((float)(num_doctors = atoi(argv[8])), argv[8]);
    try_input((float)(num_nurses = atoi(argv[9])), argv[9]);
    try_input((float)(num_exam_rooms = atoi(argv[10])), argv[10]);
    try_input((float)(num_labs = atoi(argv[11])), argv[11]);
    try_input((float)(num_hospital_rooms = atoi(argv[12])), argv[12]);
    try_input((float)(min_patients_simulated = atoi(argv[13])), argv[13]);

    /* Calculate mean walk-in interarrival and mean ambulance interarrival time */
    mean_walkin_interarrival = 1.0 / mean_walkin_interarrival;
    mean_ambulance_interarrival = 1.0 / mean_ambulance_interarrival;

    /* Verify that outfile_name is within the FILENAME_LIMIT */
    if (strlen(argv[1]) + strlen(argv[2]) + strlen(argv[3]) + 10
        >= FILENAME_LIMIT)
    {
        printf("FILENAME ERROR: Filename Too Long\n");
        exit(3);
    }
    strcpy(outfile_name, "out/");
    strcat(outfile_name, argv[1]);
    strcat(outfile_name, "_");
    strcat(outfile_name, argv[2]);
    strcat(outfile_name, "_");
    strcat(outfile_name, argv[3]);
    strcat(outfile_name, ".out");

    /* Open Output file. */
    outfile = fopen(outfile_name, "w");

    /* Verify the output file has been sucessfully opened. */
    if (outfile == NULL) {
        printf("FILE ERROR: Output File \"%s\" Cannot Be Opened\n", outfile_name);
        exit(4);
    }

    /* Write report heading and input parameters. */
    try_output(fprintf(outfile, "               Single base station using simlib\n"));
    try_output(fprintf(outfile, "--------------------------------------------------------------\n\n"));
    try_output(fprintf(outfile, "[CONSTANTS]\n\n"));
    try_output(fprintf(outfile, "Max number of channels:%20d\n\n\n", MAX_CHANNELS));
    try_output(fprintf(outfile, "[INPUT PARAMETERS]\n\n"));
    try_output(fprintf(outfile, "Mean call arrival time:%20.3f calls per second\n\n",
            1.0/mean_call_interarrival));
    try_output(fprintf(outfile, "Mean call handoff arrival time:%12.3f calls per second\n\n", 
            1.0/mean_handoff_interarrival));
    try_output(fprintf(outfile, "Mean service rate per channel:%13.3f calls per second\n\n", 
            1.0/mean_call_duration));
    try_output(fprintf(outfile, "Minimum simulation duration:%15.3d seconds\n\n", sim_time_duration));
    try_output(fprintf(outfile, "Mean traffic load:%25.3f\n\n\n", 
            (1.0/mean_call_interarrival + 1.0/mean_handoff_interarrival)/(MAX_CHANNELS*(1.0/mean_call_duration))));

    /* Initialize simlib */
    init_simlib();

    /* Set maxatr = max(maximum number of attributes per record, 4) */
    maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

    /* Initialize the model. */
    init_model();

    /* Run the simulation while more calls are still needed. */
    while (sim_time < sim_time_duration) {

        /* Determine the next event. */
        timing();

        /* Invoke the appropriate event function. */
        switch (next_event_type) {
            case EVENT_START_CALL:
                if (num_open_channels > 0) {
                    /* Add dummy struct to active channel list */
                    list_file(FIRST, LIST_ACTIVE_CHANNELS);
                    total_calls_connected++;
                    num_open_channels--;
                    /* Schedule when the call will end */
                    event_schedule(sim_time + expon(mean_call_duration, STREAM_CALL_DURATION),
                                   EVENT_END_CALL);
                } else {
                    total_calls_rejected++;
                }
                /* Schedule next new call */
                event_schedule(sim_time + expon(mean_call_interarrival, STREAM_CALL_INTERARRIVAL),
                               EVENT_START_CALL);
                break;
            case EVENT_HANDOFF_CALL:
                if (num_open_channels > 0) {
                    /* Add dummy struct to active channel list */
                    list_file(FIRST, LIST_ACTIVE_CHANNELS);
                    total_handoffs_connected++;
                    num_open_channels--;
                    /* Schedule when the call will end */
                    event_schedule(sim_time + expon(mean_call_duration, STREAM_CALL_DURATION),
                                   EVENT_END_CALL);
                } else {
                    total_handoffs_rejected++;
                }
                /* Schedule next handoff call */
                event_schedule(sim_time + expon(mean_handoff_interarrival, STREAM_HANDOFF_INTERARRIVAL),
                               EVENT_HANDOFF_CALL);
                break;
            case EVENT_END_CALL:
                /* Remove dummy struct from active channel list */
                list_remove(FIRST, LIST_ACTIVE_CHANNELS);
                num_open_channels++;
                break;
        }
    }

    /* Invoke the report generator and end the simulation. */
    report();

    /* Close file and verify that is is successful */
    if (fclose(outfile) != 0) {
        printf("FILE ERROR: Output File \"%s\" Cannot Be Closed\n", outfile_name);
        exit(4);
    }

    return 0;
}


void init_model(void)  /* Initialization function. */
{
    /* Initialize non-simlib variables */
    num_open_channels = MAX_CHANNELS;
    total_calls_connected = 0;
    total_calls_rejected = 0;
    total_handoffs_connected = 0;
    total_handoffs_rejected = 0;

    /* Initialize random number streams */
    seconds = time(NULL);
    STREAM_CALL_INTERARRIVAL = 1 * seconds % 60;
    STREAM_HANDOFF_INTERARRIVAL = 2 * seconds % 60;
    STREAM_CALL_DURATION = 3 * seconds % 60;
    
    /* Schedule first new call and first handoff call */
    event_schedule(sim_time + expon(mean_call_interarrival, STREAM_CALL_INTERARRIVAL),
                   EVENT_START_CALL);
    event_schedule(sim_time + expon(mean_handoff_interarrival, STREAM_HANDOFF_INTERARRIVAL),
                   EVENT_HANDOFF_CALL);
}


void report(void)  /* Report generator function. */
{
    /* Get and write out estimates of desired measures of performance. */
    try_output(fprintf(outfile, "[PERFORMANCE METRICS]\n"));
    try_output(fprintf(outfile, "\nBase Station Channel Utilization:%10.1f%%\n", 
               100.0 * filest(LIST_ACTIVE_CHANNELS) / MAX_CHANNELS));
    try_output(fprintf(outfile, "\n(New) Call Block Probability:%14.1f%%\n", 
               100.0 * total_calls_rejected / (total_calls_connected + total_calls_rejected)));
    try_output(fprintf(outfile, "\nHandoff Dropping Probability:%14.1f%%\n\n", 
               100.0 * total_handoffs_rejected / (total_handoffs_connected + total_handoffs_rejected)));
    try_output(fprintf(outfile, "\n[TERMINATION METRICS]\n"));
    try_output(fprintf(outfile, "\nTime simulation ended:%21.3f seconds\n", sim_time));
    try_output(fprintf(outfile, "\nTotal calls simulated:%21d calls\n", 
               total_calls_connected + total_calls_rejected + total_handoffs_connected + total_handoffs_rejected));
}

void try_input(float input, char* input_str) /* Validate input or exit */
{
    if (input == 0)
    {
        printf("INPUT ERROR: \"%s\" Is Not A Valid Input\n", input_str);
        exit(2);
    }
}

void try_output(int status) /* Validate output or exit */
{
    if (status < 0)
    {
        printf("FILE ERROR: Output File \"%s\" Cannot Be Written To\n", outfile_name);
        if (fclose(outfile) != 0)
        {
            printf("FILE ERROR: Output File \"%s\" Cannot Be Closed\n", outfile_name);
        }
        exit(5);
    }
}
