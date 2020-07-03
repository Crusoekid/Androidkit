#include <jni.h>
#include <string>
#include <string.h>
#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

extern "C" {
#include "aes.h"
}
//CRYPT CONFIG
#define MAX_LEN (3*1024*1024)
#define ENCRYPT 0
#define DECRYPT 1
#define AES_KEY_SIZE 256
#define READ_LEN 10


//AES_IV
static unsigned char AES_IV[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
//AES_KEY
static unsigned char AES_KEY[32] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71,
                                     0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c,
                                     0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf,
                                     0xf4 };
/**
 * unsigned char 转化为 jbyteArray 返回
 * @param env
 * @param buf
 * @param len
 * @return
 */
jbyteArray as_java_byte_array(JNIEnv *env, unsigned char *buf, int len) {
    jbyteArray array = env->NewByteArray(len);
    env->SetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte *>(buf));
    return array;
}

/**
 * jbyteArray 转化为 unsigned char 返回
 * @param env
 * @param array
 * @return
 */
unsigned char *as_unsigned_char_array(JNIEnv *env, jbyteArray array) {
    int len = env->GetArrayLength(array);
    unsigned char *buf = new unsigned char[len];
    env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte *>(buf));
    return buf;
}

jbyteArray crypt(JNIEnv *env, jbyteArray jarray, jint jmode) {
    //check input data
    unsigned int len = (unsigned int) (env->GetArrayLength(jarray));
    if (len <= 0 || len >= MAX_LEN) {
        return NULL;
    }

    unsigned char *data = (unsigned char*) env->GetByteArrayElements(
            jarray, NULL);
    if (!data) {
        return NULL;
    }

    //计算填充长度，当为加密方式且长度不为16的整数倍时，则填充，与3DES填充类似(DESede/CBC/PKCS5Padding)
    unsigned int mode = (unsigned int) jmode;
    unsigned int rest_len = len % AES_BLOCK_SIZE;
    unsigned int padding_len = (
            (ENCRYPT == mode) ? (AES_BLOCK_SIZE - rest_len) : 0);
    unsigned int src_len = len + padding_len;

    //设置输入
    unsigned char *input = (unsigned char *) malloc(src_len);
    memset(input, 0, src_len);
    memcpy(input, data, len);
    if (padding_len > 0) {
        memset(input + len, (unsigned char) padding_len, padding_len);
    }
    //data不再使用
    env->ReleaseByteArrayElements(jarray, (jbyte*)data, 0);

    //设置输出Buffer
    unsigned char * buff = (unsigned char*) malloc(src_len);
    if (!buff) {
        free(input);
        return NULL;
    }
    memset(buff, 0, src_len);

    //set key & iv
    unsigned int key_schedule[AES_BLOCK_SIZE * 4] = { 0 }; //>=53(这里取64)
    aes_key_setup(AES_KEY, key_schedule, AES_KEY_SIZE);

    //执行加解密计算(CBC mode)
    if (mode == ENCRYPT) {
        aes_encrypt_cbc(input, src_len, buff, key_schedule, AES_KEY_SIZE,
                        AES_IV);
    } else {
        aes_decrypt_cbc(input, src_len, buff, key_schedule, AES_KEY_SIZE,
                        AES_IV);
    }

    //解密时计算填充长度
    if (ENCRYPT != mode) {
        unsigned char * ptr = buff;
        ptr += (src_len - 1);
        padding_len = (unsigned int) *ptr;
        if (padding_len > 0 && padding_len <= AES_BLOCK_SIZE) {
            src_len -= padding_len;
        }
        ptr = NULL;
    }

    //设置返回变量
    jbyteArray bytes = env->NewByteArray(src_len);
    env->SetByteArrayRegion(bytes, 0, src_len, (jbyte*) buff);

    //内存释放
    free(input);
    free(buff);

    return bytes;
}



jbyteArray decode(JNIEnv *env,jbyteArray data){
    return crypt(env, data, 1);

}

extern "C"{
JNIEXPORT jbyteArray JNICALL
Java_com_zj_myapplication_MainActivity_encode(JNIEnv *env, jclass obj, jbyteArray data){
    return crypt(env, data, 0);
}

JNIEXPORT jbyteArray JNICALL
Java_com_zj_myapplication_MainActivity_decode(JNIEnv *env, jclass obj, jbyteArray data){
    return decode(env, data);
}
}
