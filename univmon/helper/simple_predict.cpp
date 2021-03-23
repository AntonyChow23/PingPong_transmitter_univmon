#include "simple_predict.h"
#include <stdio.h>
#include <stdlib.h>

#define Malloc(type, n) (type *)malloc((n) * sizeof(type))

int do_predict_simple(double *counters, struct model *model_) {
    int w_size = model_->nr_feature;  // 5
    double sum = 0;
    for (int i = 0; i < w_size; i++) {
        sum += model_->w[i] * counters[i];
    }
    sum += model_->w[w_size] * model_->bias;

    return (sum > 0) ? model_->label[0] : model_->label[1];
}

struct model *get_model_simple(int nr_feature, double *w, double bias) {
    struct model *model_ = Malloc(struct model, 1);
    model_->nr_class = 2;
    model_->nr_feature = nr_feature;
    model_->bias = bias;

    int n;
    if (model_->bias >= 0)
        n = model_->nr_feature + 1;
    else
        n = model_->nr_feature;

    model_->label = Malloc(int, model_->nr_class);
    for (int i = 0; i < model_->nr_class; i++) model_->label[i] = i;

    int w_size = n;
    model_->w = Malloc(double, w_size);
    for (int i = 0; i < w_size; i++) model_->w[i] = w[i];

    return model_;
}

void release_model_simple(struct model *model_) {
    if (model_) {
        if (model_->w) {
            free(model_->w);
        }
        if (model_->label) {
            free(model_->label);
        }
        free(model_);
    }
}