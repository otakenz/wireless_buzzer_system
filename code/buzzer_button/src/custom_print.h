#include "HWCDC.h"

#define DEBUG 1

inline void print(const char *msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

inline void print(int msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

inline void print(String msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

inline void print(unsigned char msg, int base) {
  if (DEBUG) {
    Serial.print(msg, base);
  }
}

inline void println(const char *msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

inline void println(int msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

inline void println(String msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}
