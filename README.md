imagegroup
==========

A program written in C++ for rapid manual grouping of images. The source requires BOOST 1.5.55 (or greater) and OpenCV 2.4.6 (or greater) to compile.

To compile on a Linux platform using g++ use the following code after changing directory to the main.cpp containing path.

 ```sh
 g++ main.cpp -o imggrp -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_nonfree -lopencv_objdetect -lopencv_ml -lopencv_features2d -lboost_filesystem -lboost_system -lboost_regex

 ```
