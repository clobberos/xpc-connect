# xpc-connect

[![build status](https://travis-ci.com/jongear/xpc-connect.svg?branch=master)](https://travis-ci.com/jongear/xpc-connect) [![downloads](https://img.shields.io/npm/dt/xpc-connect.svg)](https://www.npmjs.com/package/xpc-connect) [![Contributor count](https://img.shields.io/github/contributors/jongear/xpc-connect.svg)](https://github.com/jongear/xpc-connect/graphs/contributors)

Connection binding for node.js

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
const xpcConnect = require('xpc-connect');
const xpcConnect = new xpcConnect('<Mach service name>');

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

## Build Errors

Before creating a new issue for build errors, please set your path to the following:

```sh
/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/opt/X11/bin
```

MacPorts and other similiar tools might be adding an incompatible compiler to your PATH (see issue [#2](https://github.com/sandeepmistry/node-xpc-connect/issues/2)) for more details.

[![Analytics](https://ga-beacon.appspot.com/UA-56089547-1/sandeepmistry/node-xpc-connect?pixel)](https://github.com/igrigorik/ga-beacon)

