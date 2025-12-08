#ifndef COMMANDS_H
#define COMMANDS_H

#include "elkirc.h"
#include "network.h"

void process_command(int sock, const char *input);

#endif
