# MNIST on device

## Introduction
On-device MNIST inference example using STM32F746G-DISCO Board & Tensorflow Lite for Microcontroller(TFLM)

## Requirements
*  [STM32F746G-DISCO Board](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html)
* [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
* X-CUBE-AI 7.3.0 (Built in this project)

## Tutorial

### Project Setup
1. Create new STM32 project in STM32CubeIDE
    1. Menu [File] > [New] > [STM32 Project]
    1. Tab [Board Selector] > Find & Select 'STM32F746G-DISCO' > Button [Next]
    1. Configure your project (e.g. project name) > Button [Finish]
2. Open the `{project-name}.ioc` file > Click 'Software Packs' in [Pinout & Configuration] Tab > Click 'Select Components'
3. Find and Install STMicroelectronics.X-CUBE-AI > Select Artificial Intelligence X-CUBE-AI Core
4. Save the `{project-name}.ioc` file to generate code

### Build & Train Model
TODO: Add jupyter notebook

### TFLite Model Conversion
1. Store your keras(tensorflow) model to SavedModel using Python
```python
model = ... # Get model
model.save('/path/to/location') # ft.keras.models.save_model()
```
2. Convert SavedModel to TFLite model
```python
import tensorflow as tf

# Convert the model (path to the SavedModel directory)
converter = tf.lite.TFLiteConverter.from_saved_model('/path/to/location')
tflite_model = converter.convert()

# Save the model
with open('model.tflite', 'wb') as f:
  f.write(tflite_model)
```

### Add Model
1. Open the `{Project name}.ioc` file
2. Tab [Pinout & Configuration] > [Categories] > [Software Packs] > Select [STMicroelectronics.X-CUBE-AI]
3. Button [Add network] > Select 'TFLite' and 'TFLite Micro runtime' > Button [Browse...] > Select your TFLite model(`.tflite` file)
    * This step is essential to use TLFM
4. Button [Analyze] > [OK]
5. Save the `{project-name}.ioc` file to generate code and **convert tflite to c bytes array**
    * In `X-CUBE-AI/App/network.c` file, you can see c bytes array of tflite file

### Run Inference
You can see TensorFlow for Microcontroller [Get Started](https://www.tensorflow.org/lite/microcontrollers/get_started_low_level) Reference to run inference. But there are X-CUBE-AI APIs which wrap TFLM classes and functions. It is highly compatible with STM MCU and equivalent to use TFLM library. So, I used X-CUBE-AI APIs.

TODO: Write this section