#include "sketch_config.h"

/* constants.h used for solar energy*/

/* define univmon sketch mode */
#define ONLY_UPDATE_LAST_LAYER_SKETCH
#define USE_SVM_MODEL
#define DEBUG_MODE  // uncomment this line to enable debugging
// #define SMALL_COUNTER

/*
 * If DEBUG_PRINT is defined, enable printing on dbg_printf.
 * If DEBUG_PRINT is not defined, dbg_printf is not enabled
 */
#ifdef DEBUG_MODE
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_fprintf(...) fprintf(__VA_ARGS__)
#else
/* When debugging is disnabled, no code gets generated for these */
#define dbg_fprintf(...)
#endif

/* define data types */
#ifdef SMALL_COUNTER
typedef int16_t my_int;
#else
typedef int32_t my_int;
#endif

typedef uint32_t my_key;

/* define input data size */
// #define INPUT_NUM 430485
// #define INPUT_NUM 86097
/* define input data maximum */
//#define INPUT_MAX 248875
/* define input data path */
//#define INPUT_FILE "../test_input/data_2500000"
/* define sketch parameters */
// #define CS_LVLS 8
// #define CS_ROW_NO 5
// #define CS_COL_NO 492
/* constant to decide whether to +1 or -1 */
#define INCR_CONST 2
/* constant to decide whether to sample or not */
#define ZERO_ONE_CONST 2
#define PRIME 39916801
/* define top K size */
// #define TOPK_SIZE 50
/* define SAMPLE_RATE: the possibility that a input value could enter next layer
 * of sketch */
// #define SAMPLE_RATE 5
/* sketch length reduce rate */
// #define SKETCH_REDUCE_RATE 1.3
/* topK heap reduce rate */
#define HEAP_REDUCE_RATE 1