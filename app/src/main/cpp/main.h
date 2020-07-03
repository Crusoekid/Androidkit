//
// Created by jiangzhongbo on 2019/12/17.
//

#ifndef FACE_LANDMARK_MAIN_H
#define FACE_LANDMARK_MAIN_H

#include <jni.h>
class Session {
public:
    bool ok;
    unsigned char r[5];
    unsigned char *key;
    int key_len;
public:
};


#endif //FACE_LANDMARK_MAIN_H
