#include "utilities.h"

#include "script.h"

int 
main(int argc, char const *argv[])
{
    script_run_file(argv[1]);
    return 0;
}
