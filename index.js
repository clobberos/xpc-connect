var events = require('events');

var binding = require('bindings')('xpc-connect.node');
var XpcConnect = binding.XpcConnect;

inherits(XpcConnect, events.EventEmitter);

// extend prototype
function inherits(target, source) {
  for (var k in source.prototype) {
    target.prototype[k] = source.prototype[k];
  }
}

module.exports = XpcConnect;
