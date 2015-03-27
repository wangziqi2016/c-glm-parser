
#include "glm_parser.h"

static FILE *log_fp;

void setup_logging(string filename)
{
    // This file pointer is not closed before program termination
    // Must flush data using '\n' or fflush() manually to prevent
    // buffered data loss
    log_fp = fopen(filename.c_str(), "a");
    fprintf(log_fp, "==============Start running===================\n");
    
    return;
}

// Call this to write a new record of local time into log buffer
// No newline character so that subsequent function calls could 
// append to this timestamp to record more meaningful actions
static void logging_time()
{
    time_t timer = time(NULL);
    struct tm *lt = localtime(&timer);
    string atime(asctime(lt));
    
    fprintf(log_fp, "%s ", atime.c_str());
    
    return;   
}

void logging_info()
{
    
}

void logging_debug()
{
    
}
