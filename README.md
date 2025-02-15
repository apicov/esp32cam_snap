# Build the project

After cloning this project, it should be possible to build it either with `IDF`. The following steps will walk you to build the project.

## Install dependencies

  Clone this project, then inside the project create a `components` folder and install the dependencies:

  ```sh
  cd $PROJECT_PATH/esp32-cam_AI
  mkdir components && cd components
  git clone https://github.com/espressif/esp32-camera.git
  git clone https://github.com/uktechbr/espidf-flatbuffers.git
  git clone https://github.com/apicov/esp-tflite-micro.git

  # set the array branch of esp-tflite-micro
  cd esp-tflite-micro
  git checkout array
  cd $PROJECT_PATH/esp32-cam_AI
  ```

## Build and Flash with IDF (optional)

  With IDF it should be straightforward to build the project, flash it and monitor:

  ```sh
  idf.py build
  idf.py flash monitor
  ```

  To exit the monitor use `Ctrl + ]`.
