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
#include "Synet/Tensor.h"
#include "Synet/Params.h"

namespace Synet
{
    template <class T> class Layer
    {
    public:
        typedef T Type;
        typedef Synet::Tensor<T> Tensor;
        typedef std::vector<Tensor> Tensors;
        typedef std::vector<Tensor*> TensorPtrs;
        typedef std::shared_ptr<Layer> LayerSharedPtr;
        typedef std::vector<LayerSharedPtr> LayerSharedPtrs;

        Layer(const LayerParam & param)
            : _param(param)
        {
            //_weight.resize(_param.weight().size());
            //for (size_t i = 0; i < _weight.size(); ++i)
            //{
            //    if(_param.weight()[i].offset() < 0 && _param.weight()[i].size() < 0)
            //        _weight[i].Reshape(_param.weight()[i].dim(), Type(), _param.weight()[i].format());
            //}
        }

        virtual ~Layer()
        {
        }

        const LayerParam & Param() const 
        { 
            return _param; 
        }

        const Tensors & Weight() const 
        { 
            return _weight; 
        }

        virtual size_t MemoryUsage() const
        {
            return 0;
        }

        virtual void CompactWeight()
        {
        }

        virtual void Reshape(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst) = 0;

        inline void Forward(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst)
        {
            ForwardCpu(src, buf, dst);
        }

        bool Load(std::istream & is, const LayerSharedPtrs & layers)
        {
            _weight.resize(_param.weight().size());
            for (size_t i = 0; i < _weight.size(); ++i)
            {
                const WeightParam & param = _param.weight()[i];
                Tensor & tensor = _weight[i];
                ptrdiff_t offset = param.offset();
                ptrdiff_t size = param.size();
                if (offset < 0 && size < 0)
                {
                    tensor.Reshape(param.dim(), Type(), param.format());
                    if (!is.read((char*)tensor.CpuData(), tensor.Size() * sizeof(T)))
                        return false;
                }
                else
                {
                    bool unique = true;
                    for (size_t j = 0; j < layers.size() && unique; ++j)
                    {
                        if (layers[j].get() == this)
                            break;
                        for (size_t k = 0; k < layers[j]->Param().weight().size() && unique; ++k)
                        {
                            if (layers[j]->Param().weight()[k].offset() == offset)
                            {
                                tensor.Share(layers[j]->Weight()[k]);
                                unique = false;
                            }
                        }
                    }
                    if (unique)
                    {
                        tensor.Reshape(param.dim(), Type(), param.format());
                        is.seekg(offset, std::ios::beg);
                        if (!is.read((char*)tensor.CpuData(), size))
                            return false;
                    }
                }
            }
            return true;
        }

        bool Load(const char * & data, size_t & size, const LayerSharedPtrs & layers)
        {
            _weight.resize(_param.weight().size());
            for (size_t i = 0; i < _weight.size(); ++i)
            {
                const WeightParam & param = _param.weight()[i];
                Tensor & tensor = _weight[i];
                ptrdiff_t offset = param.offset();
                ptrdiff_t length = param.size();
                if (offset < 0 && length < 0)
                {
                    tensor.Reshape(param.dim(), Type(), param.format());
                    length = tensor.Size() * sizeof(T);
                    if (length > size)
                        return false;
                    memcpy((char*)tensor.CpuData(), data, length);
                    data += length;
                    size -= length;
                }
                else
                {
                    bool unique = true;
                    for (size_t j = 0; j < layers.size() && unique; ++j)
                    {
                        if (layers[j].get() == this)
                            break;
                        for (size_t k = 0; k < layers[j]->Param().weight().size() && unique; ++k)
                        {
                            if (layers[j]->Param().weight()[k].offset() == offset)
                            {
                                tensor.Share(layers[j]->Weight()[k]);
                                unique = false;
                            }
                        }
                    }
                    if (unique)
                    {
                        if (offset + length > size)
                            return false;
                        tensor.Reshape(param.dim(), Type(), param.format());
                        memcpy((char*)tensor.CpuData(), data + offset, length);
                    }
                }
            }
            return true;
        }

    protected:
        virtual void ForwardCpu(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst) = 0;

    private:
        const LayerParam & _param;
        Tensors _weight;
    };
}