// tube-env.h

#ifndef TUBE_ENV_H
#define TUBE_ENV_H

// Type definition for the Error Buffer
typedef struct EBT {
   unsigned char *errorAddr;
   unsigned int errorNum;
   char errorMsg[256];
} ErrorBuffer_type;

// Type definition for the Error Handler
typedef void (*ErrorHandler_type) (ErrorBuffer_type *eb);

// Type definition for the Escape Handler
typedef void (*EscapeHandler_type) (unsigned int flag);

// Type definition for the Event Handler
typedef void (*EventHandler_type) (unsigned int a, unsigned int x, unsigned int y);

// Type definition for the Event Handler
typedef void (*ExitHandler_type) ();

// Type definition for the Event Handler
typedef void (*ExceptionHandler_type) ();

// Type definition for the environment structure
typedef struct ET {
  // Buffers
  char commandBuffer[256];
  unsigned char timeBuffer[8];
  // Memory Limits
  unsigned int memoryLimit;
  unsigned int realEndOfMemory;
  // Error
  ErrorHandler_type errorHandler;
  ErrorBuffer_type *errorBufferPtr;
  // Escape
  EscapeHandler_type escapeHandler;
  unsigned int *escapeFlagPtr;
  // Events
  EventHandler_type eventHandler;
  // Exit
  ExitHandler_type exitHandler;
  // Low-level
  ExceptionHandler_type undefinedInstructionHandler;
  ExceptionHandler_type prefetchAbortHandler;
  ExceptionHandler_type dataAbortHandler;
  ExceptionHandler_type addressExceptionHandler;
} Environment_type;

// Global pointer to the environment
extern Environment_type *env;

#endif
