#include <stdio.h>
#include <stdarg.h>
#include "cmsis_os.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"

#include "tflm_c.h"
#include "network_tflite_data.h"
#define INPUT_DATA_WIDTH  28
#define INPUT_DATA_HEIGTH 28

#define DRAW_AREA_X       10
#define DRAW_AREA_WIDTH   224
#define DRAW_AREA_Y       10
#define DRAW_AREA_HEIGHT  224

#define BTN_AREA_X        250
#define BTN_AREA_WIDTH    INPUT_DATA_WIDTH
#define BTN_AREA_Y        10
#define BTN_AREA_HEIGHT   INPUT_DATA_HEIGTH

volatile int displayInitialized = 0;
volatile int eventButtonTouched = 0;

#define LOG_BUFFER_SIZE 32
void printlog(const char* format, ...) {
  if (!displayInitialized) {
    return;
  }
  
  va_list args;
  va_start(args, format);

  uint8_t buffer[LOG_BUFFER_SIZE];
  vsprintf((char *)buffer, format, args);

  int i = 0;
  while (buffer[i] != '\0' && i < LOG_BUFFER_SIZE) i++;
  while (i < LOG_BUFFER_SIZE) {
    buffer[i] = ' ';
    i++;
  }
  buffer[LOG_BUFFER_SIZE - 1] = '\0';

  BSP_LCD_DisplayStringAtLine(10, buffer);

  va_end(args);
}

int tflm_io_write(const void *buff, uint16_t count) {
  return 0;
}

void TchDspTask(void const * argument) {
  // LCD Initialize
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_SelectLayer(0);
  BSP_LCD_DisplayOn();
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  displayInitialized = 1;

  // Touch Screen Initialize
  uint32_t screenSizeX = BSP_LCD_GetXSize();
  uint32_t screenSizeY = BSP_LCD_GetYSize();
  BSP_TS_Init(screenSizeX, screenSizeY);

  // Draw Areas
  BSP_LCD_DrawRect(
    DRAW_AREA_X - 1, DRAW_AREA_Y - 1,
    DRAW_AREA_WIDTH + 1, DRAW_AREA_HEIGHT + 1
  );
  BSP_LCD_DrawRect(
    BTN_AREA_X - 1, BTN_AREA_Y - 1,
    BTN_AREA_WIDTH + 1, BTN_AREA_HEIGHT + 1
  );

  TS_StateTypeDef ts;
  int flagDrawing = 1;
  uint16_t xpos = 0, ypos = 0, pxpos = 0, pypos = 0;

  for (;;) {
    BSP_TS_GetState(&ts);

    if (ts.touchDetected) {
      // get xpos and ypos of touch point
      xpos = ts.touchX[0] & 0xFFF;
      ypos = ts.touchY[0] & 0xFFF;

      // drawing area
      if ((DRAW_AREA_X < xpos && xpos < (DRAW_AREA_X + DRAW_AREA_WIDTH)) &&
          (DRAW_AREA_Y < ypos && ypos < (DRAW_AREA_Y + DRAW_AREA_HEIGHT))) {
        if (flagDrawing) {
          flagDrawing = 0;
        } else {
          // drawing lines (for continuous drawing)
          BSP_LCD_DrawLine(pxpos, pypos, xpos, ypos);
        }

        pxpos = xpos, pypos = ypos;
      }

      // scale down action area
      if ((BTN_AREA_X < xpos && xpos < (BTN_AREA_X + BTN_AREA_WIDTH)) &&
          (BTN_AREA_Y < ypos && ypos < (BTN_AREA_Y + BTN_AREA_HEIGHT))) {
        eventButtonTouched = 1;
      }
    } else {
      flagDrawing = 1;
    }
  }
}

#define TENSOR_ARENA_SIZE 83456

void InferenceTask(void const * argument) {
  while (!displayInitialized) osDelay(10);

  uint32_t hdl;
  static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

  TfLiteStatus status = tflm_c_create(
      g_tflm_network_model_data,
      tensor_arena,
      TENSOR_ARENA_SIZE,
      &hdl
  );

  printlog("Model Ready: %d", status);

  struct tflm_c_tensor_info input_tensor;
  struct tflm_c_tensor_info output_tensor;

  volatile static uint8_t drawing_area[DRAW_AREA_HEIGHT][DRAW_AREA_WIDTH];
  volatile static uint8_t drawing_area_small[BTN_AREA_HEIGHT][BTN_AREA_WIDTH];

  uint32_t total_time = 0;
  uint32_t inference_time = 0;

  for (;;) {
    if (eventButtonTouched) {
      total_time = osKernelSysTick();

      printlog("Getting Input Data ...");

      // get drawing area pixels
      for (int i = 0; i < DRAW_AREA_HEIGHT; i++) {
        for (int j = 0; j < DRAW_AREA_WIDTH; j++) {
          drawing_area[i][j] = BSP_LCD_ReadPixel(DRAW_AREA_X + j, DRAW_AREA_Y + i);
        }
      }

      // scaling down
      int x_scale_factor = DRAW_AREA_WIDTH / BTN_AREA_WIDTH;
      int y_scale_factor = DRAW_AREA_HEIGHT / BTN_AREA_HEIGHT;

      for (int i = 0; i < DRAW_AREA_HEIGHT; i++) {
        for (int j = 0; j < DRAW_AREA_WIDTH; j++) {
          if (drawing_area[i][j]) {
            drawing_area_small[i / y_scale_factor][j / x_scale_factor] = 0xFF;
          }
        }
      }

      // draw scaling down data
      for (int i = 0; i < BTN_AREA_HEIGHT; i++) {
        for (int j = 0; j < BTN_AREA_WIDTH; j++) {
          int color = LCD_COLOR_BLACK;
          if (drawing_area_small[i][j]) {
            color = LCD_COLOR_WHITE;
          }
          BSP_LCD_DrawPixel(BTN_AREA_X + j, BTN_AREA_Y + i, color);
        }
      }

      printlog("Data Preprocessing ...");

      tflm_c_input(hdl, 0, &input_tensor);
      float *input = (float *)input_tensor.data;
      for (int i = 0; i < BTN_AREA_HEIGHT; i++) {
        for (int j = 0; j < BTN_AREA_WIDTH; j++) {
          input[BTN_AREA_HEIGHT * i + j] = drawing_area_small[i][j] / 255.0f;
        }
      }

      printlog("Inferring ...");

      inference_time = osKernelSysTick();
      tflm_c_invoke(hdl);
      inference_time = osKernelSysTick() - inference_time;

      tflm_c_output(hdl, 0, &output_tensor);
      float *output = (float *)output_tensor.data;

      // print results
      char result_str[32];
      for (int i = 0; i < 10; i++) {
        sprintf(result_str, "[%d]: %.2f", i, output[i]);
        BSP_LCD_DisplayStringAt(296, 10 + (20 * i), (uint8_t*)result_str, LEFT_MODE);
      }

      total_time = osKernelSysTick() - total_time;
      printlog("%2.3fs(%2.3fs for Inference)",
          (float)total_time / osKernelSysTickFrequency,
          (float)inference_time / osKernelSysTickFrequency
      );

      eventButtonTouched = 0;
    }
  }
}
