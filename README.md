# Build the project

To build the project for the first time, then it needs to be configured
before building it. Run the following script to configure the project:
```sh
# run only once to configure the project
python configure_app.py
```

Once the project is configured, it's possible run build, flash and monitor it:
  ```sh
  # build it
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
