#include "mnist.h"
#include "tflm_c.h"
#include "network_tflite_data.h"

#define TENSOR_ARENA_SIZE   (80 * 1024)
#define TENSOR_INPUT_SIZE   (28 * 28)
#define TENSOR_OUTPUT_SIZE  (10 * 1)

static int inference_btn_clicked;

void mnist_inference_task(void const * argument) {
  // initialize MNIST model
  uint32_t* hdl;
  static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

  if (tflm_c_create(
      g_tflm_network_model_data,
      tensor_arena,
      TENSOR_ARENA_SIZE,
      &hdl
  )) {
    return;
  }

  // create input_tensor, output_tensor
  struct tflm_c_tensor_info input_tensor;
  struct tflm_c_tensor_info output_tensor;

  volatile static uint8_t handwriting_data[TENSOR_INPUT_SIZE];

  for (;;) {
    // wait for inference button click event
    if (inference_btn_clicked) {
      // get handwriting data from display
      get_handwriting_data(handwriting_data);

      // prepare input tensor
      tflm_c_input(hdl, 0, &input_tensor);
      float* input = (float*)input_tensor.data;
      for (int i = 0; i < TENSOR_INPUT_SIZE; i++) {
        input[i] = handwriting_data[i] / 255.0f;
      }

      // inference
      tflm_c_invoke(hdl);

      // get result
      tflm_c_output(hdl, 0, &output_tensor);
      float* output = (float*)output_tensor.data;

      // show result to display
      print_inference_result(output);
    }
  }
}

void mnist_display_task(void const * argument) {

}
