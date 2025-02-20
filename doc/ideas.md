# About

The purpose of this file is to outline some ideas and targets for this
project. The outline doesn't have an order and may change continuously as the
priority/interests shift.

The aim of this project is to create something that can be used by more people,
although the final product isn't clear yet, we want to use this project as a mean 
to learn different development tools/libraries.

Many of the points outlined in the following sections may endup as project
"Issues" so we can approach them independently.

## Tools

Among the things that we want to explore and hence to put attention for this
project are:

- [ ] learn the architecture of the ESP32 SoC and the IDF framework
- [ ] make more use of C++'s stdlib
- [ ] start writing tests
- [ ] use a documentation tool
- [ ] improve the knowledge of CMake
- [ ] see up to what extent we can use AI in this microcontroller, i.e
      `tensorflow-lite`
- [ ] learn more of the components included int he IDF framework

## Next steps

### Start saving images in a remote server

After starting to taking pictures and storing them in the SD card, the process
`"Eject SD" -> "Unload data" -> "Reinsert SD"` loop became cumbersome, this
raised the need to thinking to store the data in a different way, potentially in
a remote server via HTTP or some other mean.
