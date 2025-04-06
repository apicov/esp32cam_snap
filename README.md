# Build the project

With IDF it should be straightforward to build the project, flash it and monitor this application:

  ```sh
  # run only once to configure the project
  python configure_app.py

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
