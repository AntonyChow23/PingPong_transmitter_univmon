#include "svm_config.h"

int nr_feature[MAX_LAYER] = {5, 5, 5, 5, 5, 5, 5};
double bias[MAX_LAYER] = {1, 1, 1, 1, 1, 1, 1};
double w[MAX_LAYER][ROW + 1] = {
    {0.58393114813776137, 2.0792535108194361, -0.03509074194884141,
     -0.85331501200572557, -0.13223224902859884, 0.58393114813776137},
    {-0.012415499306666865, 0.97464720448481845, 0.19507606825175475,
     -0.38064642125830322, -0.047417646791436181, 0.97464720448481845},
    {-0.020011455141180058, 2.2191210321385255, -0.760325654483334,
     -0.033880361955418221, -0.022827382843397827, -0.760325654483334},
    {-0.0082606351028714567, 1.1364169555103543, -0.25161340922370118,
     0.017231863487598899, -0.00024982097245168687, 0.017231863487598899},
    {-0.0013018533168094094, 0.76422759704230914, 0.20222483781709294,
     -0.071648857367047125, -0.0068123144473930157, -0.0068123144473930157},
    {-0.01686610497767058, 0.070965170650002241, -0.039172479791090113,
     -0.013406721533461771, -0.0058253737111358074, 0.99701649315270646},
    {0.0010032149591085936, 0.001301336015367518, 0.001301336015367518,
     0.001301336015367518, 0.0013054372007254573, 0.99993562108491141}};