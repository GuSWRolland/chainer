#include "xchainer/cuda/cuda_device.h"

#include <cmath>
#include <cstdint>

#include <cuda_runtime.h>

#include "xchainer/array.h"
#include "xchainer/cuda/cuda_runtime.h"
#include "xchainer/cuda/elementwise.cuh"
#include "xchainer/device.h"
#include "xchainer/dtype.h"

namespace xchainer {
namespace cuda {

namespace {

template <typename T>
struct ExpImpl {
    __device__ void operator()(int64_t /*i*/, T x, T& out) { out = std::exp(x); }
};

}  // namespace

void CudaDevice::Exp(const Array& x, const Array& out) {
    CheckDevicesCompatible(x, out);
    CheckCudaError(cudaSetDevice(index()));
    VisitFloatingPointDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        Elementwise<const T, T>(ExpImpl<T>{}, x, out);
    });
}

namespace {

template <typename T>
struct LogImpl {
    __device__ void operator()(int64_t /*i*/, T x, T& out) { out = std::log(x); }
};

}  // namespace

void CudaDevice::Log(const Array& x, const Array& out) {
    CheckDevicesCompatible(x, out);
    CheckCudaError(cudaSetDevice(index()));
    VisitFloatingPointDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        Elementwise<const T, T>(LogImpl<T>{}, x, out);
    });
}

}  // namespace cuda
}  // namespace xchainer
