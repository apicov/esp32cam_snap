# About

Use the an [esp32-camera](https://components.espressif.com/components/espressif/esp32-camera/versions/2.0.15) supported device to "snap" pictures and publish them via [MQTT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html).

The ESP32-CAM enabled device will subscribe to a configured MQTT topic and wait for the "snap" command, this command will trigger an action to capture and send an image over MQTT to a configured topic (might be a different topic to the one used for the "snap" command).

A concrete example might be to use the ESP32-CAM device to communicate with a [Telegram bot](https://github.com/apicov/telegram_bot_esp32_cam) in order to pass "snap" commands and share images using [Telegram](https://telegram.org/).

# Build the project

The project needs to be configured before being able to build it for the first time; to do this, run the configuration script from the project's root:
```sh
python configure_app.py
```

Once the project is configured, it should be possible run build, flash and monitor it:
  ```sh
  idf.py build
  idf.py flash monitor
  ```

To exit the monitor use `Ctrl + ]`.


# Build the component documentation

To generate the component documentation, [Doxygen](https://www.doxygen.nl/) needs to be
locally installed. The documentation can be generated with the following command:

 ```sh
 cd doc/component/documentation
 doxygen
 ```

The documentation in HTML format is stored in the `html` folder.
