# Object Detection

This project includes a set of libraries written in **pure C** for performing object detection.  
It has been developed and tested on **resource-constrained devices** such as ESP32 boards.  
The project provides a **lightweight, non-AI-based** solution that is suitable for embedded systems.

---

## Project Structure

``` 
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
```


---

## `libraries/`

This folder contains all the core logic for the object detection system, broken down into modular files.  
All global functions are declared in the corresponding `.h` files.

### `constants.h`
Defines all constants used throughout the detection process.  
You can freely modify parameters such as thresholds and configuration values to suit your use case.

### `processing.h`
Includes functions required to preprocess the input image before detection.  

### `detection.h`
Contains the actual object detection algorithms.  
These are lightweight and tailored for environments with limited computational resources.

--- 

## `main/`

### `main.c`
The file `main.c` demonstrates a high-level use case of how to integrate and apply the object detection libraries in a practical application.

---

## Notes

- The code is written with **portability and low overhead** in mind.
- Designed for **real-time processing** without reliance on external libraries or machine learning frameworks.
- Suitable for **microcontroller applications** where memory and CPU are limited.


