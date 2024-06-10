#ifndef model__h
#define model__h

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    float setosa;
    float versicolor;
    float virginica;
} iris_model_output;

void iris_model_load(const void*);

iris_model_output iris_model_calculate(float* values);

#ifdef __cplusplus
}
#endif

#endif