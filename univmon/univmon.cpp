#include "univmon.h"

/* initialize functions */
void init_seed_pair(seedPair* current_pair) {
    current_pair->seed1 = rand() % PRIME;
    current_pair->seed2 = rand() % PRIME;
}

void init_single_sketch_copy(singleSketchCopy* current_copy, int is_mice_layer,
                             uint32_t current_layer_sketch_length) {
    int i;
    init_seed_pair(&(current_copy->seed_sketch));
    initMinHeap(&(current_copy->topKs), TOPK_SIZE);
    if (!is_mice_layer) {
        for (i = 0; i < CS_ROW_NO; i++) {
            init_seed_pair(&(current_copy->seed_pos[i]));
            init_seed_pair(&(current_copy->seed_filter[i]));
            current_copy->sketch_counter[i] =
                (int32_t *)calloc(current_layer_sketch_length, sizeof(int32_t));
        }
    }
}

void init_univmon(univSketch* univ) {
//    srand((unsigned)time(NULL));
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);

        if (i < ELEPHANT_LAYER) {
            current_layer->is_extra_layer = 0;
            current_layer->is_mice_layer = 0;
        } else if (i >= ELEPHANT_LAYER && i < ELEPHANT_LAYER + EXTRA_LAYER) {
            current_layer->is_extra_layer = 1;
            current_layer->is_mice_layer = 0;
        } else {
            current_layer->is_extra_layer = 0;
            current_layer->is_mice_layer = 1;
        }

        if (!current_layer->is_mice_layer) {
            current_layer->current_svm_model =
                get_model_simple(nr_feature[i], w[i], bias[i]);
            current_layer->current_layer_sketch_length = CS_COL_NO;
//                (uint32_t)(CS_COL_NO / pow(SKETCH_REDUCE_RATE, i));
        } else {
            current_layer->current_layer_sketch_length = 0;
        }

        if (current_layer->is_extra_layer || current_layer->is_mice_layer) {
            current_layer->all_copy =
                (singleSketchCopy *)calloc(1 + EXTRA_SKETCH_NUM, sizeof(singleSketchCopy));
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                init_single_sketch_copy(
                    &(current_layer->all_copy[j]), current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length);
            }
        } else {
            current_layer->all_copy = (singleSketchCopy *)calloc(1, sizeof(singleSketchCopy));
            init_single_sketch_copy(&(current_layer->all_copy[0]),
                                    current_layer->is_mice_layer,
                                    current_layer->current_layer_sketch_length);
        }
    }
}

/* print functions */
void print_seed_pair(seedPair* current_pair, FILE* fp) {
    fprintf(fp, "seed1 %d seed2 %d\n", current_pair->seed1,
            current_pair->seed2);
}

void print_heap(minHeap* current_heap, FILE* fp) {
    int i;
    for (i = 0; i < current_heap->size; i++) {
        fprintf(fp, "heap_elem %d key %d count %d\n", i,
                current_heap->elem[i].key, current_heap->elem[i].count);
    }
}

void swap_int(int* a, int* b) {
    int temp = *b;
    *b = *a;
    *a = temp;
}

void print_sorted_heap(minHeap* current_heap, FILE* fp) {
    int freq[TOPK_SIZE];
    int item[TOPK_SIZE];
    int k;
    for (k = 0; k < current_heap->size; k++) {
        item[k] = current_heap->elem[k].key;
        freq[k] = current_heap->elem[k].count;
    }

    /* do bubble sort */
    int i = 0;
    int j;
    while (i < current_heap->size) {
        j = 0;
        while (j < i) {
            if (freq[j] < freq[i]) {
                swap_int(&freq[j], &freq[i]);
                swap_int(&item[j], &item[i]);
            }
            j++;
        }
        i++;
    }

    for (i = 0; i < current_heap->size; i++) {
        fprintf(fp, "key %d count %d\n", item[i], freq[i]);
    }
}

void print_single_sketch_copy(singleSketchCopy* current_copy, FILE* fp,
                              int is_mice_layer,
                              uint32_t current_layer_sketch_length) {
    fprintf(fp, "seed_sketch ");
    print_seed_pair(&(current_copy->seed_sketch), fp);
    print_heap(&(current_copy->topKs), fp);
    // print_sorted_heap(&(current_copy->topKs), fp);
    if (!is_mice_layer) {
        int i, j;
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "seed_pos %d ", i);
            print_seed_pair(&(current_copy->seed_pos[i]), fp);
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "seed_filter %d ", i);
            print_seed_pair(&(current_copy->seed_filter[i]), fp);
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "sketch_counter %d ", i);
            for (j = 0; j < current_layer_sketch_length; j++) {
                fprintf(fp, "%d,", current_copy->sketch_counter[i][j]);
            }
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "\n");
}

/* update functions */
uint32_t simple_hash(seedPair current_pair, uint32_t key) {
//    return ((current_pair.seed1 * key + current_pair.seed2) % PRIME);
//    return XXH32(&key, sizeof(key), current_pair.seed1)%PRIME;
    return XXH32(&key, sizeof(key), current_pair.seed1);
}

int insert_sketch_counter_single_line(singleSketchCopy* current_copy,
                                      uint32_t current_layer_sketch_length,
                                      int line_num, uint32_t key) {
    uint32_t pos = simple_hash(current_copy->seed_pos[line_num], key) %
                   current_layer_sketch_length;
    int f2_filter = simple_hash(current_copy->seed_filter[line_num], key) % 2;
    current_copy->sketch_counter[line_num][pos] += (1 - 2 * f2_filter);

    int current_value;
    if ((1 - 2 * f2_filter) > 0) {
        current_value = current_copy->sketch_counter[line_num][pos];
    } else {
        current_value = -current_copy->sketch_counter[line_num][pos];
    }
    return current_value;
}

void quick_sort(int* values, int first_index, int last_index) {
    // declaring index variables
    int pivotIndex, temp, index_a, index_b;

    if (first_index < last_index) {
        // assigning first element index as pivot element
        pivotIndex = first_index;
        index_a = first_index;
        index_b = last_index;

        // Sorting in Ascending order with quick sort
        while (index_a < index_b) {
            while (values[index_a] <= values[pivotIndex] &&
                   index_a < last_index) {
                index_a++;
            }
            while (values[index_b] > values[pivotIndex]) {
                index_b--;
            }

            if (index_a < index_b) {
                // Swapping operation
                temp = values[index_a];
                values[index_a] = values[index_b];
                values[index_b] = temp;
            }
        }

        // At the end of first iteration, swap pivot element with index_b
        // element
        temp = values[pivotIndex];
        values[pivotIndex] = values[index_b];
        values[index_b] = temp;

        // Recursive call for quick sort, with partitioning
        quick_sort(values, first_index, index_b - 1);
        quick_sort(values, index_b + 1, last_index);
    }
}

int svm_predict_median(int* values, struct model* current_svm_model) {
    double normalize_counters[CS_ROW_NO];
    double median_counter = values[CS_ROW_NO / 2];

    if (median_counter == 0) median_counter = 1;

    for (int i = 0; i < CS_ROW_NO; i++)
        normalize_counters[i] = (double)values[i] / median_counter;

    int svm = do_predict_simple(normalize_counters, current_svm_model);
    return svm;
}

int find_median_value(int* values, struct model* current_svm_model) {
    int median;
    quick_sort(values, 0, CS_ROW_NO - 1);
    if (CS_ROW_NO % 2 == 0) {
        median = (values[CS_ROW_NO / 2] + values[CS_ROW_NO / 2 - 1]) / 2;
    } else {
        median = values[CS_ROW_NO / 2];
    }
    median = (median <= 1) ? 1 : median;

    if (USE_MODEL) {
        int svm = svm_predict_median(values, current_svm_model);
        if (svm) {
            median = 1;
        }
    }

    return median;
}

int adjust_counter_for_single_elephant_copy(
    singleSketchCopy* current_copy, uint32_t current_layer_sketch_length,
    struct model* current_svm_model, uint32_t key) {
    int values[CS_ROW_NO];
    int line_num;
    for (line_num = 0; line_num < CS_ROW_NO; line_num++) {
        values[line_num] = insert_sketch_counter_single_line(
            current_copy, current_layer_sketch_length, line_num, key);
    }
    int median = find_median_value(values, current_svm_model);
    return median;
}

void adjust_heap_for_single_elephant_copy(singleSketchCopy* current_copy,
                                          int median, uint32_t key) {
    minHeap* current_heap = &(current_copy->topKs);
    int found = find(current_heap, key);
    if (found != (-1)) {
        (*current_heap).elem[found].count++;
        // (*current_heap).elem[found].count = median;
        for (int i = ((*current_heap).size - 1) / 2; i >= 0; i--) {
            heapify(current_heap, i);
        }
    } else {
        if ((*current_heap).size < TOPK_SIZE) {
            insertNode(current_heap, key, median);
        } else if (median > (*current_heap).elem[0].count) {
            deleteNode(current_heap);
            insertNode(current_heap, key, median);
        }
    }
}

int adjust_heap_for_single_mice_copy(singleSketchCopy* current_copy,
                                     uint32_t key) {
    minHeap* current_heap = &(current_copy->topKs);
    int count = 0;
    int found = find(current_heap, key);
    if (found != (-1)) {
        (*current_heap).elem[found].count++;
        count = (*current_heap).elem[found].count;
        for (int i = ((*current_heap).size - 1) / 2; i >= 0; i--) {
            heapify(current_heap, i);
        }
    } else if ((*current_heap).size < TOPK_SIZE) {
        insertNode(current_heap, key, 1);
        count = 1;
    }
    return count;
}

//void update_bottom_up(univSketch* univ, int bottom_layer_num, int up_layer_num,
//                      int copy_num, uint32_t key) {
//    singleSketchLayer* current_layer;
//    singleSketchCopy* current_copy;
//
//    /* update bottom layer: counter and heap */
//    current_layer = &(univ->all_layers[bottom_layer_num]);
//    current_copy = &(current_layer->all_copy[copy_num]);
//    int median;
//    if (!(current_layer->is_mice_layer)) {
//        median = adjust_counter_for_single_elephant_copy(
//            current_copy, current_layer->current_layer_sketch_length,
//            current_layer->current_svm_model, key);
//        adjust_heap_for_single_elephant_copy(current_copy, median, key);
//    } else {
//        median = adjust_heap_for_single_mice_copy(current_copy, key);
//    }
//
//    /* update all layers except bottom layer: heap */
//    int i;
//    for (i = up_layer_num; i < bottom_layer_num; i++) {
//        current_layer = &(univ->all_layers[i]);
//        current_copy = &(current_layer->all_copy[copy_num]);
//        adjust_heap_for_single_elephant_copy(current_copy, median, key);
//    }
//}

void update_bottom_up(univSketch* univ, int bottom_layer_num, int up_layer_num,
                      int copy_num, uint32_t key) {
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;

    /* update bottom layer: counter and heap */
    current_layer = &(univ->all_layers[bottom_layer_num]);
    current_copy = &(current_layer->all_copy[copy_num]);
    int median;
    if (!(current_layer->is_mice_layer)) {
        median = adjust_counter_for_single_elephant_copy(
            current_copy, current_layer->current_layer_sketch_length,
            current_layer->current_svm_model, key);
        adjust_heap_for_single_elephant_copy(current_copy, median, key);
    } else {
        median = adjust_heap_for_single_mice_copy(current_copy, key);
    }

    /* update all layers except bottom layer: heap */
    int i;
    for (i = up_layer_num; i < bottom_layer_num; i++) {
        current_layer = &(univ->all_layers[i]);
        current_copy = &(current_layer->all_copy[copy_num]);
        median = adjust_counter_for_single_elephant_copy(
            current_copy, current_layer->current_layer_sketch_length,
            current_layer->current_svm_model, key);
        adjust_heap_for_single_elephant_copy(current_copy, median, key);
    }
}

int could_enter_current_copy(singleSketchCopy* current_copy, uint32_t key) {
    if (simple_hash(current_copy->seed_sketch, key) % SAMPLE_RATE == 0) {
        return 1;
    } else {
        return 0;
    }
}

int find_bottom_layer_num(univSketch* univ, int start_num, int end_num,
                          int copy_num, uint32_t key) {
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;
    int bottom_layer_num = start_num;
    int i;
    for (i = start_num + 1; i < end_num; i++) {
        current_layer = &(univ->all_layers[i]);
        current_copy = &(current_layer->all_copy[copy_num]);
        if (could_enter_current_copy(current_copy, key)) {
            bottom_layer_num = i;
        } else {
            break;
        }
    }
    return bottom_layer_num;
}

void univmon_processing(univSketch* univ, uint32_t key) {
    int start_num, end_num, copy_num, bottom_layer_num, up_layer_num;
    /* update copy 0 */
    start_num = 0;
    end_num = CS_LVLS;
    copy_num = 0;
    bottom_layer_num =
        find_bottom_layer_num(univ, start_num, end_num, copy_num, key);
    up_layer_num = 0;
    update_bottom_up(univ, bottom_layer_num, up_layer_num, copy_num, key);

    /* update other copies */
    if (bottom_layer_num >= ELEPHANT_LAYER - 1) {
        start_num = ELEPHANT_LAYER - 1;
        end_num = CS_LVLS;
        up_layer_num = ELEPHANT_LAYER;
        int i;
        for (i = 1; i < 1 + EXTRA_SKETCH_NUM; i++) {
            copy_num = i;
            bottom_layer_num =
                find_bottom_layer_num(univ, start_num, end_num, copy_num, key);
            if (bottom_layer_num >= up_layer_num) {
                update_bottom_up(univ, bottom_layer_num, up_layer_num, copy_num,
                                 key);
            }
        }
    }
}