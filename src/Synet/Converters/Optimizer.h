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
#include "Synet/Params.h"

namespace Synet
{
    class Optimizer
    {
    public:

        bool Run(Synet::NetworkParam & network)
        {
            LayerParams merged;
            if (!Merge(network.layers(), merged))
                return false;
            network.layers() = merged;

            merged.clear();
            if (!Merge(network.layers(), merged))
                return false;
            network.layers() = merged;

            return true;
        }

    private:
        typedef std::vector<Synet::LayerParam> LayerParams;
        typedef std::pair<String, String> Change;
        typedef std::vector<Change> Changes;

        bool Merge(const LayerParams & src, LayerParams & dst)
        {
            Changes changes;
            for (size_t i = 0; i < src.size(); ++i)
            {
                if (MergeConvolutionAndActivation(src, i, dst, changes))
                    continue;
                if (MergeThreeConvolutions(src, i, dst, changes))
                    continue;
                if (MergeSoftmax(src, i, dst, changes))
                    continue;
                if (MergeFused0(src, i, dst, changes))
                    continue;
                if (MergeFused1(src, i, dst, changes))
                    continue;
                if (MergeFused2(src, i, dst, changes))
                    continue;
                if (MergeFused3(src, i, dst, changes))
                    continue;
                if (MergeFused4(src, i, dst, changes))
                    continue;
                if (MergeFused5(src, i, dst, changes))
                    continue;
                if (MergeFused6(src, i, dst, changes))
                    continue;
                if (MergeFused7(src, i, dst, changes))
                    continue;
                dst.push_back(src[i]);
            }
            for (size_t k = 0; k < changes.size(); ++k)
            {
                for (size_t i = 0; i < dst.size(); ++i)
                {
                    for (size_t j = 0; j < dst[i].src().size(); ++j)
                    {
                        if (dst[i].src()[j] == changes[k].first)
                            dst[i].src()[j] = changes[k].second;
                    }
                }            
            }   
            return true;
        }

        bool MergeConvolutionAndActivation(const LayerParams & src, size_t index, LayerParams & dst, Changes & changes)
        {
            if (index == 0)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution)
                return false;
            if (src[index].src().size() != 1 || src[index].src()[0] != src[index - 1].name())
                return false;
            if (src[index].dst()[0] != src[index - 1].name())
            {
                for (size_t i = index + 1; i < src.size(); ++i)
                {
                    for (size_t j = 0; j < src[i].src().size(); ++j)
                    {
                        if (src[i].src()[j] == src[index - 1].name())
                            return false;
                    }
                }
            }
            if (src[index].type() == LayerTypeRestrictRange)
            {
                dst.back().convolution().activationType() = ActivationFunctionTypeRestrictRange;
                dst.back().convolution().activationParam0() = src[index].restrictRange().lower();
                dst.back().convolution().activationParam1() = src[index].restrictRange().upper();
                changes.push_back(Change(src[index].name(), src[index - 1].name()));
                return true;
            }
            if (src[index].type() == LayerTypeRelu)
            {
                dst.back().convolution().activationType() = src[index].relu().negativeSlope() == 0.0f ? ActivationFunctionTypeRelu : ActivationFunctionTypeLeakyRelu;
                dst.back().convolution().activationParam0() = src[index].relu().negativeSlope();
                changes.push_back(Change(src[index].name(), src[index - 1].name()));
                return true;
            }
            if (src[index].type() == LayerTypePrelu)
            {
                dst.back().convolution().activationType() = ActivationFunctionTypePrelu;
                dst.back().weight().push_back(src[index].weight()[0]);
                changes.push_back(Change(src[index].name(), src[index - 1].name()));
                return true;
            }
            return false;
        }

        bool MergeThreeConvolutions(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (src.size() < index + 3)
                return false;
            const LayerParam & l0 = src[index + 0];
            const Shape & k0 = l0.convolution().kernel();
            const LayerParam & l1 = src[index + 1];
            const Shape & k1 = l1.convolution().kernel();
            const LayerParam & l2 = src[index + 2];
            const Shape & k2 = l2.convolution().kernel();
            if (l0.type() != LayerTypeConvolution || l1.type() != LayerTypeConvolution || 
                l2.type() != LayerTypeConvolution || l1.src()[0] != l0.name() || l2.src()[0] != l1.name())
                return false;
            if (l0.weight()[0].format() != TensorFormatNhwc)
                return false;
            if (k0.size() < 2 || (k0[0] != k0[1] || (k0[0] != 1 && k0[0] != 3)))
                return false;
            if (l1.convolution().outputNum() != l1.convolution().group())
                return false;
            if (k1.size() < 2 || k1[0] != 3 || k1[1] != 3)
                return false;
            if (k2.size() < 2 || k2[0] != 1 || k2[1] != 1)
                return false;
            if (InsideLink(src, index, 3))
                return false;
            if (l1.convolution().outputNum() < l2.convolution().outputNum())
                return false;
            LayerParam layer;
            layer.type() = LayerTypeMergedConvolution;
            layer.name() = l2.name();
            layer.src() = l0.src();
            layer.dst().push_back(layer.name());
            for (size_t l = 0; l < 3; ++l)
                for(size_t i = 0; i < src[index + l].weight().size(); ++i)
                    layer.weight().push_back(src[index + l].weight()[i]);
            layer.mergedConvolution().conv().push_back(l0.convolution());
            layer.mergedConvolution().conv().push_back(l1.convolution());
            layer.mergedConvolution().conv().push_back(l2.convolution());
            dst.push_back(layer);
            index += 2;
            return true;
        }

        bool MergeSoftmax(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 5)
                return false;
            if (src[index + 0].type() != LayerTypeReduction || src[index + 0].reduction().type() != ReductionTypeMax ||
                src[index + 0].reduction().axis().size() != 1)
                return false;
            if (src[index + 1].type() != LayerTypeBinaryOperation || src[index + 1].binaryOperation().type() != BinaryOperationTypeSub ||
                src[index + 1].src()[0] != src[index + 0].src()[0] || src[index + 1].src()[1] != src[index + 0].name())
                return false;
            if (src[index + 2].type() != LayerTypeUnaryOperation || src[index + 2].unaryOperation().type() != UnaryOperationTypeExp ||
                src[index + 2].src()[0] != src[index + 1].name())
                return false;
            if (src[index + 3].type() != LayerTypeReduction || src[index + 3].reduction().type() != ReductionTypeSum ||
                src[index + 3].reduction().axis() != src[index + 0].reduction().axis() || src[index + 3].src()[0] != src[index + 2].name())
                return false;
            if (src[index + 4].type() != LayerTypeBinaryOperation || src[index + 4].binaryOperation().type() != BinaryOperationTypeDiv ||
                src[index + 4].src()[0] != src[index + 2].name() || src[index + 4].src()[1] != src[index + 3].name())
                return false;
            for (size_t i = index + 5; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = 0; k < 4; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeSoftmax;
            layer.name() = src[index + 4].name();
            layer.src().push_back(src[index + 0].src()[0]);
            layer.dst().push_back(layer.name());
            layer.softmax().axis() = src[index + 0].reduction().axis()[0];
            dst.push_back(layer);
            index += 4;
            return true;
        }

        bool MergeFused0(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 6)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || !src[index - 1].convolution().biasTerm() || 
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeRelu || src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeUnaryOperation || src[index + 1].unaryOperation().type() != UnaryOperationTypeAbs || 
                src[index + 1].src()[0] != src[index - 1].name())
                return false;
            if (!IsSub(src[index + 2]) || src[index + 2].src() != Strings({ src[index - 1].name(), src[index + 1].name() }))
                return false;
            if (src[index + 3].type() != LayerTypeScale || src[index + 3].scale().biasTerm() || src[index + 3].src()[0] != src[index + 2].name())
                return false;
            if (src[index + 4].type() != LayerTypeScale || src[index + 4].scale().biasTerm() || src[index + 4].src()[0] != src[index + 3].name())
                return false;
            if (src[index + 5].type() != LayerTypeEltwise || src[index + 5].eltwise().operation() != EltwiseOperationTypeSum || 
                !src[index + 5].eltwise().coefficients().empty() || src[index + 5].src() != Strings({ src[index + 0].name(), src[index + 4].name() }))
                return false;
            for (size_t i = index + 6; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 5; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 5].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst().push_back(layer.name());
            layer.fused().type() = 0;
            layer.weight().push_back(src[index - 1].weight()[1]);
            layer.weight().push_back(src[index + 3].weight()[0]);
            layer.weight().push_back(src[index + 4].weight()[0]);
            dst.back().weight().resize(1);
            dst.back().convolution().biasTerm() = false;
            dst.push_back(layer);
            index += 5;
            return true;
        }

        bool MergeFused1(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 5)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || !src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeRelu || src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeScale || src[index + 1].scale().axis() != 0 || !src[index + 1].scale().biasTerm() ||
                src[index + 1].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu || src[index + 2].src()[0] != src[index + 1].name())
                return false;
            if (src[index + 3].type() != LayerTypeScale || !src[index + 3].scale().biasTerm() || src[index + 3].src()[0] != src[index + 2].name())
                return false;
            if (src[index + 4].type() != LayerTypeEltwise || src[index + 4].eltwise().operation() != EltwiseOperationTypeSum ||
                !src[index + 4].eltwise().coefficients().empty() || src[index + 4].src() != Strings({ src[index + 0].name(), src[index + 3].name() }))
                return false;
            for (size_t i = index + 5; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 4; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 4].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst().push_back(layer.name());
            layer.fused().type() = 1;
            layer.weight().push_back(src[index - 1].weight()[1]);
            layer.weight().push_back(src[index + 1].weight()[0]);
            layer.weight().push_back(src[index + 1].weight()[1]);
            layer.weight().push_back(src[index + 3].weight()[0]);
            layer.weight().push_back(src[index + 3].weight()[1]);
            changes.push_back(Change(layer.dst()[0], layer.src()[0]));
            layer.dst()[0] = layer.src()[0];
            dst.back().weight().resize(1);
            dst.back().convolution().biasTerm() = false;
            dst.push_back(layer);
            index += 4;
            return true;
        }

        bool MergeFused2(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 2)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeBatchNorm || !src[index + 0].batchNorm().useGlobalStats() || !src[index + 0].batchNorm().yoloCompatible() || 
                src[index + 0].src()[0] != src[index - 1].name() || src[index + 0].dst()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeScale || !src[index + 1].scale().biasTerm() || src[index + 1].scale().axis() != 1 ||
                src[index + 1].src()[0] != src[index - 1].name() || src[index + 1].dst()[0] != src[index - 1].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu || 
                src[index + 2].src()[0] != src[index - 1].name() || src[index + 2].dst()[0] != src[index - 1].name())
                return false;
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 2].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst() = src[index + 2].dst();
            layer.fused().type() = 2;
            layer.fused().floats().resize(2);
            layer.fused().floats()[0] = src[index + 0].batchNorm().eps();
            layer.fused().floats()[1] = src[index + 2].relu().negativeSlope();
            layer.weight().push_back(src[index + 0].weight()[0]);
            layer.weight().push_back(src[index + 0].weight()[1]);
            layer.weight().push_back(src[index + 1].weight()[0]);
            layer.weight().push_back(src[index + 1].weight()[1]);
            dst.push_back(layer);
            index += 2;
            return true;
        }

        bool MergeFused3(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 6)
                return false;
            if ((src[index - 1].type() != LayerTypeConvolution || !src[index - 1].convolution().biasTerm() || 
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity) &&
                (src[index - 1].type() != LayerTypeInnerProduct || !src[index - 1].innerProduct().biasTerm()))
                return false;
            if (src[index + 0].type() != LayerTypeRelu || src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeUnaryOperation || src[index + 1].unaryOperation().type() != UnaryOperationTypeNeg ||
                src[index + 1].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu || src[index + 2].src()[0] != src[index + 1].name())
                return false;
            if (src[index + 3].type() != LayerTypeUnaryOperation || src[index + 3].unaryOperation().type() != UnaryOperationTypeNeg ||
                src[index + 3].src()[0] != src[index + 2].name())
                return false;
            if (src[index + 4].type() != LayerTypeScale || src[index + 4].scale().biasTerm() || src[index + 4].src()[0] != src[index + 3].name())
                return false;
            if (src[index + 5].type() != LayerTypeEltwise || src[index + 5].eltwise().operation() != EltwiseOperationTypeSum ||
                !src[index + 5].eltwise().coefficients().empty() || src[index + 5].src() != Strings({ src[index + 0].name(), src[index + 4].name() }))
                return false;
            for (size_t i = index + 6; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 5; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            if (dst.back().type() == LayerTypeConvolution)
            {
                dst.back().name() = src[index + 5].name();
                dst.back().dst().back() = dst.back().name();
                dst.back().convolution().activationType() = ActivationFunctionTypePrelu;
                dst.back().weight().push_back(src[index + 4].weight()[0]);
            }
            else
            {
                LayerParam layer;
                layer.type() = LayerTypeFused;
                layer.name() = src[index + 5].name();
                layer.src().push_back(src[index - 1].name());
                layer.dst().push_back(layer.name());
                layer.fused().type() = 3;
                layer.weight().push_back(src[index - 1].weight()[1]);
                layer.weight().push_back(src[index + 4].weight()[0]);
                dst.back().weight().resize(1);
                dst.back().innerProduct().biasTerm() = false;
                dst.push_back(layer);
            }
            index += 5;
            return true;
        }

        bool MergeFused4(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 3)
                return false;
            if ((src[index - 1].type() != LayerTypeConvolution || !src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity))
                return false;
            if (src[index + 0].type() != LayerTypeScale || !src[index + 0].scale().biasTerm() || src[index + 0].src()[0] != src[index - 1].name() ||
                src[index + 0].weight()[0].dim()[0] != 1 || src[index + 0].weight()[1].dim()[0] != 1)
                return false;
            if (src[index + 1].type() != LayerTypeConcat || src[index + 1].src().size() != 2 ||
                src[index + 1].src()[0] != src[index - 1].name() || src[index + 1].src()[1] != src[index + 0].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu || src[index + 2].src()[0] != src[index + 1].name())
                return false;
            for (size_t i = index + 3; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 2; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 2].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst().push_back(layer.name());
            layer.fused().type() = 4;
            layer.weight().push_back(src[index - 1].weight()[1]);
            layer.weight().push_back(src[index + 0].weight()[0]);
            layer.weight().push_back(src[index + 0].weight()[1]);
            dst.back().weight().resize(1);
            dst.back().convolution().biasTerm() = false;
            dst.push_back(layer);
            index += 2;
            return true;
        }

        bool MergeFused5(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 3)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeScale || !src[index + 0].scale().biasTerm() || src[index + 0].scale().axis() != 1 ||
                src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeScale || !src[index + 1].scale().biasTerm() || src[index + 1].scale().axis() != 1 ||
                src[index + 1].src()[0] != src[index - 0].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu ||
                src[index + 2].src()[0] != src[index + 1].name())
                return false;
            for (size_t i = index + 3; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 2; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 2].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst() = src[index + 2].dst();
            layer.fused().type() = 5;
            layer.weight().push_back(src[index + 0].weight()[0]);
            layer.weight().push_back(src[index + 0].weight()[1]);
            layer.weight().push_back(src[index + 1].weight()[0]);
            layer.weight().push_back(src[index + 1].weight()[1]);
            changes.push_back(Change(layer.dst()[0], layer.src()[0]));
            layer.dst()[0] = layer.src()[0];
            dst.push_back(layer);
            index += 2;
            return true;
        }

        bool MergeFused6(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 2)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeScale || !src[index + 0].scale().biasTerm() || src[index + 0].scale().axis() != 1 ||
                src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypeRelu ||
                src[index + 1].src()[0] != src[index + 0].name())
                return false;
            for (size_t i = index + 2; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 1; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 1].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst() = src[index + 1].dst();
            layer.fused().type() = 6;
            layer.weight().push_back(src[index + 0].weight()[0]);
            layer.weight().push_back(src[index + 0].weight()[1]);
            changes.push_back(Change(layer.dst()[0], layer.src()[0]));
            layer.dst()[0] = layer.src()[0];
            dst.push_back(layer);
            index += 1;
            return true;
        }

        bool MergeFused7(const LayerParams & src, size_t & index, LayerParams & dst, Changes & changes)
        {
            if (index == 0 || src.size() < index + 5)
                return false;
            if (src[index - 1].type() != LayerTypeConvolution || !src[index - 1].convolution().biasTerm() ||
                src[index - 1].convolution().activationType() != ActivationFunctionTypeIdentity)
                return false;
            if (src[index + 0].type() != LayerTypeRelu || src[index + 0].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 1].type() != LayerTypePower || src[index + 1].power().power() != 1.0f || src[index + 1].power().scale() != -1.0f ||
                src[index + 1].power().shift() != 0.0f || src[index + 1].src()[0] != src[index - 1].name())
                return false;
            if (src[index + 2].type() != LayerTypeRelu || src[index + 2].src()[0] != src[index + 1].name())
                return false;
            if (src[index + 3].type() != LayerTypeScale || !src[index + 3].scale().biasTerm() || src[index + 3].src()[0] != src[index + 2].name())
                return false;
            if (src[index + 4].type() != LayerTypeEltwise || src[index + 4].eltwise().operation() != EltwiseOperationTypeSum ||
                !src[index + 4].eltwise().coefficients().empty() || src[index + 4].src() != Strings({ src[index + 0].name(), src[index + 3].name() }))
                return false;
            for (size_t i = index + 5; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (ptrdiff_t k = -1; k < 4; ++k)
                    {
                        if (src[i].src()[j] == src[index + k].name())
                            return false;
                    }
                }
            }
            LayerParam layer;
            layer.type() = LayerTypeFused;
            layer.name() = src[index + 4].name();
            layer.src().push_back(src[index - 1].name());
            layer.dst().push_back(layer.name());
            layer.fused().type() = 7;
            layer.weight().push_back(src[index - 1].weight()[1]);
            layer.weight().push_back(src[index + 3].weight()[0]);
            layer.weight().push_back(src[index + 3].weight()[1]);
            changes.push_back(Change(layer.dst()[0], layer.src()[0]));
            layer.dst()[0] = layer.src()[0];
            dst.back().weight().resize(1);
            dst.back().convolution().biasTerm() = false;
            dst.push_back(layer);
            index += 4;
            return true;
        }

        bool IsSub(const LayerParam & layer) const
        {
            if (layer.type() == LayerTypeEltwise && layer.eltwise().operation() == EltwiseOperationTypeSum && layer.eltwise().coefficients() == Floats({ 1.0f, -1.0f }))
                return true;
            if (layer.type() == LayerTypeBinaryOperation && layer.binaryOperation().type() == BinaryOperationTypeSub)
                return true;
            return false;
        }

        bool InsideLink(const LayerParams & src, size_t start, size_t count) const
        {
            for (size_t i = start + count; i < src.size(); ++i)
            {
                for (size_t j = 0; j < src[i].src().size(); ++j)
                {
                    for (size_t k = 0; k < count - 1; ++k)
                    {
                        if (src[i].src()[j] == src[start + k].name())
                            return true;
                    }
                }
            }
            return false;
        }
    };
}