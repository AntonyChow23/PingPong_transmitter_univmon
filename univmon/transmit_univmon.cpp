#include <time.h>
#include "mbed.h"
#include "sx1276-hal.h"
#include "univmon.h"

/* print functions */
void print_seed_pair(seedPair* current_pair) {
    debug("seed1 %d seed2 %d\r\n", current_pair->seed1, current_pair->seed2);
}

void print_heap(minHeap* current_heap) {
    int i;
    for (i = 0; i < current_heap->size; i++) {
        debug("heap_elem %d key %d count %d\r\n", i, current_heap->elem[i].key,
              current_heap->elem[i].count);
    }
}

void print_single_sketch_copy(singleSketchCopy* current_copy, int is_mice_layer,
                              uint32_t current_layer_sketch_length) {
    debug("seed_sketch ");
    print_seed_pair(&(current_copy->seed_sketch));
    print_heap(&(current_copy->topKs));
    if (!is_mice_layer) {
        int i, j;
        for (i = 0; i < CS_ROW_NO; i++) {
            debug("seed_pos %d ", i);
            print_seed_pair(&(current_copy->seed_pos[i]));
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            debug("seed_filter %d ", i);
            print_seed_pair(&(current_copy->seed_filter[i]));
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            debug("sketch_counter %d ", i);
            for (j = 0; j < current_layer_sketch_length; j++) {
                debug("%d,", current_copy->sketch_counter[i][j]);
            }
            debug("\r\n");
        }
    }
    debug("\r\n");
}

void print_univmon(univSketch* univ) {
    debug("start printing univmon\r\n");
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);
        if (!(current_layer->is_extra_layer || current_layer->is_mice_layer)) {
            debug("layer %d copy 0\r\n", i);
            print_single_sketch_copy(
                &(current_layer->all_copy[0]), current_layer->is_mice_layer,
                current_layer->current_layer_sketch_length);
        } else {
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                debug("layer %d copy %d\r\n", i, j);
                print_single_sketch_copy(
                    &(current_layer->all_copy[j]), current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length);
            }
        }
    }
}

void print_single_sketch_copy_counter_heap(
    singleSketchCopy* current_copy, int is_mice_layer,
    uint32_t current_layer_sketch_length) {
    print_heap(&(current_copy->topKs));
    if (!is_mice_layer) {
        int i, j;
        for (i = 0; i < CS_ROW_NO; i++) {
            debug("sketch_counter %d ", i);
            for (j = 0; j < current_layer_sketch_length; j++) {
                debug("%d,", current_copy->sketch_counter[i][j]);
            }
            debug("\r\n");
        }
    }
    debug("\r\n");
}

void print_univmon_counter_heap(univSketch* univ) {
    debug("start printing univmon counter and heap\r\n");
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);
        if (!(current_layer->is_extra_layer || current_layer->is_mice_layer)) {
            debug("layer %d copy 0\r\n", i);
            print_single_sketch_copy_counter_heap(
                &(current_layer->all_copy[0]), current_layer->is_mice_layer,
                current_layer->current_layer_sketch_length);
        } else {
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                debug("layer %d copy %d\r\n", i, j);
                print_single_sketch_copy_counter_heap(
                    &(current_layer->all_copy[j]), current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length);
            }
        }
    }
}

void init_univmon_transmit(univTransmit* univ_transmit, univSketch* univ) {
    univ_transmit->layer_num = 0;
    univ_transmit->copy_num = 0;
    univ_transmit->is_heap = 1;
    univ_transmit->heap_elem_num = 0;
    univ_transmit->sketch_counter_row = 0;
    univ_transmit->sketch_counter_column = 0;
    singleSketchLayer* current_layer =
        &(univ->all_layers[univ_transmit->layer_num]);
    univ_transmit->current_layer_sketch_length =
        current_layer->current_layer_sketch_length;
}

int get_int_from_cur_univmon_transmit(univTransmit* univ_transmit,
                                      univSketch* univ) {
    int current_int = 0;
    singleSketchLayer* current_layer =
        &(univ->all_layers[univ_transmit->layer_num]);
    singleSketchCopy* current_copy =
        &(current_layer->all_copy[univ_transmit->copy_num]);
    minHeap current_heap = current_copy->topKs;
    int32_t* current_counter =
        current_copy->sketch_counter[univ_transmit->sketch_counter_row];
    if (univ_transmit->is_heap) {
        if (univ_transmit->heap_elem_num < current_heap.size * 2) {
            int temp_heap_location = (int)univ_transmit->heap_elem_num / (int)2;
            if (univ_transmit->heap_elem_num % 2 == 0) {
                current_int = current_heap.elem[temp_heap_location].key;
            } else {
                current_int = current_heap.elem[temp_heap_location].count;
            }
        } else {
            current_int = 0;
        }
    } else {
        current_int = current_counter[univ_transmit->sketch_counter_column];
    }
    return current_int;
}

int end_of_heap(univTransmit* univ_transmit) {
    if (univ_transmit->is_heap == 1 &&
        univ_transmit->heap_elem_num == TOPK_SIZE * 2 - 1) {
        return 1;
    } else
        return 0;
}

int end_of_one_copy(univTransmit* univ_transmit, univSketch* univ) {
    singleSketchLayer* current_layer =
        &(univ->all_layers[univ_transmit->layer_num]);
    if (current_layer->is_mice_layer) {
        if (end_of_heap(univ_transmit)) {
            return 1;
        } else
            return 0;
    } else {
        if (univ_transmit->is_heap == 0 &&
            univ_transmit->sketch_counter_row == CS_ROW_NO - 1 &&
            univ_transmit->sketch_counter_column ==
                univ_transmit->current_layer_sketch_length - 1) {
            return 1;
        } else
            return 0;
    }
}

int end_of_univmon(univTransmit* univ_transmit, univSketch* univ) {
    if (univ_transmit->layer_num == CS_LVLS - 1 &&
        univ_transmit->copy_num == EXTRA_SKETCH_NUM &&
        end_of_one_copy(univ_transmit, univ)) {
        return 1;
    } else
        return 0;
}

void update_univ_transmit_for_new_copy(univTransmit* univ_transmit,
                                       univSketch* univ) {
    singleSketchLayer* current_layer =
        &(univ->all_layers[univ_transmit->layer_num]);

    /* update layer_num and copy_num */
    if (current_layer->is_extra_layer || current_layer->is_mice_layer) {
        if (univ_transmit->copy_num < EXTRA_SKETCH_NUM) {
            univ_transmit->copy_num += 1;
        } else {
            univ_transmit->layer_num += 1;
            univ_transmit->copy_num = 0;
        }
    } else {
        univ_transmit->layer_num += 1;
        univ_transmit->copy_num = 0;
    }

    /* update all other variables */
    univ_transmit->is_heap = 1;
    univ_transmit->heap_elem_num = 0;
    univ_transmit->sketch_counter_row = 0;
    univ_transmit->sketch_counter_column = 0;
    current_layer = &(univ->all_layers[univ_transmit->layer_num]);
    univ_transmit->current_layer_sketch_length =
        current_layer->current_layer_sketch_length;
}

void update_univ_transmit_in_one_copy(univTransmit* univ_transmit) {
    /* update heap */
    if (univ_transmit->is_heap) {
        if (end_of_heap(univ_transmit)) {
            univ_transmit->is_heap = 0;
        } else {
            univ_transmit->heap_elem_num += 1;
        }
    } else {
        if (univ_transmit->sketch_counter_column ==
            univ_transmit->current_layer_sketch_length - 1) {
            univ_transmit->sketch_counter_row += 1;
            univ_transmit->sketch_counter_column = 0;
        } else {
            univ_transmit->sketch_counter_column += 1;
        }
    }
}

// return 1: end of univmon, else does not reach end of univmon
int update_univmon_transmit(univTransmit* univ_transmit, univSketch* univ) {
    int is_end_of_univmon = end_of_univmon(univ_transmit, univ);
    if (is_end_of_univmon == 0) {
        if (end_of_one_copy(univ_transmit, univ)) {
            update_univ_transmit_for_new_copy(univ_transmit, univ);
        } else {
            update_univ_transmit_in_one_copy(univ_transmit);
        }
    }
    return is_end_of_univmon;
}