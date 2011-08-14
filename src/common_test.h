// Copyright 2011 Google Inc. All Rights Reserved.

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base64.h"
#include "heap.h"
#include "thread.h"
#include "stringprintf.h"
#include "class_linker.h"
#include "dex_file.h"

#include "gtest/gtest.h"

namespace art {

// package java.lang;
// public class Object {}

// package java.lang;
// public class Class {}
//
// package java.lang.reflect;
// public class Field {}
//
// package java.lang.reflect;
// public class Method {}
//
// package java.lang;
// public class String {
//   char[] value;
//   int hashCode;
//   int offset;
//   int count;
// }
//
// package java.lang;
// public class ClassLoader {
//   Object packages;
//   ClassLoader parent;
// }
//
// package dalvik.system;
// public class BaseDexClassLoader extends ClassLoader {
//   String originalPath;
//   Object pathList;
// }
//
// package dalvik.system;
// public class PathClassLoader extends BaseDexClassLoader {}
//
// package java.lang;
// public interface Cloneable {}
//
// package java.io;
// public interface Serializable {}
//
// package java.lang;
// public class StackTraceElement {
//   String declaringClass;
//   String methodName;
//   String fileName;
//   int lineNumber;
// }
//
// package java.lang;
// public class Throwable {
//   String detailMessage;
//   Throwable cause;
//   Object suppressedExceptions;
//   Object stackState;
//   StackTraceElement[] stackTrace;
// }
//
// package java.lang;
// public class Exception extends Throwable {}
//
// package java.lang;
// public class NullPointerException extends Exception {}
//
static const char kJavaLangDex[] =
  "ZGV4CjAzNQCEjWBqSq808bjn0gC+ptCv0wtDNadp4vEQCgAAcAAAAHhWNBIAAAAAAAAAAHwJAAAy"
  "AAAAcAAAABIAAAA4AQAAAQAAAIABAAARAAAAjAEAAAwAAAAUAgAADgAAAHQCAADcBQAANAQAAFAF"
  "AABYBQAAcQUAAH0FAACPBQAAnwUAAK8FAAC7BQAAvgUAAOIFAAADBgAAGwYAAC4GAABHBgAAXgYA"
  "AHUGAACXBgAAqwYAAMoGAADeBgAA9QYAABAHAAAsBwAAOQcAAFQHAABhBwAAdwcAAIoHAACiBwAA"
  "rwcAAL8HAADCBwAAxgcAAOYHAADtBwAA9AcAAAQIAAATCAAAHQgAACcIAAAzCAAAPwgAAEcIAABV"
  "CAAAXwgAAGcIAABxCAAAfQgAAIkIAACfCAAABwAAAAgAAAAJAAAACgAAAAsAAAAMAAAADQAAAA4A"
  "AAAPAAAAEAAAABEAAAASAAAAEwAAABQAAAAVAAAAHgAAAB8AAAAgAAAAHgAAAA8AAAAAAAAAAQAL"
  "ACoAAAABAAkALQAAAAUACQArAAAABQAFACwAAAAKAAsAIwAAAAoACwAlAAAACgAAACcAAAAKAAsA"
  "KAAAAAsAAAAiAAAACwAAACYAAAALAAAAKQAAAAsAEAAxAAAADAAMACEAAAAMAAsAJAAAAAwACQAu"
  "AAAADAARAC8AAAAMAAkAMAAAAAEAAAAAAAAAAgAAAAAAAAAEAAAAAAAAAAUAAAAAAAAABwAAAAAA"
  "AAAIAAAAAAAAAAkAAAAAAAAACgAAAAAAAAALAAAAAAAAAAwAAAAAAAAADQAAAAAAAAAOAAAAAAAA"
  "AAkAAAABAAAA/////wAAAAAYAAAAAAAAAOIIAAAAAAAABQAAAAEAAAAJAAAAAAAAAAMAAAAAAAAA"
  "7AgAAAAAAAABAAAAAQAAAAUAAAAAAAAAAQAAAAAAAAD6CAAAAAAAAAIAAAABAAAAAQAAAAAAAAAZ"
  "AAAAAAAAAAgJAAAAAAAAAwAAAAEGAAAJAAAAAAAAABoAAAAAAAAAAAAAAAAAAAAEAAAAAQAAAAkA"
  "AAAAAAAAAgAAAAAAAAASCQAAAAAAAAYAAAABBgAACQAAAAAAAAAEAAAAAAAAAAAAAAAAAAAADAAA"
  "AAEAAAAJAAAAAAAAAB0AAAAAAAAAHAkAAAAAAAAHAAAAAQAAAAwAAAAAAAAABQAAAAAAAAAwCQAA"
  "AAAAAAgAAAABAAAABwAAAAAAAAAXAAAAAAAAADoJAAAAAAAACgAAAAEAAAAJAAAAAAAAABsAAAAA"
  "AAAARAkAAAAAAAALAAAAAQAAAAkAAAAAAAAAHAAAAAAAAABWCQAAAAAAAA0AAAABAAAACQAAAAAA"
  "AAAGAAAAAAAAAGgJAAAAAAAADgAAAAEAAAAJAAAAAAAAABYAAAAAAAAAcgkAAAAAAAABAAEAAAAA"
  "AKYIAAABAAAADgAAAAEAAQABAAAAqwgAAAQAAABwEAYAAAAOAAEAAQABAAAAsAgAAAQAAABwEAMA"
  "AAAOAAEAAQABAAAAtQgAAAQAAABwEAAAAAAOAAEAAQABAAAAuggAAAQAAABwEAYAAAAOAAEAAQAB"
  "AAAAvwgAAAQAAABwEAYAAAAOAAEAAQABAAAAxAgAAAQAAABwEAkAAAAOAAEAAQABAAAAyQgAAAQA"
  "AABwEAQAAAAOAAEAAQABAAAAzggAAAQAAABwEAYAAAAOAAEAAQABAAAA0wgAAAQAAABwEAYAAAAO"
  "AAEAAQABAAAA2AgAAAQAAABwEAYAAAAOAAEAAQABAAAA3QgAAAQAAABwEAYAAAAOAAY8aW5pdD4A"
  "F0Jhc2VEZXhDbGFzc0xvYWRlci5qYXZhAApDbGFzcy5qYXZhABBDbGFzc0xvYWRlci5qYXZhAA5D"
  "bG9uZWFibGUuamF2YQAORXhjZXB0aW9uLmphdmEACkZpZWxkLmphdmEAAUkAIkxkYWx2aWsvc3lz"
  "dGVtL0Jhc2VEZXhDbGFzc0xvYWRlcjsAH0xkYWx2aWsvc3lzdGVtL1BhdGhDbGFzc0xvYWRlcjsA"
  "FkxqYXZhL2lvL1NlcmlhbGl6YWJsZTsAEUxqYXZhL2xhbmcvQ2xhc3M7ABdMamF2YS9sYW5nL0Ns"
  "YXNzTG9hZGVyOwAVTGphdmEvbGFuZy9DbG9uZWFibGU7ABVMamF2YS9sYW5nL0V4Y2VwdGlvbjsA"
  "IExqYXZhL2xhbmcvTnVsbFBvaW50ZXJFeGNlcHRpb247ABJMamF2YS9sYW5nL09iamVjdDsAHUxq"
  "YXZhL2xhbmcvU3RhY2tUcmFjZUVsZW1lbnQ7ABJMamF2YS9sYW5nL1N0cmluZzsAFUxqYXZhL2xh"
  "bmcvVGhyb3dhYmxlOwAZTGphdmEvbGFuZy9yZWZsZWN0L0ZpZWxkOwAaTGphdmEvbGFuZy9yZWZs"
  "ZWN0L01ldGhvZDsAC01ldGhvZC5qYXZhABlOdWxsUG9pbnRlckV4Y2VwdGlvbi5qYXZhAAtPYmpl"
  "Y3QuamF2YQAUUGF0aENsYXNzTG9hZGVyLmphdmEAEVNlcmlhbGl6YWJsZS5qYXZhABZTdGFja1Ry"
  "YWNlRWxlbWVudC5qYXZhAAtTdHJpbmcuamF2YQAOVGhyb3dhYmxlLmphdmEAAVYAAltDAB5bTGph"
  "dmEvbGFuZy9TdGFja1RyYWNlRWxlbWVudDsABWNhdXNlAAVjb3VudAAOZGVjbGFyaW5nQ2xhc3MA"
  "DWRldGFpbE1lc3NhZ2UACGZpbGVOYW1lAAhoYXNoQ29kZQAKbGluZU51bWJlcgAKbWV0aG9kTmFt"
  "ZQAGb2Zmc2V0AAxvcmlnaW5hbFBhdGgACHBhY2thZ2VzAAZwYXJlbnQACHBhdGhMaXN0AApzdGFj"
  "a1N0YXRlAApzdGFja1RyYWNlABRzdXBwcmVzc2VkRXhjZXB0aW9ucwAFdmFsdWUAAwAHDgAFAAcO"
  "AAUABw4ABQAHDgAFAAcOAAUABw4ABQAHDgAFAAcOAAUABw4ABQAHDgAFAAcOAAUABw4AAAABAAaB"
  "gAS0CAACAQACAAEAA4GABMgIAAIBAAAAAQAAgYAE4AgAAAEAAYGABPgIAAABAAKBgASQCQAFAQAM"
  "AAEAAQABAAEACYGABKgJAAABAASBgATACQAAAQAFgYAE2AkABAEABAABAAEAAQAHgYAE8AkABAEA"
  "CAABAAEAAQAIgYAEiAoAAAEACoGABKAKAAABAAuBgAS4CgwAAAAAAAAAAQAAAAAAAAABAAAAMgAA"
  "AHAAAAACAAAAEgAAADgBAAADAAAAAQAAAIABAAAEAAAAEQAAAIwBAAAFAAAADAAAABQCAAAGAAAA"
  "DgAAAHQCAAABIAAADAAAADQEAAACIAAAMgAAAFAFAAADIAAADAAAAKYIAAAAIAAADAAAAOIIAAAA"
  "EAAAAQAAAHwJAAA=";

// package java.lang;
// public class Object {}
//
// class MyClass {}
static const char kMyClassDex[] =
  "ZGV4CjAzNQA5Nm9IrCVm91COwepff7LhIE23GZIxGjgIAgAAcAAAAHhWNBIAAAAAAAAAAIABAAAG"
  "AAAAcAAAAAMAAACIAAAAAQAAAJQAAAAAAAAAAAAAAAIAAACgAAAAAgAAALAAAAAYAQAA8AAAABwB"
  "AAAkAQAALwEAAEMBAABRAQAAXgEAAAEAAAACAAAABQAAAAUAAAACAAAAAAAAAAAAAAAAAAAAAQAA"
  "AAAAAAABAAAAAQAAAP////8AAAAABAAAAAAAAABrAQAAAAAAAAAAAAAAAAAAAQAAAAAAAAADAAAA"
  "AAAAAHUBAAAAAAAAAQABAAAAAABhAQAAAQAAAA4AAAABAAEAAQAAAGYBAAAEAAAAcBABAAAADgAG"
  "PGluaXQ+AAlMTXlDbGFzczsAEkxqYXZhL2xhbmcvT2JqZWN0OwAMTXlDbGFzcy5qYXZhAAtPYmpl"
  "Y3QuamF2YQABVgACAAcOAAUABw4AAAABAAGBgATwAQAAAQAAgIAEhAIACwAAAAAAAAABAAAAAAAA"
  "AAEAAAAGAAAAcAAAAAIAAAADAAAAiAAAAAMAAAABAAAAlAAAAAUAAAACAAAAoAAAAAYAAAACAAAA"
  "sAAAAAEgAAACAAAA8AAAAAIgAAAGAAAAHAEAAAMgAAACAAAAYQEAAAAgAAACAAAAawEAAAAQAAAB"
  "AAAAgAEAAA==";

// class Nested {
//     class Inner {
//     }
// }
static const char kNestedDex[] =
  "ZGV4CjAzNQAQedgAe7gM1B/WHsWJ6L7lGAISGC7yjD2IAwAAcAAAAHhWNBIAAAAAAAAAAMQCAAAP"
  "AAAAcAAAAAcAAACsAAAAAgAAAMgAAAABAAAA4AAAAAMAAADoAAAAAgAAAAABAABIAgAAQAEAAK4B"
  "AAC2AQAAvQEAAM0BAADXAQAA+wEAABsCAAA+AgAAUgIAAF8CAABiAgAAZgIAAHMCAAB5AgAAgQIA"
  "AAIAAAADAAAABAAAAAUAAAAGAAAABwAAAAkAAAAJAAAABgAAAAAAAAAKAAAABgAAAKgBAAAAAAEA"
  "DQAAAAAAAQAAAAAAAQAAAAAAAAAFAAAAAAAAAAAAAAAAAAAABQAAAAAAAAAIAAAAiAEAAKsCAAAA"
  "AAAAAQAAAAAAAAAFAAAAAAAAAAgAAACYAQAAuAIAAAAAAAACAAAAlAIAAJoCAAABAAAAowIAAAIA"
  "AgABAAAAiAIAAAYAAABbAQAAcBACAAAADgABAAEAAQAAAI4CAAAEAAAAcBACAAAADgBAAQAAAAAA"
  "AAAAAAAAAAAATAEAAAAAAAAAAAAAAAAAAAEAAAABAAY8aW5pdD4ABUlubmVyAA5MTmVzdGVkJElu"
  "bmVyOwAITE5lc3RlZDsAIkxkYWx2aWsvYW5ub3RhdGlvbi9FbmNsb3NpbmdDbGFzczsAHkxkYWx2"
  "aWsvYW5ub3RhdGlvbi9Jbm5lckNsYXNzOwAhTGRhbHZpay9hbm5vdGF0aW9uL01lbWJlckNsYXNz"
  "ZXM7ABJMamF2YS9sYW5nL09iamVjdDsAC05lc3RlZC5qYXZhAAFWAAJWTAALYWNjZXNzRmxhZ3MA"
  "BG5hbWUABnRoaXMkMAAFdmFsdWUAAgEABw4AAQAHDjwAAgIBDhgBAgMCCwQADBcBAgQBDhwBGAAA"
  "AQEAAJAgAICABNQCAAABAAGAgATwAgAAEAAAAAAAAAABAAAAAAAAAAEAAAAPAAAAcAAAAAIAAAAH"
  "AAAArAAAAAMAAAACAAAAyAAAAAQAAAABAAAA4AAAAAUAAAADAAAA6AAAAAYAAAACAAAAAAEAAAMQ"
  "AAACAAAAQAEAAAEgAAACAAAAVAEAAAYgAAACAAAAiAEAAAEQAAABAAAAqAEAAAIgAAAPAAAArgEA"
  "AAMgAAACAAAAiAIAAAQgAAADAAAAlAIAAAAgAAACAAAAqwIAAAAQAAABAAAAxAIAAA==";

// class ProtoCompare {
//     int m1(short x, int y, long z) { return x + y + (int)z; }
//     int m2(short x, int y, long z) { return x + y + (int)z; }
//     int m3(long x, int y, short z) { return (int)x + y + z; }
//     long m4(long x, int y, short z) { return x + y + z; }
// }
static const char kProtoCompareDex[] =
  "ZGV4CjAzNQBLUetu+TVZ8gsYsCOFoij7ecsHaGSEGA8gAwAAcAAAAHhWNBIAAAAAAAAAAIwCAAAP"
  "AAAAcAAAAAYAAACsAAAABAAAAMQAAAAAAAAAAAAAAAYAAAD0AAAAAQAAACQBAADcAQAARAEAAN4B"
  "AADmAQAA6QEAAO8BAAD1AQAA+AEAAP4BAAAOAgAAIgIAADUCAAA4AgAAOwIAAD8CAABDAgAARwIA"
  "AAEAAAAEAAAABgAAAAcAAAAJAAAACgAAAAIAAAAAAAAAyAEAAAMAAAAAAAAA1AEAAAUAAAABAAAA"
  "yAEAAAoAAAAFAAAAAAAAAAIAAwAAAAAAAgABAAsAAAACAAEADAAAAAIAAAANAAAAAgACAA4AAAAD"
  "AAMAAAAAAAIAAAAAAAAAAwAAAAAAAAAIAAAAAAAAAHACAAAAAAAAAQABAAEAAABLAgAABAAAAHAQ"
  "BQAAAA4ABwAFAAAAAABQAgAABQAAAJAAAwSEUbAQDwAAAAcABQAAAAAAWAIAAAUAAACQAAMEhFGw"
  "EA8AAAAGAAUAAAAAAGACAAAEAAAAhCCwQLBQDwAJAAUAAAAAAGgCAAAFAAAAgXC7UIGCuyAQAAAA"
  "AwAAAAEAAAAEAAAAAwAAAAQAAAABAAY8aW5pdD4AAUkABElKSVMABElTSUoAAUoABEpKSVMADkxQ"
  "cm90b0NvbXBhcmU7ABJMamF2YS9sYW5nL09iamVjdDsAEVByb3RvQ29tcGFyZS5qYXZhAAFTAAFW"
  "AAJtMQACbTIAAm0zAAJtNAABAAcOAAIDAAAABw4AAwMAAAAHDgAEAwAAAAcOAAUDAAAABw4AAAAB"
  "BACAgATEAgEA3AIBAPgCAQCUAwEArAMAAAwAAAAAAAAAAQAAAAAAAAABAAAADwAAAHAAAAACAAAA"
  "BgAAAKwAAAADAAAABAAAAMQAAAAFAAAABgAAAPQAAAAGAAAAAQAAACQBAAABIAAABQAAAEQBAAAB"
  "EAAAAgAAAMgBAAACIAAADwAAAN4BAAADIAAABQAAAEsCAAAAIAAAAQAAAHACAAAAEAAAAQAAAIwC"
  "AAA=";

// class ProtoCompare2 {
//     int m1(short x, int y, long z) { return x + y + (int)z; }
//     int m2(short x, int y, long z) { return x + y + (int)z; }
//     int m3(long x, int y, short z) { return (int)x + y + z; }
//     long m4(long x, int y, short z) { return x + y + z; }
// }
static const char kProtoCompare2Dex[] =
  "ZGV4CjAzNQDVUXj687EpyTTDJZEZPA8dEYnDlm0Ir6YgAwAAcAAAAHhWNBIAAAAAAAAAAIwCAAAP"
  "AAAAcAAAAAYAAACsAAAABAAAAMQAAAAAAAAAAAAAAAYAAAD0AAAAAQAAACQBAADcAQAARAEAAN4B"
  "AADmAQAA6QEAAO8BAAD1AQAA+AEAAP4BAAAPAgAAIwIAADcCAAA6AgAAPQIAAEECAABFAgAASQIA"
  "AAEAAAAEAAAABgAAAAcAAAAJAAAACgAAAAIAAAAAAAAAyAEAAAMAAAAAAAAA1AEAAAUAAAABAAAA"
  "yAEAAAoAAAAFAAAAAAAAAAIAAwAAAAAAAgABAAsAAAACAAEADAAAAAIAAAANAAAAAgACAA4AAAAD"
  "AAMAAAAAAAIAAAAAAAAAAwAAAAAAAAAIAAAAAAAAAHICAAAAAAAAAQABAAEAAABNAgAABAAAAHAQ"
  "BQAAAA4ABwAFAAAAAABSAgAABQAAAJAAAwSEUbAQDwAAAAcABQAAAAAAWgIAAAUAAACQAAMEhFGw"
  "EA8AAAAGAAUAAAAAAGICAAAEAAAAhCCwQLBQDwAJAAUAAAAAAGoCAAAFAAAAgXC7UIGCuyAQAAAA"
  "AwAAAAEAAAAEAAAAAwAAAAQAAAABAAY8aW5pdD4AAUkABElKSVMABElTSUoAAUoABEpKSVMAD0xQ"
  "cm90b0NvbXBhcmUyOwASTGphdmEvbGFuZy9PYmplY3Q7ABJQcm90b0NvbXBhcmUyLmphdmEAAVMA"
  "AVYAAm0xAAJtMgACbTMAAm00AAEABw4AAgMAAAAHDgADAwAAAAcOAAQDAAAABw4ABQMAAAAHDgAA"
  "AAEEAICABMQCAQDcAgEA+AIBAJQDAQCsAwwAAAAAAAAAAQAAAAAAAAABAAAADwAAAHAAAAACAAAA"
  "BgAAAKwAAAADAAAABAAAAMQAAAAFAAAABgAAAPQAAAAGAAAAAQAAACQBAAABIAAABQAAAEQBAAAB"
  "EAAAAgAAAMgBAAACIAAADwAAAN4BAAADIAAABQAAAE0CAAAAIAAAAQAAAHICAAAAEAAAAQAAAIwC"
  "AAA=";

// javac MyClass.java && dx --dex --output=MyClass.dex
//   --core-library MyClass.class java/lang/Object.class && base64 MyClass.dex
// package java.lang;
// public class Object {}
// class MyClass {
//   native void foo();
//   native int fooI(int x);
//   native int fooII(int x, int y);
//   native double fooDD(double x, double y);
//   native Object fooIOO(int x, Object y, Object z);
//   static native Object fooSIOO(int x, Object y, Object z);
//   static synchronized native Object fooSSIOO(int x, Object y, Object z);
// }
static const char kMyClassNativesDex[] =
  "ZGV4CjAzNQA4WWrpXgdlkoTHR8Yubx4LJO4HbGsX1p1EAwAAcAAAAHhWNBIAAAAAAAAAALACAAAT"
  "AAAAcAAAAAUAAAC8AAAABQAAANAAAAAAAAAAAAAAAAkAAAAMAQAAAgAAAFQBAACwAQAAlAEAAOIB"
  "AADqAQAA7QEAAPIBAAD1AQAA+QEAAP4BAAAEAgAADwIAACMCAAAxAgAAPgIAAEECAABGAgAATQIA"
  "AFMCAABaAgAAYgIAAGsCAAABAAAAAwAAAAcAAAAIAAAACwAAAAIAAAAAAAAAwAEAAAQAAAABAAAA"
  "yAEAAAUAAAABAAAA0AEAAAYAAAADAAAA2AEAAAsAAAAEAAAAAAAAAAIABAAAAAAAAgAEAAwAAAAC"
  "AAAADQAAAAIAAQAOAAAAAgACAA8AAAACAAMAEAAAAAIAAwARAAAAAgADABIAAAADAAQAAAAAAAMA"
  "AAABAAAA/////wAAAAAKAAAAAAAAAH8CAAAAAAAAAgAAAAAAAAADAAAAAAAAAAkAAAAAAAAAiQIA"
  "AAAAAAABAAEAAAAAAHUCAAABAAAADgAAAAEAAQABAAAAegIAAAQAAABwEAgAAAAOAAIAAAAAAAAA"
  "AQAAAAEAAAACAAAAAQABAAMAAAABAAMAAwAGPGluaXQ+AAFEAANEREQAAUkAAklJAANJSUkABExJ"
  "TEwACUxNeUNsYXNzOwASTGphdmEvbGFuZy9PYmplY3Q7AAxNeUNsYXNzLmphdmEAC09iamVjdC5q"
  "YXZhAAFWAANmb28ABWZvb0REAARmb29JAAVmb29JSQAGZm9vSU9PAAdmb29TSU9PAAhmb29TU0lP"
  "TwADAAcOAAEABw4AAAABAAiBgASUAwAAAwUAgIAEqAMGiAIAAaiCCAABgAIAAYACAAGAAgABgAIA"
  "AYACAAwAAAAAAAAAAQAAAAAAAAABAAAAEwAAAHAAAAACAAAABQAAALwAAAADAAAABQAAANAAAAAF"
  "AAAACQAAAAwBAAAGAAAAAgAAAFQBAAABIAAAAgAAAJQBAAABEAAABAAAAMABAAACIAAAEwAAAOIB"
  "AAADIAAAAgAAAHUCAAAAIAAAAgAAAH8CAAAAEAAAAQAAALACAAA=";

// class CreateMethodDescriptor {
//     Float m1(int a, double b, long c, Object d) { return null; }
//     CreateMethodDescriptor m2(boolean x, short y, char z) { return null; }
// }
static const char kCreateMethodDescriptorDex[] =
  "ZGV4CjAzNQBSU7aKdNXwH+uOpti/mvZ4/Dk8wM8VtNbgAgAAcAAAAHhWNBIAAAAAAAAAAEwCAAAQ"
  "AAAAcAAAAAoAAACwAAAAAwAAANgAAAAAAAAAAAAAAAQAAAD8AAAAAQAAABwBAACkAQAAPAEAAJQB"
  "AACcAQAAnwEAALwBAAC/AQAAwgEAAMUBAADfAQAA5gEAAOwBAAD/AQAAEwIAABYCAAAZAgAAHAIA"
  "ACACAAABAAAAAwAAAAQAAAAFAAAABgAAAAkAAAAKAAAACwAAAAwAAAANAAAACAAAAAQAAAB8AQAA"
  "BwAAAAUAAACIAQAADAAAAAgAAAAAAAAABAACAAAAAAAEAAEADgAAAAQAAAAPAAAABgACAAAAAAAE"
  "AAAAAAAAAAYAAAAAAAAAAgAAAAAAAAA6AgAAAAAAAAEAAQABAAAAJAIAAAQAAABwEAMAAAAOAAgA"
  "BwAAAAAAKQIAAAIAAAASABEABQAEAAAAAAAyAgAAAgAAABIAEQADAAAACQAHAAAAAAAEAAAAAgAB"
  "AAMABgAGPGluaXQ+AAFDABtDcmVhdGVNZXRob2REZXNjcmlwdG9yLmphdmEAAUQAAUkAAUoAGExD"
  "cmVhdGVNZXRob2REZXNjcmlwdG9yOwAFTElESkwABExaU0MAEUxqYXZhL2xhbmcvRmxvYXQ7ABJM"
  "amF2YS9sYW5nL09iamVjdDsAAVMAAVYAAVoAAm0xAAJtMgABAAcOAAIEAAAAAAcOAAMDAAAABw4A"
  "AAABAgCAgAS8AgEA1AIBAOgCDAAAAAAAAAABAAAAAAAAAAEAAAAQAAAAcAAAAAIAAAAKAAAAsAAA"
  "AAMAAAADAAAA2AAAAAUAAAAEAAAA/AAAAAYAAAABAAAAHAEAAAEgAAADAAAAPAEAAAEQAAACAAAA"
  "fAEAAAIgAAAQAAAAlAEAAAMgAAADAAAAJAIAAAAgAAABAAAAOgIAAAAQAAABAAAATAIAAA==";

// class X {}
// class Y extends X {}
static const char kXandY[] =
  "ZGV4CjAzNQAlLMqyB72TxJW4zl5w75F072u4Ig6KvCMEAgAAcAAAAHhWNBIAAAAAAAAAAHwBAAAG"
  "AAAAcAAAAAQAAACIAAAAAQAAAJgAAAAAAAAAAAAAAAMAAACkAAAAAgAAALwAAAAIAQAA/AAAACwB"
  "AAA0AQAAOQEAAD4BAABSAQAAVQEAAAEAAAACAAAAAwAAAAQAAAAEAAAAAwAAAAAAAAAAAAAAAAAA"
  "AAEAAAAAAAAAAgAAAAAAAAAAAAAAAAAAAAIAAAAAAAAABQAAAAAAAABnAQAAAAAAAAEAAAAAAAAA"
  "AAAAAAAAAAAFAAAAAAAAAHEBAAAAAAAAAQABAAEAAABdAQAABAAAAHAQAgAAAA4AAQABAAEAAABi"
  "AQAABAAAAHAQAAAAAA4ABjxpbml0PgADTFg7AANMWTsAEkxqYXZhL2xhbmcvT2JqZWN0OwABVgAG"
  "WC5qYXZhAAIABw4AAwAHDgAAAAEAAICABPwBAAABAAGAgASUAgALAAAAAAAAAAEAAAAAAAAAAQAA"
  "AAYAAABwAAAAAgAAAAQAAACIAAAAAwAAAAEAAACYAAAABQAAAAMAAACkAAAABgAAAAIAAAC8AAAA"
  "ASAAAAIAAAD8AAAAAiAAAAYAAAAsAQAAAyAAAAIAAABdAQAAACAAAAIAAABnAQAAABAAAAEAAAB8"
  "AQAA";

// class Statics {
//   static boolean s0 = true;
//   static byte s1 = 5;
//   static char s2 = 'a';
//   static short s3 = (short) 65000;
//   static int s4 = 2000000000;
//   static long s5 = 0x123456789abcdefL;
//   static float s6 = 0.5f;
//   static double s7 = 16777217;
//   static Object s8 = "android";
//   static Object[] s9 = { "a", "b" };
// }
static const char kStatics[] =
  "ZGV4CjAzNQAYalInXcX4y0OBgb2yCw2/jGzZBSe34zmwAwAAcAAAAHhWNBIAAAAAAAAAABwDAAAc"
  "AAAAcAAAAAwAAADgAAAAAQAAABABAAAKAAAAHAEAAAMAAABsAQAAAQAAAIQBAAAMAgAApAEAADwC"
  "AABGAgAATgIAAFECAABUAgAAVwIAAFoCAABdAgAAYAIAAGsCAAB/AgAAggIAAJACAACTAgAAlgIA"
  "AKsCAACuAgAAtwIAALoCAAC+AgAAwgIAAMYCAADKAgAAzgIAANICAADWAgAA2gIAAN4CAAACAAAA"
  "AwAAAAQAAAAFAAAABgAAAAcAAAAIAAAACQAAAAoAAAAMAAAADQAAAA4AAAAMAAAACQAAAAAAAAAG"
  "AAoAEgAAAAYAAAATAAAABgABABQAAAAGAAgAFQAAAAYABAAWAAAABgAFABcAAAAGAAMAGAAAAAYA"
  "AgAZAAAABgAHABoAAAAGAAsAGwAAAAYAAAAAAAAABgAAAAEAAAAHAAAAAQAAAAYAAAAAAAAABwAA"
  "AAAAAAALAAAAAAAAAPUCAAAAAAAABAAAAAAAAADiAgAAOAAAABITagMAABJQawABABMAYQBsAAIA"
  "EwDo/W0AAwAUAACUNXdnAAQAGADvzauJZ0UjAWgABQAVAAA/ZwAGABgAAAAAEAAAcEFoAAcAGgAQ"
  "AGkACAASICMACwASARoCDwBNAgABGgERAE0BAANpAAkADgABAAEAAQAAAPACAAAEAAAAcBACAAAA"
  "DgAIPGNsaW5pdD4ABjxpbml0PgABQgABQwABRAABRgABSQABSgAJTFN0YXRpY3M7ABJMamF2YS9s"
  "YW5nL09iamVjdDsAAVMADFN0YXRpY3MuamF2YQABVgABWgATW0xqYXZhL2xhbmcvT2JqZWN0OwAB"
  "YQAHYW5kcm9pZAABYgACczAAAnMxAAJzMgACczMAAnM0AAJzNQACczYAAnM3AAJzOAACczkAAgAH"
  "HS08S0taeEt4SwABAAcOAAoAAgAACAEIAQgBCAEIAQgBCAEIAQgBCACIgASkAwGAgASkBAAAAAwA"
  "AAAAAAAAAQAAAAAAAAABAAAAHAAAAHAAAAACAAAADAAAAOAAAAADAAAAAQAAABABAAAEAAAACgAA"
  "ABwBAAAFAAAAAwAAAGwBAAAGAAAAAQAAAIQBAAABIAAAAgAAAKQBAAACIAAAHAAAADwCAAADIAAA"
  "AgAAAOICAAAAIAAAAQAAAPUCAAAAEAAAAQAAABwDAAA=";

// class Main {
//   public static void main(String args[]) {
//   }
// }
static const char kMainDex[] =
  "ZGV4CjAzNQAPNypTL1TulODHFdpEa2pP98I7InUu7uQgAgAAcAAAAHhWNBIAAAAAAAAAAIwBAAAI"
  "AAAAcAAAAAQAAACQAAAAAgAAAKAAAAAAAAAAAAAAAAMAAAC4AAAAAQAAANAAAAAwAQAA8AAAACIB"
  "AAAqAQAAMgEAAEYBAABRAQAAVAEAAFgBAABtAQAAAQAAAAIAAAAEAAAABgAAAAQAAAACAAAAAAAA"
  "AAUAAAACAAAAHAEAAAAAAAAAAAAAAAABAAcAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAADAAAA"
  "AAAAAH4BAAAAAAAAAQABAAEAAABzAQAABAAAAHAQAgAAAA4AAQABAAAAAAB4AQAAAQAAAA4AAAAB"
  "AAAAAwAGPGluaXQ+AAZMTWFpbjsAEkxqYXZhL2xhbmcvT2JqZWN0OwAJTWFpbi5qYXZhAAFWAAJW"
  "TAATW0xqYXZhL2xhbmcvU3RyaW5nOwAEbWFpbgABAAcOAAMBAAcOAAAAAgAAgIAE8AEBCYgCDAAA"
  "AAAAAAABAAAAAAAAAAEAAAAIAAAAcAAAAAIAAAAEAAAAkAAAAAMAAAACAAAAoAAAAAUAAAADAAAA"
  "uAAAAAYAAAABAAAA0AAAAAEgAAACAAAA8AAAAAEQAAABAAAAHAEAAAIgAAAIAAAAIgEAAAMgAAAC"
  "AAAAcwEAAAAgAAABAAAAfgEAAAAQAAABAAAAjAEAAA==";

// class StaticLeafMethods {
//   static void nop() {
//   }
//   static byte identity(byte x) {
//     return x;
//   }
//   static int identity(int x) {
//     return x;
//   }
//   static int sum(int a, int b) {
//     return a + b;
//   }
//   static int sum(int a, int b, int c) {
//     return a + b + c;
//   }
//   static int sum(int a, int b, int c, int d) {
//     return a + b + c + d;
//   }
//   static int sum(int a, int b, int c, int d, int e) {
//     return a + b + c + d + e;
//   }
//   static double identity(double x) {
//     return x;
//   }
//   static double sum(double a, double b) {
//     return a + b;
//   }
//   static double sum(double a, double b, double c) {
//     return a + b + c;
//   }
//   static double sum(double a, double b, double c, double d) {
//     return a + b + c + d;
//   }
//   static double sum(double a, double b, double c, double d, double e) {
//     return a + b + c + d + e;
//   }
// }
static const char kStaticLeafMethodsDex[] =
  "ZGV4CjAzNQD8gEpaFD0w5dM8dsPaCQ3wIh0xaUjfni+IBQAAcAAAAHhWNBIAAAAAAAAAAPQEAAAW"
  "AAAAcAAAAAYAAADIAAAADAAAAOAAAAAAAAAAAAAAAA4AAABwAQAAAQAAAOABAACIAwAAAAIAAK4D"
  "AAC2AwAAuQMAAL0DAADAAwAAxAMAAMkDAADPAwAA1gMAAN4DAADhAwAA5QMAAOoDAADwAwAA9wMA"
  "AP8DAAAUBAAAKAQAAEAEAABDBAAATQQAAFIEAAABAAAAAwAAAAkAAAAPAAAAEAAAABIAAAACAAAA"
  "AAAAADgDAAAEAAAAAQAAAEADAAAFAAAAAQAAAEgDAAAGAAAAAQAAAFADAAAHAAAAAQAAAFwDAAAI"
  "AAAAAQAAAGgDAAAKAAAAAgAAAHgDAAALAAAAAgAAAIADAAAMAAAAAgAAAIgDAAANAAAAAgAAAJQD"
  "AAAOAAAAAgAAAKADAAASAAAABQAAAAAAAAADAAsAAAAAAAMAAAATAAAAAwABABMAAAADAAYAEwAA"
  "AAMACwAUAAAAAwACABUAAAADAAMAFQAAAAMABAAVAAAAAwAFABUAAAADAAcAFQAAAAMACAAVAAAA"
  "AwAJABUAAAADAAoAFQAAAAQACwAAAAAAAwAAAAAAAAAEAAAAAAAAABEAAAAAAAAAtwQAAAAAAAAB"
  "AAEAAQAAAFcEAAAEAAAAcBANAAAADgABAAEAAAAAAFwEAAABAAAADwAAAAIAAgAAAAAAYgQAAAEA"
  "AAAQAAAAAQABAAAAAABoBAAAAQAAAA8AAAAAAAAAAAAAAG4EAAABAAAADgAAAAYABAAAAAAAcwQA"
  "AAMAAACrAAIEEAAAAAgABgAAAAAAegQAAAQAAACrAAIEy2AQAAoACAAAAAAAggQAAAUAAACrAAIE"
  "y2DLgBAAAAAMAAoAAAAAAIsEAAAGAAAAqwACBMtgy4DLoBAAAwACAAAAAACVBAAAAwAAAJAAAQIP"
  "AAAABAADAAAAAACcBAAABAAAAJAAAQKwMA8ABQAEAAAAAACkBAAABQAAAJAAAQKwMLBADwAAAAYA"
  "BQAAAAAArQQAAAYAAACQAAECsDCwQLBQDwABAAAAAAAAAAEAAAABAAAAAgAAAAEAAQADAAAAAQAB"
  "AAEAAAAEAAAAAQABAAEAAQAFAAAAAQABAAEAAQABAAAAAQAAAAIAAAACAAAAAgACAAMAAAACAAIA"
  "AgAAAAQAAAACAAIAAgACAAUAAAACAAIAAgACAAIABjxpbml0PgABQgACQkIAAUQAAkREAANEREQA"
  "BEREREQABUREREREAAZEREREREQAAUkAAklJAANJSUkABElJSUkABUlJSUlJAAZJSUlJSUkAE0xT"
  "dGF0aWNMZWFmTWV0aG9kczsAEkxqYXZhL2xhbmcvT2JqZWN0OwAWU3RhdGljTGVhZk1ldGhvZHMu"
  "amF2YQABVgAIaWRlbnRpdHkAA25vcAADc3VtAAEABw4ABQEABw4AFwEABw4ACAEABw4AAwAHDgAa"
  "AgAABw4AHQMAAAAHDgAgBAAAAAAHDgAjBQAAAAAABw4ACwIAAAcOAA4DAAAABw4AEQQAAAAABw4A"
  "FAUAAAAAAAcOAAAADQAAgIAEgAQBCJgEAQisBAEIwAQBCNQEAQjoBAEIgAUBCJgFAQi0BQEI0AUB"
  "COgFAQiABgEInAYAAAAMAAAAAAAAAAEAAAAAAAAAAQAAABYAAABwAAAAAgAAAAYAAADIAAAAAwAA"
  "AAwAAADgAAAABQAAAA4AAABwAQAABgAAAAEAAADgAQAAASAAAA0AAAAAAgAAARAAAAsAAAA4AwAA"
  "AiAAABYAAACuAwAAAyAAAA0AAABXBAAAACAAAAEAAAC3BAAAABAAAAEAAAD0BAAA";

static inline DexFile* OpenDexFileBase64(const char* base64) {
  CHECK(base64 != NULL);
  size_t length;
  byte* dex_bytes = DecodeBase64(base64, &length);
  CHECK(dex_bytes != NULL);
  DexFile* dex_file = DexFile::OpenPtr(dex_bytes, length);
  CHECK(dex_file != NULL);
  return dex_file;
}

class RuntimeTest : public testing::Test {
 protected:
  virtual void SetUp() {
    is_host_ = getenv("ANDROID_BUILD_TOP") != NULL;

    android_data_.reset(strdup(is_host_ ? "/tmp/art-data-XXXXXX" : "/sdcard/art-data-XXXXXX"));
    ASSERT_TRUE(android_data_ != NULL);
    const char* android_data_modified = mkdtemp(android_data_.get());
    // note that mkdtemp side effects android_data_ as well
    ASSERT_TRUE(android_data_modified != NULL);
    setenv("ANDROID_DATA", android_data_modified, 1);
    art_cache_.append(android_data_.get());
    art_cache_.append("/art-cache");
    int mkdir_result = mkdir(art_cache_.c_str(), 0700);
    ASSERT_EQ(mkdir_result, 0);

    java_lang_dex_file_.reset(OpenDexFileBase64(kJavaLangDex));

    std::vector<DexFile*> boot_class_path;
    boot_class_path.push_back(java_lang_dex_file_.get());

    runtime_.reset(Runtime::Create(boot_class_path));
    ASSERT_TRUE(runtime_ != NULL);
    class_linker_ = runtime_->GetClassLinker();
  }

  virtual void TearDown() {
    const char* android_data = getenv("ANDROID_DATA");
    ASSERT_TRUE(android_data != NULL);
    DIR* dir = opendir(art_cache_.c_str());
    ASSERT_TRUE(dir != NULL);
    while (true) {
      struct dirent entry;
      struct dirent* entry_ptr;
      int readdir_result = readdir_r(dir, &entry, &entry_ptr);
      ASSERT_EQ(0, readdir_result);
      if (entry_ptr == NULL) {
        break;
      }
      if ((strcmp(entry_ptr->d_name, ".") == 0) || (strcmp(entry_ptr->d_name, "..") == 0)) {
        continue;
      }
      std::string filename(art_cache_);
      filename.push_back('/');
      filename.append(entry_ptr->d_name);
      int unlink_result = unlink(filename.c_str());
      ASSERT_EQ(0, unlink_result);
    }
    closedir(dir);
    int rmdir_cache_result = rmdir(art_cache_.c_str());
    ASSERT_EQ(0, rmdir_cache_result);
    int rmdir_data_result = rmdir(android_data_.get());
    ASSERT_EQ(0, rmdir_data_result);
  }

  std::string GetLibCoreDexFileName() {
    if (is_host_) {
      const char* host_dir = getenv("ANDROID_HOST_OUT");
      CHECK(host_dir != NULL);
      return StringPrintf("%s/framework/core-hostdex.jar", host_dir);
    }
    return std::string("/system/framework/core.jar");
  }

  DexFile* GetLibCoreDex() {
    std::string libcore_dex_file_name = GetLibCoreDexFileName();
    return DexFile::OpenZip(libcore_dex_file_name);
  }

  void UseLibCoreDex() {
    delete runtime_.release();
    java_lang_dex_file_.reset(GetLibCoreDex());

    std::vector<DexFile*> boot_class_path;
    boot_class_path.push_back(java_lang_dex_file_.get());

    runtime_.reset(Runtime::Create(boot_class_path));
    ASSERT_TRUE(runtime_ != NULL);
    class_linker_ = runtime_->GetClassLinker();
  }

  PathClassLoader* AllocPathClassLoader(const DexFile* dex_file) {
    class_linker_->RegisterDexFile(dex_file);
    std::vector<const DexFile*> dex_files;
    dex_files.push_back(dex_file);
    return class_linker_->AllocPathClassLoader(dex_files);
  }

  bool is_host_;
  scoped_ptr_malloc<char> android_data_;
  std::string art_cache_;
  scoped_ptr<DexFile> java_lang_dex_file_;
  scoped_ptr<Runtime> runtime_;
  ClassLinker* class_linker_;
};

}  // namespace art
