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

#include "Synet/SigmoidLayer.h"

namespace Synet
{
    template <class T, template<class> class A> void SigmoidLayer<T, A>::ForwardCpu(const std::vector<Synet::Tensor<T, A>*> & src, const std::vector<Synet::Tensor<T, A>*> & dst)
    {
        const Type * pSrc = src[0]->Data();
        Type * pDst = dst[0]->Data();
        size_t size = src[0]->Size();
        Type slope = this->Param().sigmoidLayer().slope();
        for (size_t i = 0; i < size; ++i)
            pDst[i] = Type(1.0) / (Type(1.0) + ::exp(-pSrc[i] * slope));
    }

    SYNET_CLASS_INSTANCE(SigmoidLayer);
}