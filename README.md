# About

Use an [esp32-camera](https://components.espressif.com/components/espressif/esp32-camera/versions/2.0.15) supported device to "snap" pictures and publish them via [MQTT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html).

The ESP32-CAM enabled device will subscribe to a configured MQTT topic and wait for the "snap" command, this command will trigger an action to capture and send an image over MQTT to a configured topic (might be a different topic to the one used for the "snap" command).

A concrete example may be to use an ESP32-CAM device to communicate with a user created [Telegram bot](https://github.com/apicov/esp32cam_snapbot), then take snapshots and share them from a [Telegram](https://telegram.org/) account.

# Build the project

The project needs to be configured before building it for the first time. Run the "menuconfig" of the project and configure all the items listed under the `esp32cam_snap` menu
```sh
idf.py menuconfig
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
