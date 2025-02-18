# Testing

The tests in this project follow the guidelines outlined int the ESP-IDF
[documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/esp-idf-tests-with-pytest.html).

## Install the dependencies

This test suite requires the `pytest-embedded` module, which can be installed by any of these means:
  1. Use the ESP-IDF's `install.sh` script as explained in the [documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/esp-idf-tests-with-pytest.html#installation)
  2. Use `pip` as explained in the module's [repository](https://github.com/espressif/pytest-embedded).

## Run the tests

To run the tests in this folder, execute the following command from this
project's root folder:

```shell
pytest tests/
```

