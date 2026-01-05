# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

xpc-connect is a Node.js native addon that provides bindings for Apple's XPC (Inter-Process Communication) Services on macOS. It allows Node.js applications to communicate with macOS system services using XPC connections.

**Platform:** macOS only (specified in package.json os field)
**Node Version:** >=18.0

## Common Commands

### Building
```bash
npm run build          # Rebuild native addon (runs node-gyp rebuild)
npm run prebuild       # Clean build directory before building
```

### Testing
```bash
npm test               # Run all tests
npm run test:coverage  # Run tests with coverage
npm run validate       # Run test:coverage (pre-commit validation)
```

### Linting
```bash
npm run lint           # Run ESLint
```

### Committing
This project uses **commitizen** for standardized commits:
```bash
git add -A
git commit -m "your message"   # Triggers commitizen workflow
```
The commit hook will guide you through creating a conventional changelog-formatted commit. All commits must follow `cz-conventional-changelog` standards.

## Architecture

### Layer Structure

1. **JavaScript Layer** (`index.js`)
   - Thin wrapper that loads the native addon via `bindings` package
   - Extends the native XpcConnect class with EventEmitter functionality
   - Entry point for require('xpc-connect')

2. **Native Addon Layer** (`src/XpcConnect.cpp`, `src/XpcConnect.h`)
   - C++ implementation using NAN (Native Abstractions for Node.js)
   - Wraps Apple's XPC C API for Node.js consumption
   - Handles async event processing via libuv

3. **Build Configuration** (`binding.gyp`)
   - node-gyp configuration for compiling the native addon
   - macOS-specific compilation settings (ObjC++ support)

### XPC Connection Lifecycle

The native addon manages XPC connections through a specific lifecycle:

1. **Initialization**: Create XpcConnect instance with service name
2. **Setup**: Call `setup()` once to establish connection
   - Creates dispatch queue
   - Creates XPC connection to mach service
   - Sets up event handler
   - Activates connection
   - **Can only be called once** (throws on multiple calls)
3. **Communication**: Use `sendMessage(obj)` to send messages
   - **Must be called after setup()** (throws if called before)
4. **Shutdown**: Call `shutdown()` to close connection
   - Cancels XPC connection
   - Cleanup happens asynchronously when XPC_ERROR_CONNECTION_INVALID is received

### Event Processing

The addon uses a queue-based async event processing model:

- XPC events are queued on the dispatch queue thread
- Events are transferred to a mutex-protected queue
- `uv_async_send()` triggers processing on the Node.js event loop
- Events are emitted as Node.js EventEmitter events ('error' or 'event')

### Memory Management

Important patterns in the native code:

- Uses `this->Ref()` during setup to prevent garbage collection until connection closes
- Uses `this->Unref()` when connection fully closes after XPC_ERROR_CONNECTION_INVALID
- Properly retains/releases XPC objects using `xpc_retain()`/`xpc_release()`
- Async resources are cleaned up via `AsyncCloseCallback` when connection closes

### Data Type Conversions

The addon supports bidirectional conversion between JavaScript and XPC types:

**JavaScript → XPC:**
- int32/uint32 → XPC_TYPE_INT64
- string → XPC_TYPE_STRING
- Array → XPC_TYPE_ARRAY
- Object → XPC_TYPE_DICTIONARY
- Buffer → XPC_TYPE_DATA
- Buffer with `isUuid` property → XPC_TYPE_UUID

**XPC → JavaScript:**
- XPC_TYPE_INT64 → int32
- XPC_TYPE_STRING → string
- XPC_TYPE_ARRAY → Array
- XPC_TYPE_DICTIONARY → Object
- XPC_TYPE_DATA → Buffer
- XPC_TYPE_UUID → Buffer (containing uuid_t bytes)

Conversion logic is in:
- `ValueToXpcObject()`, `ObjectToXpcObject()`, `ArrayToXpcObject()` (JS → XPC)
- `XpcObjectToValue()`, `XpcDictionaryToObject()`, `XpcArrayToArray()` (XPC → JS)

## Testing

Tests use Jest and interact with real macOS services (e.g., `com.apple.blued` for Bluetooth).

Key test patterns:
- Tests interact with actual system services, so they may require system permissions
- Tests verify proper error handling (setup before sendMessage, multiple setup calls, etc.)
- Tests verify the connection can be immediately shutdown after setup or after receiving messages

## Development Notes

- This is a **macOS-only** native addon. Cannot be developed or run on other platforms.
- Uses **NAN (Native Abstractions for Node.js)** for Node.js version compatibility
- The native code uses **Objective-C++** (hence the `-ObjC++` compiler flag)
- XPC connections require proper entitlements and may need system permissions
- The addon creates a privileged XPC connection (`XPC_CONNECTION_MACH_SERVICE_PRIVILEGED`)
