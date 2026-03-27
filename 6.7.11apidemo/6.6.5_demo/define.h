#pragma once
#include <stdio.h>

extern FILE *logfile;
#define LOG(...) do { fprintf(logfile, __VA_ARGS__); printf(__VA_ARGS__); fflush(logfile); } while (0);
