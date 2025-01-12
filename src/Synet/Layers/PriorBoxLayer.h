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
    template <class T> class PriorBoxLayer : public Synet::Layer<T>
    {
    public:
        typedef T Type;
        typedef Layer<T> Base;
        typedef typename Base::TensorPtrs TensorPtrs;

        PriorBoxLayer(const LayerParam & param)
            : Base(param)
        {
        }

        virtual void Reshape(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst)
        {
            const PriorBoxParam & param = this->Param().priorBox();
            _version = param.version();
            _minSizes = param.minSize();
            _flip = param.flip();
            _clip = param.clip();
            _offset = param.offset();
            _scaleAllSizes = param.scaleAllSizes();
            _aspectRatios.clear();
            _aspectRatios.push_back(1.0f);
            for (int i = 0; i < param.aspectRatio().size(); ++i)
            {
                float aspectRatio = param.aspectRatio()[i];
                bool alreadyExist = false;
                for (int j = 0; j < _aspectRatios.size(); ++j)
                {
                    if (::fabs(aspectRatio - _aspectRatios[j]) < 1e-6)
                    {
                        alreadyExist = true;
                        break;
                    }
                }
                if (!alreadyExist)
                {
                    _aspectRatios.push_back(aspectRatio);
                    if (_flip)
                        _aspectRatios.push_back(1.0f / aspectRatio);
                }
            }
            if (_scaleAllSizes) {
                _numPriors = _aspectRatios.size() * _minSizes.size();
            }
            else {
                _numPriors = _aspectRatios.size() + _minSizes.size() - 1;
            }
            if (param.maxSize().size() > 0)
            {
                assert(param.minSize().size() == param.maxSize().size());
                _maxSizes = param.maxSize();
                _numPriors += _maxSizes.size();
            }
            if (param.variance().size() > 1)
            {
                assert(param.variance().size() == 4);
                _variance = param.variance();
            }
            else if (param.variance().size() == 1)
                _variance.push_back(param.variance()[0]);
            else
                _variance.push_back(0.1f);
            if (param.imgSize().size() == 2)
            {
                _imgH = param.imgSize()[0];
                _imgW = param.imgSize()[1];
            }
            else if (param.imgSize().size() == 1)
            {
                _imgH = param.imgSize()[0];
                _imgW = param.imgSize()[0];
            }
            else
            {
                _imgH = 0;
                _imgW = 0;
            }
            _trans = src[0]->Format() == TensorFormatNhwc;
            size_t layerH = _trans ? src[0]->Axis(1) : src[0]->Axis(2);
            size_t layerW = _trans ? src[0]->Axis(2) : src[0]->Axis(3);
            if (param.step().size() == 2)
            {
                _stepH = param.step()[0];
                _stepW = param.step()[1];
            }
            else if (param.step().size() == 1)
            {
                _stepH = param.step()[0];
                _stepW = param.step()[0];
            }
            else if (_version == 2)
            {
                size_t dataH = _trans ? src[1]->Axis(1) : src[1]->Axis(2);
                size_t dataW = _trans ? src[1]->Axis(2) : src[1]->Axis(3);
                _stepH = float(dataH) / layerH;
                _stepW = float(dataW) / layerW;
            }
            else
            {
                _stepH = 0;
                _stepW = 0;
            }
            Shape shape(3);
            shape[0] = 1;
            shape[1] = 2;
            shape[2] = layerW * layerH * _numPriors * 4;
            dst[0]->Reshape(shape);
            CalculatePriorBox(src, dst);
        }

    protected:
        virtual void ForwardCpu(const TensorPtrs & src, const TensorPtrs & buf, const TensorPtrs & dst)
        {
        }

    private:
        void CalculatePriorBox(const TensorPtrs & src, const TensorPtrs & dst)
        {
            size_t layerH = _trans ? src[0]->Axis(1) : src[0]->Axis(2);
            size_t layerW = _trans ? src[0]->Axis(2) : src[0]->Axis(3);
            size_t imgW, imgH;
            if (_imgH == 0 || _imgW == 0)
            {
                imgH = _trans ? src[1]->Axis(1) : src[1]->Axis(2);
                imgW = _trans ? src[1]->Axis(2) : src[1]->Axis(3);
            }
            else
            {
                imgH = _imgH;
                imgW = _imgW;
            }
            float stepW, stepH;
            if (_stepW == 0 || _stepH == 0)
            {
                stepH = float(imgH) / layerH;
                stepW = float(imgW) / layerW;
            }
            else
            {
                stepH = _stepH;
                stepW = _stepW;
            }
            Type * pDst = dst[0]->CpuData();
            size_t dim = layerH * layerW * _numPriors * 4;
            size_t index = 0;
            for (size_t h = 0; h < layerH; ++h)
            {
                for (size_t w = 0; w < layerW; ++w)
                {
                    float centerX, centerY;
                    if (_version == 2)
                    {
                        centerX = (w + 0.5f) * _stepW;
                        centerY = (h + 0.5f) * _stepH;
                    }
                    else
                    {
                        centerX = (w + _offset) * stepW;
                        centerY = (h + _offset) * stepH;
                    }
                    float boxW, boxH;
                    for (size_t s = 0; s < _minSizes.size(); ++s)
                    {
                        int minS = (int)_minSizes[s];
                        boxW = boxH = _version == 2 ? _minSizes[s] : (float)minS;
                        pDst[index++] = (centerX - boxW / 2.0f) / imgW;
                        pDst[index++] = (centerY - boxH / 2.0f) / imgH;
                        pDst[index++] = (centerX + boxW / 2.0f) / imgW;
                        pDst[index++] = (centerY + boxH / 2.0f) / imgH;
                        if (_version == 2)
                        {

                            if (_scaleAllSizes || (!_scaleAllSizes && (s == _minSizes.size() - 1)))
                            {
                                size_t sm = _scaleAllSizes ? s : 0;
                                for (size_t r = 0; r < _aspectRatios.size(); ++r)
                                {
                                    float ar = _aspectRatios[r];
                                    if (::fabs(ar - 1.) < 1e-6)
                                        continue;
                                    boxW = _minSizes[sm] * sqrt(ar);
                                    boxH = _minSizes[sm] / sqrt(ar);
                                    pDst[index++] = (centerX - boxW / 2.0f) / imgW;
                                    pDst[index++] = (centerY - boxH / 2.0f) / imgH;
                                    pDst[index++] = (centerX + boxW / 2.0f) / imgW;
                                    pDst[index++] = (centerY + boxH / 2.0f) / imgH;
                                }
                            }
                            if (_maxSizes.size() > s)
                            {
                                boxW = boxH = (float)::sqrt(_minSizes[s] * _maxSizes[s]);
                                pDst[index++] = (centerX - boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY - boxH / 2.0f) / imgH;
                                pDst[index++] = (centerX + boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY + boxH / 2.0f) / imgH;
                            }
                        }
                        else
                        {
                            if (_maxSizes.size() > 0)
                            {
                                int maxS = (int)_maxSizes[s];
                                boxW = boxH = (float)::sqrt(minS * maxS);
                                pDst[index++] = (centerX - boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY - boxH / 2.0f) / imgH;
                                pDst[index++] = (centerX + boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY + boxH / 2.0f) / imgH;
                            }
                            for (size_t r = 0; r < _aspectRatios.size(); ++r)
                            {
                                float ar = _aspectRatios[r];
                                if (::fabs(ar - 1.) < 1e-6)
                                    continue;
                                boxW = minS * sqrt(ar);
                                boxH = minS / sqrt(ar);
                                pDst[index++] = (centerX - boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY - boxH / 2.0f) / imgH;
                                pDst[index++] = (centerX + boxW / 2.0f) / imgW;
                                pDst[index++] = (centerY + boxH / 2.0f) / imgH;
                            }
                        }
                    }
                }
            }
            if (_clip)
            {
                for (size_t d = 0; d < dim; ++d)
                    pDst[d] = std::min<Type>(std::max<Type>(pDst[d], Type(0)), Type(1));
            }
            pDst += dst[0]->Size(2);
            if (_variance.size() == 1)
                CpuSet(dim, Type(_variance[0]), pDst);
            else
            {
                size_t offset = 0;
                for (size_t h = 0; h < layerH; ++h)
                    for (size_t w = 0; w < layerW; ++w)
                        for (size_t i = 0; i < _numPriors; ++i)
                            for (size_t j = 0; j < 4; ++j)
                                pDst[offset++] = _variance[j];
            }
        }

        int _version;
        Floats _minSizes, _maxSizes, _aspectRatios, _variance;
        bool _flip, _clip, _scaleAllSizes, _trans;
        size_t _numPriors, _imgW, _imgH;
        float _stepW, _stepH, _offset;
    };
}