#include <unistd.h>

#include <string>

#include "lib/v8/include/libplatform/libplatform.h"
#include "lib/v8/include/v8-context.h"
#include "lib/v8/include/v8-initialization.h"
#include "lib/v8/include/v8-isolate.h"
#include "lib/v8/include/v8-local-handle.h"
#include "lib/v8/include/v8-primitive.h"
#include "lib/v8/include/v8-script.h"
#include "utility/logger.h"
#include "xian_net.h"

void test() {
  auto pingType = make_shared<string>("ping");
  uint32_t ping1 = XianNet::GetInstance().NewService(pingType);
  uint32_t ping2 = XianNet::GetInstance().NewService(pingType);
  uint32_t pong = XianNet::GetInstance().NewService(pingType);
  auto msg1 =
      XianNet::GetInstance().MakeMsg(ping1, new char[3]{'h', 'i', '\0'}, 3);
  // auto msg2 = XianNet::GetInstance().MakeMsg(ping2, new char[6] { 'h', 'e',
  // 'l', 'l', 'o', '\0' }, 6);
  XianNet::GetInstance().SendMessage(pong, msg1);
}

void TestSocketCtrl() {
  int fd = XianNet::GetInstance().Listen(8001, 1);
  usleep(15 * 1000000);
  XianNet::GetInstance().CloseConn(fd);
}

void TestEcho() {
  auto t = make_shared<string>("gateway");
  uint32_t gateway = XianNet::GetInstance().NewService(t);
}

void StartV8() {
  Info("v8引擎开始初始化");
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);
    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    {
      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source =
          v8::String::NewFromUtf8Literal(isolate, "'Hello' + ', World!'");

      // Compile the source code.
      v8::Local<v8::Script> script =
          v8::Script::Compile(context, source).ToLocalChecked();

      // Run the script to get the result.
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

      // Convert the result to an UTF8 string and print it.
      v8::String::Utf8Value utf8(isolate, result);

      Info("输出1： {}", *utf8);
    }

    {
      // Use the JavaScript API to generate a WebAssembly module.
      //
      // |bytes| contains the binary format for the following module:
      //
      //     (func (export "add") (param i32 i32) (result i32)
      //       get_local 0
      //       get_local 1
      //       i32.add)
      //

      const char csource[] = R"(
        let bytes = new Uint8Array([
          0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
          0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
          0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
          0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
        ]);
        let module = new WebAssembly.Module(bytes);
        let instance = new WebAssembly.Instance(module);
        instance.exports.add(3, 4);
      )";

      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source =
          v8::String::NewFromUtf8Literal(isolate, csource);

      // Compile the source code.
      v8::Local<v8::Script> script =
          v8::Script::Compile(context, source).ToLocalChecked();

      // Run the script to get the result.
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

      // Convert the result to a uint32 and print it.
      uint32_t number = result->Uint32Value(context).ToChecked();

      Info("输出2： 3 + 4 = {}", number);
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params.array_buffer_allocator;
}

int main() {
  // TestEcho();
  // Info("\n(╯°□°）╯︵ ┻━┻  XianNet启动成功   ┳━┳ノ(゜-゜ノ)");
  // XianNet::GetInstance().Wait();
  // TestEcho();
  StartV8();
  return 0;
}