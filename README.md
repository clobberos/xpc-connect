**[Install](#install)** | **[Contributing](#contributing)** 

# xpc-connect

[![build status](https://travis-ci.com/jongear/xpc-connect.svg?branch=master)](https://travis-ci.com/jongear/xpc-connect) [![downloads](https://img.shields.io/npm/dt/xpc-connect.svg)](https://www.npmjs.com/package/xpc-connect) [![Contributor count](https://img.shields.io/github/contributors/jongear/xpc-connect.svg)](https://github.com/jongear/xpc-connect/graphs/contributors)

[XPC Services connection](https://developer.apple.com/documentation/xpc/xpc_services_connection_h) bindings for node.js

## Supported data types

 * int32/uint32
 * string
 * array
 * buffer
 * uuid
 * object

## Install
```js
npm install xpc-connect
```

## Example

```js
const XpcConnect = require('xpc-connect');
const xpcConnect = new XpcConnect('<Mach service name>');

xpcConnect.on('error', function(message) {
    ...
});

xpcConnect.on('event', function(event) {
    ...
});

xpcConnect.setup();

const mesage = {
    ... 
};

xpcConnect.sendMessage(mesage);
```

## Contributing

Please checkout the [contributing guide](CONTRIBUTING.md) to learn about our release process.