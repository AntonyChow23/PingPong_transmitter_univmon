#include "simple_predict.h"

#define MAX_LAYER 7
#define ROW 5

extern int nr_feature[MAX_LAYER];
extern double bias[MAX_LAYER];
extern double w[MAX_LAYER][ROW + 1];