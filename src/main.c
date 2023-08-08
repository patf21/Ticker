#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "store.h"
#include "ticker.h"
int main(int argc, char *argv[]) {
    if(ticker()){
	return EXIT_FAILURE;
    }
    else
	return EXIT_SUCCESS;
}
