#include <queue>

#import <Foundation/Foundation.h>

#include <node_buffer.h>

#include "XpcConnect.h"
#include <nan.h>

using namespace v8;

Nan::Persistent<FunctionTemplate> XpcConnect::constructor_template;

NAN_MODULE_INIT(XpcConnect::Init) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> tmpl = Nan::New<FunctionTemplate>(New);
  constructor_template.Reset(tmpl);

  tmpl->InstanceTemplate()->SetInternalFieldCount(1);
  tmpl->SetClassName(Nan::New("XpcConnect").ToLocalChecked());

  Nan::SetPrototypeMethod(tmpl, "setup", Setup);
  Nan::SetPrototypeMethod(tmpl, "shutdown", Shutdown);
  Nan::SetPrototypeMethod(tmpl, "sendMessage", SendMessage);

  Nan::Set(target, Nan::New("XpcConnect").ToLocalChecked(), Nan::GetFunction(tmpl).ToLocalChecked());
}

XpcConnect::XpcConnect(std::string serviceName) :
  node::ObjectWrap(),
  serviceName(serviceName),
  dispatchQueue(nullptr),
  xpcConnection(nullptr),
  calledSetup(false),
  calledShutdown(false),
  connectionClosed(false),
  asyncResource(nullptr),
  asyncHandle(nullptr) {
}

XpcConnect::~XpcConnect() {
}

void XpcConnect::setup() {
  if (this->calledSetup) {
    return;
  }
  this->calledSetup = true;

  // Prevent `this`/the wrapped JS object from being destroyed until the XPC
  // connection is fully closed (after a call to shutdown())
  this->Ref();

  this->asyncHandle = new uv_async_t;
  uv_async_init(uv_default_loop(), this->asyncHandle, (uv_async_cb)XpcConnect::AsyncCallback);
  this->asyncHandle->data = this;

  this->asyncResource = new Nan::AsyncResource("XpcConnect");

  uv_mutex_init(&this->eventQueueMutex);

  this->dispatchQueue = dispatch_queue_create(this->serviceName.c_str(), 0);
  this->xpcConnection = xpc_connection_create_mach_service(this->serviceName.c_str(), this->dispatchQueue, XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);

  xpc_connection_set_event_handler(this->xpcConnection, ^(xpc_object_t event) {
    xpc_retain(event);
    this->queueEvent(event);
  });

  xpc_connection_activate(this->xpcConnection);
}

void XpcConnect::shutdown() {
  this->calledShutdown = true;
  if (this->xpcConnection != nullptr) {
    xpc_connection_cancel(this->xpcConnection);
  }
}

void XpcConnect::sendMessage(xpc_object_t message) {
  if (this->xpcConnection != nullptr) {
    xpc_connection_send_message(this->xpcConnection, message);
  }
}

void XpcConnect::queueEvent(xpc_object_t event) {

  uv_mutex_lock(&this->eventQueueMutex);
  eventQueue.push(event);
  uv_mutex_unlock(&this->eventQueueMutex);

  uv_async_send(this->asyncHandle);
}

NAN_METHOD(XpcConnect::New) {
  Nan::HandleScope scope;
  std::string serviceName = "";

  if (info.Length() > 0 && info[0]->IsString()) {
    Nan::Utf8String arg0(info[0]);

    serviceName = *arg0;
  }

  XpcConnect* p = new XpcConnect(serviceName);
  p->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}


NAN_METHOD(XpcConnect::Setup) {
  Nan::HandleScope scope;

  XpcConnect* p = node::ObjectWrap::Unwrap<XpcConnect>(info.This());

  if (p->calledSetup) {
    Nan::ThrowError("XpcConnect setup already called");
  } else {
    p->setup();
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(XpcConnect::Shutdown) {
  Nan::HandleScope scope;

  XpcConnect* p = node::ObjectWrap::Unwrap<XpcConnect>(info.This());

  p->shutdown();

  info.GetReturnValue().SetUndefined();
}

xpc_object_t XpcConnect::ValueToXpcObject(Local<Value> value) {
  xpc_object_t xpcObject = NULL;

  if (value->IsInt32() || value->IsUint32()) {
    xpcObject = xpc_int64_create(value->IntegerValue(Nan::GetCurrentContext()).FromJust());
  } else if (value->IsString()) {
    Nan::Utf8String valueString(value);

    xpcObject = xpc_string_create(*valueString);
  } else if (value->IsArray()) {
    Local<Array> valueArray = Local<Array>::Cast(value);

    xpcObject = XpcConnect::ArrayToXpcObject(valueArray);
  } else if (node::Buffer::HasInstance(value)) {
    Local<Object> valueObject = value.As<Object>();

    if (Nan::HasRealNamedProperty(valueObject, Nan::New("isUuid").ToLocalChecked()).FromMaybe(false)) {
      uuid_t *uuid = (uuid_t *)node::Buffer::Data(valueObject);

      xpcObject = xpc_uuid_create(*uuid);
    } else {
      xpcObject = xpc_data_create(node::Buffer::Data(valueObject), node::Buffer::Length(valueObject));
    }
  } else if (value->IsObject()) {
    Local<Object> valueObject = value.As<Object>();

    xpcObject = XpcConnect::ObjectToXpcObject(valueObject);
  } else {
  }

  return xpcObject;
}

xpc_object_t XpcConnect::ObjectToXpcObject(Local<Object> object) {
  xpc_object_t xpcObject = xpc_dictionary_create(NULL, NULL, 0);

  Local<Array> propertyNames = Nan::GetPropertyNames(object).ToLocalChecked();

  for(uint32_t i = 0; i < propertyNames->Length(); i++) {
    Local<Value> propertyName = Nan::Get(propertyNames, i).ToLocalChecked();

    if (propertyName->IsString()) {
      Nan::Utf8String propertyNameString(propertyName);

      Local<Value> propertyValue = Nan::GetRealNamedProperty(object, propertyName.As<String>()).ToLocalChecked();

      xpc_object_t xpcValue = XpcConnect::ValueToXpcObject(propertyValue);
      xpc_dictionary_set_value(xpcObject, *propertyNameString, xpcValue);
      if (xpcValue) {
        xpc_release(xpcValue);
      }
    }
  }

  return xpcObject;
}

xpc_object_t XpcConnect::ArrayToXpcObject(Local<Array> array) {
  xpc_object_t xpcArray = xpc_array_create(NULL, 0);

  for(uint32_t i = 0; i < array->Length(); i++) {
    Local<Value> value = Nan::Get(array, i).ToLocalChecked();

    xpc_object_t xpcValue = XpcConnect::ValueToXpcObject(value);
    xpc_array_append_value(xpcArray, xpcValue);
    if (xpcValue) {
      xpc_release(xpcValue);
    }
  }

  return xpcArray;
}

Local<Value> XpcConnect::XpcObjectToValue(xpc_object_t xpcObject) {
  Local<Value> value;

  xpc_type_t valueType = xpc_get_type(xpcObject);

  if (valueType == XPC_TYPE_INT64) {
    value = Nan::New((int32_t)xpc_int64_get_value(xpcObject));
  } else if(valueType == XPC_TYPE_STRING) {
    value = Nan::New(xpc_string_get_string_ptr(xpcObject)).ToLocalChecked();
  } else if(valueType == XPC_TYPE_DICTIONARY) {
    value = XpcConnect::XpcDictionaryToObject(xpcObject);
  } else if(valueType == XPC_TYPE_ARRAY) {
    value = XpcConnect::XpcArrayToArray(xpcObject);
  } else if(valueType == XPC_TYPE_DATA) {
    value = Nan::CopyBuffer((char *)xpc_data_get_bytes_ptr(xpcObject), xpc_data_get_length(xpcObject)).ToLocalChecked();
  } else if(valueType == XPC_TYPE_UUID) {
    value = Nan::CopyBuffer((char *)xpc_uuid_get_bytes(xpcObject), sizeof(uuid_t)).ToLocalChecked();
  } else {
    NSLog(@"XpcObjectToValue: Could not convert to value!, %@", xpcObject);
  }

  return value;
}

Local<Object> XpcConnect::XpcDictionaryToObject(xpc_object_t xpcDictionary) {
  Local<Object> object = Nan::New<Object>();

  xpc_dictionary_apply(xpcDictionary, ^bool(const char *key, xpc_object_t value) {
    Nan::Set(object, Nan::New<String>(key).ToLocalChecked(), XpcConnect::XpcObjectToValue(value));

    return true;
  });

  return object;
}

Local<Array> XpcConnect::XpcArrayToArray(xpc_object_t xpcArray) {
  Local<Array> array = Nan::New<Array>();

  xpc_array_apply(xpcArray, ^bool(size_t index, xpc_object_t value) {
    Nan::Set(array, Nan::New<Number>(index), XpcConnect::XpcObjectToValue(value));

    return true;
  });

  return array;
}

void XpcConnect::AsyncCallback(uv_async_t* handle) {
  XpcConnect *xpcConnection = (XpcConnect*)handle->data;

  xpcConnection->processEventQueue();
}

void XpcConnect::AsyncCloseCallback(uv_async_t* handle) {
  delete handle;
}

void XpcConnect::processEventQueue() {
  uv_mutex_lock(&this->eventQueueMutex);

  Nan::HandleScope scope;

  while (!this->eventQueue.empty()) {
    xpc_object_t event = this->eventQueue.front();
    this->eventQueue.pop();

    xpc_type_t eventType = xpc_get_type(event);
    if (eventType == XPC_TYPE_ERROR) {
      const char* message = "unknown";

      if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
        message = "connection interrupted";
      } else if (event == XPC_ERROR_CONNECTION_INVALID) {
        message = "connection invalid";
        this->connectionClosed = true;

        // Backwards compatibility: XPC always emits connection invalid
        // at the end of a connection; it's labeled as an "error" but
        // really it's standard behavior. However, previous versions of
        // XpcConnect didn't emit connection invalid when calling
        // shutdown(), so keep it that way.
        if (this->calledShutdown) {
          break;
        }
      }

      Local<Value> argv[2] = {
        Nan::New("error").ToLocalChecked(),
        Nan::New(message).ToLocalChecked()
      };

      this->asyncResource->runInAsyncScope(this->handle(), Nan::New("emit").ToLocalChecked(), 2, argv);
    } else if (eventType == XPC_TYPE_DICTIONARY) {
      Local<Object> eventObject = XpcConnect::XpcDictionaryToObject(event);

      Local<Value> argv[2] = {
        Nan::New("event").ToLocalChecked(),
        eventObject
      };

      this->asyncResource->runInAsyncScope(this->handle(), Nan::New("emit").ToLocalChecked(), 2, argv);
    }

    xpc_release(event);
  }

  uv_mutex_unlock(&this->eventQueueMutex);

  if (this->connectionClosed && this->xpcConnection != nullptr) {
    // Connection has been closed/cancelled, do cleanup.
    // (XPC_ERROR_CONNECTION_INVALID is always/only emitted as the last
    // event of a connection, so there will be no more queued events.)

    xpc_release(this->xpcConnection);
    this->xpcConnection = nullptr;
    dispatch_release(this->dispatchQueue);
    this->dispatchQueue = nullptr;
    uv_mutex_destroy(&this->eventQueueMutex);
    uv_close((uv_handle_t*)this->asyncHandle, (uv_close_cb)XpcConnect::AsyncCloseCallback);
    this->asyncHandle = nullptr;
    delete this->asyncResource;
    this->asyncResource = nullptr;
    this->Unref();
  }
}

NAN_METHOD(XpcConnect::SendMessage) {
  Nan::HandleScope scope;
  XpcConnect* p = node::ObjectWrap::Unwrap<XpcConnect>(info.This());

  if (!p->calledSetup) {
    Nan::ThrowError("XpcConnect cannot send message before setup is called");
  } else if (info.Length() > 0) {
    Local<Value> arg0 = info[0];
    if (arg0->IsObject()) {
      Local<Object> object = Local<Object>::Cast(arg0);

      xpc_object_t message = XpcConnect::ObjectToXpcObject(object);
      p->sendMessage(message);
      xpc_release(message);
    }
  }

  info.GetReturnValue().SetUndefined();
}

NODE_MODULE(binding, XpcConnect::Init);
