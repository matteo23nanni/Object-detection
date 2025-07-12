# Object-detection

The current project contains a series of libraries in pure C language used for object detection.
The project was made and tested on low resources devices like ESP32 boards.
It offers a light solution with no AI involved suitable for embedded systems.

Below is short explanation of remaining files in the project folder.

├── main
│   └── main.c
├── libraries
│   ├── detection.c
│   ├── detection.h
│   ├── processing.c
│   ├── processing.h
│   ├── constants.c
│   └── constants.h
└── README.md                  This is the file you are currently reading

## libraries

Contains all the functions that can be used for object detection.
All global functions are declared in each file header.

**constants.h**
Contains all the constants used in the detection process. The user can be freely change parameters such as thresholds.
**processing.h**
Contains all the function needed to preprocess an image before any detection process.
**detection.h**
Contains all the function that can be used to detect an object.

## main

The file `main.c` contains a high level example of a possible app that applies object detection.

