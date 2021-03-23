#ifndef _SIMPLE_PREDICT_H_
#define _SIMPLE_PREDICT_H_

/*
    The following is text in a svm model file.
    --------------------------------
    solver_type L2R_L2LOSS_SVC_DUAL
    nr_class 2
    label 0 1
    nr_feature 5
    bias -1
    w
    -0.021212504260164862
    2.2141534237569092
    -1.0420274666960228
    -0.4367593587027791
    -0.018954575697712115
    --------------------------------
    Usage
    -----
    int nr_feature = 5;
    double w[] = {-0.021212504260164862,
                  2.2141534237569092,
                  -1.0420274666960228,
                  -0.4367593587027791,
                  -0.018954575697712115};
    double bias = -1;
    struct model* model_ = get_model_simple(nr_feature, w, bias);
*/

/* from now on, the bias term will always be 1, and there will be 6 numbers in w
 * term. */

struct model {
    int nr_class; /* number of classes */
    int nr_feature;
    int *label; /* label of each class */
    double *w;
    double bias;
};

int do_predict_simple(double *counters, struct model *model_);
struct model *get_model_simple(int nr_feature, double *w, double bias);
void release_model_simple(struct model *model_);

#endif