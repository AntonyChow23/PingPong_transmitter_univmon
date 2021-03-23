#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "helper/constants.h"
#include "helper/heap.h"
#include "helper/svm_config.h"
#include "input.h"
#include "helper/xxhash.h"

typedef struct seedPair {
    uint32_t seed1;
    uint32_t seed2;
} seedPair;

typedef struct singleSketchCopy {
    seedPair seed_sketch;
    minHeap topKs;
    seedPair seed_pos[CS_ROW_NO];
    seedPair seed_filter[CS_ROW_NO];
    int32_t* sketch_counter[CS_ROW_NO];
} singleSketchCopy;

typedef struct singleSketchLayer {
    int is_extra_layer;
    int is_mice_layer;
    uint32_t current_layer_sketch_length;
    struct model* current_svm_model;
    singleSketchCopy* all_copy;
} singleSketchLayer;

typedef struct univSketch {
    singleSketchLayer all_layers[CS_LVLS];
} univSketch;

typedef struct univTransmit {
    int layer_num;
    int copy_num;
    int is_heap;  // if 1, then is heap; if 0, then is sketch counter
    int heap_elem_num;
    int sketch_counter_row;
    int sketch_counter_column;
    int current_layer_sketch_length;
} univTransmit;

void init_univmon(univSketch* univ);
void univmon_processing(univSketch* univ, uint32_t key);
void print_univmon(univSketch* univ);
void print_univmon_counter_heap(univSketch* univ);
void init_univmon_transmit(univTransmit* univ_transmit, univSketch* univ);
int get_int_from_cur_univmon_transmit(univTransmit* univ_transmit,
                                      univSketch* univ);
int update_univmon_transmit(univTransmit* univ_transmit, univSketch* univ);