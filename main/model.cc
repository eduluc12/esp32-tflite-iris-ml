#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include "model.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
constexpr int kTensorArenaSize = 16 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

extern "C" void iris_model_load(const void* buffer_model){
    model = tflite::GetModel(buffer_model);
    static tflite::MicroMutableOpResolver<2> resolver;
    if(resolver.AddFullyConnected() != kTfLiteOk){
        printf("error AddFullyConnected\n");
        return;
    }
    if(resolver.AddSoftmax() != kTfLiteOk){
        printf("error AddSoftmax\n");
        return;
    }
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("error AllocateTensors\n");
        return;
    }
}

extern "C" iris_model_output iris_model_calculate(float* values){
    iris_model_output my_model = {
        .setosa = 0,
        .versicolor = 0,
        .virginica = 0
    };

    std::copy_n(values, 4, interpreter->input(0)->data.f);

    if (interpreter->Invoke() != kTfLiteOk) {
        printf("error Invoke\n");
        return my_model;
    }

    std::copy_n(interpreter->output(0)->data.f, 3, reinterpret_cast<float*>(&my_model));

    return my_model;
}