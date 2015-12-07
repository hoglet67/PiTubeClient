// tube-env.h

#ifndef TUBE_ENV_H
#define TUBE_ENV_H

// Type definition for the Error Handler
typedef void (*ErrorHandler_type) ();

// Type definition for the Escape Handler
typedef void (*EscapeHandler_type) ();

// Type definition for the Event Handler
typedef void (*EventHandler_type) (unsigned int reg0, unsigned int reg1, unsigned int reg2);

// Type definition for the Event Handler
typedef void (*ExitHandler_type) ();

// Type definition for the Event Handler
typedef void (*ExceptionHandler_type) ();

// Type definition for the Error Buffer
typedef struct EBT {
   unsigned char *errorAddr;
   unsigned int errorNum;
   char errorMsg[248];
} ErrorBuffer_type;

// Type definition for the environment structure
typedef struct ET {
  char commandBuffer[256];
  unsigned char timeBuffer[8];
  unsigned int escapeFlag;
  unsigned int memoryLimit;
  unsigned int realEndOfMemory;
  unsigned int localBuffering;
  ErrorHandler_type errorHandler;
  ErrorBuffer_type *errorBuffer;
  EscapeHandler_type escapeHandler;
  EventHandler_type eventHandler;
  ExitHandler_type exitHandler;
  ExceptionHandler_type undefinedInstructionHandler;
  ExceptionHandler_type prefetchAbortHandler;
  ExceptionHandler_type dataAbortHandler;
  ExceptionHandler_type addressExceptionHandler;
} Environment_type;

// Global pointer to the environment
extern Environment_type *env;

#endif
