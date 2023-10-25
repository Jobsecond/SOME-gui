#include "OrtLoader.h"

#include <QString>

#ifdef ORT_API_MANUAL_INIT
#include <QLibrary>

#include <onnxruntime_cxx_api.h>

#define RETURN_FAIL_WITH_MSG(outPtr, msg)         \
        {                                         \
            if (outPtr) {                         \
                *(outPtr) = msg;                  \
            }                                     \
            return false;                         \
        }
#endif

bool InitOrtLibrary(QString *outErrString) {
#ifdef ORT_API_MANUAL_INIT
    typedef const OrtApiBase *(*OrtGetApiBaseFn)();
    OrtGetApiBaseFn funcPtr;

    QLibrary ort("onnxruntime");
    if (!ort.load()) {
        RETURN_FAIL_WITH_MSG(outErrString, ort.errorString())
    }
    funcPtr = reinterpret_cast<OrtGetApiBaseFn>(ort.resolve("OrtGetApiBase"));
    if (!funcPtr) {
        RETURN_FAIL_WITH_MSG(outErrString, "Could not resolve \"OrtGetApiBase\" symbol.")
    }
    const auto ortApiBase = funcPtr();
    if (!ortApiBase) {
        RETURN_FAIL_WITH_MSG(outErrString, "Could not get OrtApiBase")
    }
    auto ortApi = ortApiBase->GetApi(ORT_API_VERSION);
    if (!ortApi) {
        RETURN_FAIL_WITH_MSG(outErrString, "Could not get ONNX Runtime API")
    }

    Ort::InitApi(ortApi);
#endif
    return true;
}
