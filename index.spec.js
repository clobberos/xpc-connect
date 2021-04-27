const xpcConnect = require('./index');

test('test bluetooth connectivity', done => {
  let bluedService = new xpcConnect('com.apple.blued');


  bluedService.on('error', function(message) {
    console.log('error: ' + JSON.stringify(message, undefined, 2));
  });
  
  
  bluedService.on('event', function(event) {
    console.log('event: ' + JSON.stringify(event, undefined, 2));
  });
  
  bluedService.setup();
  
  bluedService.sendMessage({
    kCBMsgId: 1, 
    kCBMsgArgs: {
      kCBMsgArgAlert: 1,
      kCBMsgArgName: 'node'
    }
  });
  
  setTimeout(function() {
    bluedService.shutdown();
    done();   
  }, 500);  
});

test('multiple setups should fail', () => {
  let svc = new xpcConnect('com.apple.blued');
  svc.setup();
  expect(svc.setup).toThrow();
  svc.shutdown();
});

test('send before setup should fail', () => {
  let svc = new xpcConnect('com.apple.blued');
  expect(svc.sendMessage).toThrow();
});

test('can shutdown immediately after setup', () => {
  let svc = new xpcConnect('com.apple.blued');
  svc.setup();
  svc.shutdown();
});

test('can shutdown immediately after receiving message', done => {
  let svc = new xpcConnect('com.apple.blued');

  let fn = (response) => {
    console.log('response: ', JSON.stringify(response, undefined, 2));
    svc.shutdown();
    done();
  };
  svc.on('error', fn);
  svc.on('event', fn);

  svc.setup();
  svc.sendMessage({
    kCBMsgId: 1,
    kCBMsgArgs: {
      kCBMsgArgAlert: 1,
      kCBMsgArgName: 'node'
    }
  });
});
