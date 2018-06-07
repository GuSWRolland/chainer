#include "xchainer/native/native_device.h"

#include <cassert>
#include <cstdint>

#include "xchainer/array.h"
#include "xchainer/device.h"
#include "xchainer/dtype.h"
#include "xchainer/indexable_array.h"
#include "xchainer/indexer.h"
#include "xchainer/native/elementwise.h"
#include "xchainer/scalar.h"
#include "xchainer/shape.h"

namespace xchainer {
namespace native {

void NativeDevice::Fill(const Array& out, Scalar value) {
    VisitDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        struct Impl {
            void operator()(int64_t /*i*/, T& out) { out = value; }
            T value;
        };
        Elementwise<T>(Impl{static_cast<T>(value)}, out);
    });
}

void NativeDevice::Arange(Scalar start, Scalar step, const Array& out) {
    VisitDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        struct Impl {
            void operator()(int64_t i, T& out) { out = start + step * i; }
            T start;
            T step;
        };
        Elementwise<T>(Impl{static_cast<T>(start), static_cast<T>(step)}, out);
    });
}

void NativeDevice::Identity(const Array& out) {
    assert(out.ndim() == 2);
    assert(out.shape()[0] == out.shape()[1]);

    VisitDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        struct Impl {
            explicit Impl(int64_t n) : n_plus_one{n + 1} {}
            void operator()(int64_t i, T& out) { out = i % n_plus_one == 0 ? T{1} : T{0}; }
            int64_t n_plus_one;
        };
        Elementwise<T>(Impl{out.shape()[0]}, out);
    });
}

void NativeDevice::Eye(int64_t k, const Array& out) {
    VisitDtype(out.dtype(), [k, &out](auto pt) {
        using T = typename decltype(pt)::type;
        struct Impl {
            Impl(int64_t m, int64_t k) : start{k < 0 ? -k * m : k}, stop{m * (m - k)}, step{m + 1} {}
            void operator()(int64_t i, T& out) { out = start <= i && i < stop && (i - start) % step == 0 ? T{1} : T{0}; }
            int64_t start;
            int64_t stop;
            int64_t step;
        };
        Elementwise<T>(Impl{out.shape()[1], k}, out);
    });
}

void NativeDevice::Diagflat(const Array& v, int64_t k, const Array& out) {
    assert(v.ndim() == 1);
    assert(out.ndim() == 2);

    VisitDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;

        // Start indices for the 2-D array axes with applied offset k.
        int64_t row_start{0};
        int64_t col_start{0};

        if (k >= 0) {
            col_start += k;
        } else {
            row_start -= k;
        }

        IndexableArray<const T, 1> v_iarray{v};
        IndexableArray<T, 2> out_iarray{out};
        Indexer<1> v_indexer{v.shape()};
        Indexer<1> out_rows_indexer{Shape{out.shape()[0]}};
        Indexer<1> out_cols_indexer{Shape{out.shape()[1]}};
        Indexer<2> out_indexer{out.shape()};

        // Initialize all elements to 0 first instead of conditionally filling in the diagonal.
        for (auto out_it = out_indexer.It(0); out_it; ++out_it) {
            out_iarray[out_it] = T{0};
        }

        for (auto v_it = v_indexer.It(0); v_it; ++v_it) {
            auto out_rows_it = out_rows_indexer.It(row_start + v_it.raw_index());
            auto out_cols_it = out_cols_indexer.It(col_start + v_it.raw_index());
            auto out_it = out_indexer.At(out_rows_it, out_cols_it);
            out_iarray[out_it] = v_iarray[v_it];
        }
    });
}

void NativeDevice::Linspace(double start, double stop, const Array& out) {
    assert(out.ndim() == 1);
    assert(out.shape()[0] > 0);

    VisitDtype(out.dtype(), [&](auto pt) {
        using T = typename decltype(pt)::type;
        struct Impl {
            void operator()(int64_t i, T& out) {
                double value = n == 1 ? start : (start * (n - 1 - i) + stop * i) / (n - 1);
                out = static_cast<T>(value);
            }
            int64_t n;
            double start;
            double stop;
        };

        int64_t n = out.shape()[0];
        Elementwise<T>(Impl{n, start, stop}, out);
    });
}

}  // namespace native
}  // namespace xchainer
