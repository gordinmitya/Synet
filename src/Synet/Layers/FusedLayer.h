/*
* Synet Framework (http://github.com/ermig1979/Synet).
*
* Copyright (c) 2018-2018 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include "Synet/Common.h"
#include "Synet/Layer.h"

namespace Synet
{
    namespace Detail
    {
        template <class T> SYNET_INLINE T FusedLayerForward0(T x, T s)
        {
            return (x - (T)::abs(x))*s + std::max((T)0, x);
        }

        template <class T> void FusedLayerForwardCpu0(const T * src, const T * bias, const T * scale, size_t count, size_t size, T * dst, int trans)
        {
            if ((trans || size == 1) && count != 1)
            {
                for (size_t j = 0; j < size; ++j)
                {
                    for (size_t i = 0; i < count; ++i)
                        dst[i] = FusedLayerForward0(src[i] + bias[i], scale[i]);
                    src += count;
                    dst += count;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    for (size_t j = 0; j < size; ++j)
                        dst[j] = FusedLayerForward0(src[j] + bias[i], scale[i]);
                    src += size;
                    dst += size;
                }
            }
        }

        template <class T> SYNET_INLINE T FusedLayerForward1(T x, T s, T b)
        {
            return std::max((T)0, -x)*s + b + std::max((T)0, x);
        }

        template <class T> void FusedLayerForwardCpu1(const T * src, const T * bias0, const T * scale1, const T * bias1, size_t count, size_t size, T * dst, int trans)
        {
            if ((trans || size == 1) && count != 1)
            {
                for (size_t j = 0; j < size; ++j)
                {
                    for (size_t i = 0; i < count; ++i)
                        dst[i] = FusedLayerForward1(src[i] + bias0[i], scale1[i], bias1[i]);
                    src += count;
                    dst += count;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    for (size_t j = 0; j < size; ++j)
                        dst[j] = FusedLayerForward1(src[j] + bias0[i], scale1[i], bias1[i]);
                    src += size;
                    dst += size;
                }
            }
        }

        template <class T> SYNET_INLINE T FusedLayerForward2(T src, T scale, T bias, T slope)
        {
            T x = src * scale + bias;
            return std::max((T)0, x) + std::min((T)0, x)*slope;
        }

        template <class T> void FusedLayerForwardCpu2(const T * src, const T * scale, const T * bias, size_t count, size_t size, T slope, T * dst, int trans)
        {
            if ((trans || size == 1) && count != 1)
            {
                for (size_t j = 0; j < size; ++j)
                {
                    for (size_t i = 0; i < count; ++i)
                        dst[i] = FusedLayerForward2(src[i], scale[i], bias[i], slope[0]);
                    src += count;
                    dst += count;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    for (size_t j = 0; j < size; ++j)
                        dst[j] = FusedLayerForward2(src[j], scale[i], bias[i], slope[0]);
                    src += size;
                    dst += size;
                }
            }
        }

        template <class T> SYNET_INLINE T FusedLayerForward3(T x, T s)
        {
            return std::max((T)0, x) + std::min(x, (T)0) * s;
        }        
        
        template <class T> void FusedLayerForwardCpu3(const T * src, const T * bias, const T * scale, size_t count, size_t size, T * dst, int trans)
        {
            if ((trans || size == 1) && count != 1)
            {
                for (size_t j = 0; j < size; ++j)
                {
                    for (size_t i = 0; i < count; ++i)
                        dst[i] = FusedLayerForward3(src[i] + bias[i], scale[i]);
                    src += count;
                    dst += count;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    for (size_t j = 0; j < size; ++j)
                        dst[j] = FusedLayerForward3(src[j] + bias[i], scale[i]);
                    src += size;
                    dst += size;
                }
            }
        }

        template <class T> void FusedLayerForwardCpu4(const T * src, const T * bias0, const T * scale1, const T * bias1, size_t count, size_t size, T * dst, int trans)
        {
            if ((trans || size == 1) && count != 1)
            {
                for (size_t j = 0; j < size; ++j)
                {
                    for (size_t i = 0; i < count; ++i)
                    {
                        T x = src[i] + bias0[i];
                        dst[i] = std::max((T)0, x);
                        dst[i + count] = std::max((T)0, x*scale1[0] + bias1[0]);
                    }
                    src += count;
                    dst += 2 * count;
                }
            }
            else
            {
                T * dst0 = dst, * dst1 = dst + count*size;
                for (size_t i = 0; i < count; ++i)
                {
                    for (size_t j = 0; j < size; ++j)
                    {
                        T x = src[j] + bias0[i];
                        dst0[j] = std::max((T)0, x);
                        dst1[j] = std::max((T)0, x*scale1[0] + bias1[0]);
                    }
                    src += size;
                    dst0 += size;
                    dst1 += size;
                }
            }
        }

#ifdef SYNET_SIMD_LIBRARY_ENABLE
        template <> SYNET_INLINE void FusedLayerForwardCpu0<float>(const float * src, const float * bias, const float * scale, size_t count, size_t size, float * dst, int trans)
        {
            ::SimdSynetFusedLayerForward0(src, bias, scale, count, size, dst, (::SimdBool)trans);
        }

        template <> SYNET_INLINE void FusedLayerForwardCpu1<float>(const float * src, const float * bias0, const float * scale1, const float * bias1, size_t count, size_t size, float * dst, int trans)
        {
            ::SimdSynetFusedLayerForward1(src, bias0, scale1, bias1, count, size, dst, (::SimdBool)trans);
        }

        template <> SYNET_INLINE void FusedLayerForwardCpu2<float>(const float * src, const float * scale, const float * bias, size_t count, size_t size, float slope, float * dst, int trans)
        {
            ::SimdSynetFusedLayerForward2(src, scale, bias, count, size, &slope, dst, (::SimdBool)trans);
        }

        template <> SYNET_INLINE void FusedLayerForwardCpu3<float>(const float * src, const float * bias, const float * scale, size_t count, size_t size, float * dst, int trans)
        {
            ::SimdSynetFusedLayerForward3(src, bias, scale, count, size, dst, (::SimdBool)trans);
        }

        template <> SYNET_INLINE void FusedLayerForwardCpu4<float>(const float * src, const float * bias0, const float * scale1, const float * bias1, size_t count, size_t size, float * dst, int trans)
        {
            ::SimdSynetFusedLayerForward4(src, bias0, scale1, bias1, count, size, dst, (::SimdBool)trans);
        }
#endif
    }

    template <class T> class FusedLayer : public Synet::Layer<T>
    {
    public:
        typedef T Type;
        typedef Layer<T> Base;
        typedef typename Base::TensorPtrs TensorPtrs;

        FusedLayer(const LayerParam & param)
            : Base(param)
        {
        }

        virtual void Reshape(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst)
        {
            const FusedParam & fused = this->Param().fused();
            _type = fused.type();
            const Tensors & weight = this->Weight();
            switch (_type)
            {
            case 0:
            {
                assert(weight.size() == 3);
                _t0.bias.Share(weight[0]);
                _count = _t0.bias.Size();
                _t0.scale.Reshape(_t0.bias.Shape());
                assert(weight[1].Size() == _count || weight[1].Size() == 1);
                if (weight[1].Size() == _count)
                {
                    for (size_t i = 0; i < _count; ++i)
                        _t0.scale.CpuData()[i] = weight[1].CpuData()[i];
                }
                else
                {
                    for (size_t i = 0; i < _count; ++i)
                        _t0.scale.CpuData()[i] = weight[1].CpuData()[0];
                }
                assert(weight[2].Size() == _count || weight[2].Size() == 1);
                if (weight[2].Size() == _count)
                {
                    for (size_t i = 0; i < _count; ++i)
                        _t0.scale.CpuData()[i] *= weight[2].CpuData()[i];
                }
                else
                {
                    for (size_t i = 0; i < _count; ++i)
                        _t0.scale.CpuData()[i] *= weight[2].CpuData()[0];
                }
                break;
            }
            case 1:
            {
                assert(weight.size() == 5);
                _t1.bias0.Share(weight[0]);
                _count = _t1.bias0.Size();
                assert(weight[1].Size() == 1 && weight[1].CpuData()[0] == -1.0f);
                assert(weight[2].Size() == 1 && weight[2].CpuData()[0] == 0.0f);
                _t1.scale1.Share(weight[3]);
                _t1.bias1.Share(weight[4]);
                break;
            }
            case 2:
                assert(weight.size() == 4 && weight[0].Count() == 1);
                assert(weight[0].Shape() == weight[1].Shape() && weight[0].Shape() == weight[2].Shape() && weight[0].Shape() == weight[3].Shape());
                assert(fused.floats().size() == 2);
                _t2.scale.Reshape(weight[0].Shape());
                _t2.bias.Reshape(weight[0].Shape());
                _count = _t2.scale.Size();
                for (size_t i = 0; i < _count; ++i)
                {
                    Type eps = fused.floats()[0];
                    Type scale = Type(1) / (::sqrt(weight[1].CpuData()[i]) + eps);
                    Type bias = -weight[0].CpuData()[i] * scale;
                    _t2.scale.CpuData()[i] = scale*weight[2].CpuData()[i];
                    _t2.bias.CpuData()[i] = bias*weight[2].CpuData()[i] + weight[3].CpuData()[i];
                }
                _t2.slope = fused.floats()[1];
                break;
            case 3:
            {
                assert(weight.size() == 2 && weight[0].Size() == weight[1].Size());
                _t3.bias.Share(weight[0]);
                _count = _t3.bias.Size();
                _t3.scale.Share(weight[1]);
                break;
            }
            case 4:
            {
                assert(weight.size() == 3 && weight[1].Size() == 1 && weight[2].Size() == 1 && src[0]->Count() == 4);
                _t4.bias0.Share(weight[0]);
                _count = _t4.bias0.Size();
                _t4.scale1.Share(weight[1]);
                _t4.bias1.Share(weight[2]);
                break;
            }
            case 5:
                assert(weight.size() == 4);
                assert(weight[0].Shape() == weight[1].Shape() && weight[0].Shape() == weight[2].Shape() && weight[0].Shape() == weight[3].Shape());
                _t2.scale.Reshape(weight[0].Shape());
                _t2.bias.Reshape(weight[0].Shape());
                _count = _t2.scale.Size();
                for (size_t i = 0; i < _count; ++i)
                {
                    _t2.scale.CpuData()[i] = weight[0].CpuData()[i] * weight[2].CpuData()[i];
                    _t2.bias.CpuData()[i] = weight[1].CpuData()[i] * weight[2].CpuData()[i] + weight[3].CpuData()[i];
                }
                _t2.slope = Type(0);
                break;
            case 6:
                assert(weight.size() == 2);
                assert(weight[0].Shape() == weight[1].Shape());
                _t2.scale.Share(weight[0]);
                _t2.bias.Share(weight[1]);
                _count = _t2.scale.Size();
                _t2.slope = Type(0);
                break;
            case 7:
            {
                assert(weight.size() == 3);
                _t1.bias0.Share(weight[0]);
                _count = _t1.bias0.Size();
                _t1.scale1.Share(weight[1]);
                _t1.bias1.Share(weight[2]);
                break;
            }
            default:
                assert(0);
            }

            _trans = src[0]->Format() == TensorFormatNhwc;
            _num = src[0]->Size(0, fused.axis());
            _size = src[0]->Size() / _count / _num;
            assert(_num*_size*_count == src[0]->Size());
            Shape shape = src[0]->Shape();
            if (_type == 4)
                shape[_trans ? 3 : 1] *= 2;
            dst[0]->Reshape(shape, src[0]->Format());
            _srcStride = src[0]->Size(fused.axis());
            _dstStride = dst[0]->Size(fused.axis());
        }

    protected:
        virtual void ForwardCpu(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst)
        {
            SYNET_PERF_FUNC();
            ForwardCpu(src[0]->CpuData(), dst[0]->CpuData());
        }

        void ForwardCpu(const Type * src, Type * dst)
        {
#ifdef SYNET_SIZE_STATISTIC
            std::stringstream ss;
            ss << " t=" << _type << " c=" << _count << " s=" << _size;
            SYNET_PERF_BLOCK(ss.str().c_str());
#else
            SYNET_PERF_FUNC();
#endif
            for (size_t i = 0; i < _num; ++i)
            {
                switch (_type)
                {
                case 0:
                    Detail::FusedLayerForwardCpu0(src, _t0.bias.CpuData(), _t0.scale.CpuData(), _count, _size, dst, _trans);
                    break;
                case 1:
                case 7:
                    Detail::FusedLayerForwardCpu1(src, _t1.bias0.CpuData(), _t1.scale1.CpuData(), _t1.bias1.CpuData(), _count, _size, dst, _trans);
                    break;
                case 2:
                case 5:
                case 6:
                    Detail::FusedLayerForwardCpu2(src, _t2.scale.CpuData(), _t2.bias.CpuData(), _count, _size, _t2.slope, dst, _trans);
                    break;
                case 3:
                    Detail::FusedLayerForwardCpu3(src, _t3.bias.CpuData(), _t3.scale.CpuData(), _count, _size, dst, _trans);
                    break;
                case 4:
                    Detail::FusedLayerForwardCpu4(src, _t4.bias0.CpuData(), _t4.scale1.CpuData(), _t4.bias1.CpuData(), _count, _size, dst, _trans);
                    break;
                default:
                    assert(0);
                }
                src += _srcStride;
                dst += _dstStride;
            }
        }

    private:
        typedef typename Base::Tensor Tensor;
        typedef typename Base::Tensors Tensors;

        int _type, _trans;
        size_t _count, _size, _num, _srcStride, _dstStride;

        struct T0
        {
            Tensor bias, scale;
        } _t0;

        struct T1
        {
            Tensor bias0, scale1, bias1;
        } _t1;

        struct T2
        {
            Tensor scale, bias;
            Type slope;
        } _t2;

        struct T3
        {
            Tensor bias, scale;
        } _t3;

        struct T4
        {
            Tensor bias0, scale1, bias1;
        } _t4;
    };
}