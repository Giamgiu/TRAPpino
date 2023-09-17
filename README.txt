TRAPPino project - a CNN model for classification of animals on an embedded trap-camera on Arduino.

Build a model and the environment capable to run in local on an Arduino board to classify different species of animals from small pictures, shot in the same board.

DATASET -  A subset (9 classes) of Snapshot Europe 2022. https://app.wildlifeinsights.org/explore/2004446/145625598251_2004446_426_snapshot_euro_2022

Every class contain also some images from web scraping of animals not in the wild.

Hardware - Arduino nano connect and ESP32-S3 boards

Software - Python with Tensorflow, EloquentTinyML library, some custom function to port the model into an arduino TF model


The repository contains 3 folder:

-"Colab" folder: Contain the .ipynb file in which the model is trained and then ported to an Arduino TF model.
-"TRAPpino_ESP": Contains the code to run in Espressif 32 board. This code will shot the photo, save in HD in the SD card and then preprocess, downsized it and send it to arduino
-"TRAPpino_Arduino": Contains the code to run in Arduino nano connect board. This code waits for an image from the first board and then make inference on it. In this folder there is also the TF arduino model (model.h)

