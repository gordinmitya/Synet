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

#if defined(SYNET_YOLO_ENABLE)

#ifdef SYNET_YOLO_PATH
#define SYNET_INCLUDE_YOLO(file) SYNET_INCLUDE(SYNET_YOLO_PATH, file)
#else
#error yolo library path is not defined!
#endif

#ifdef _MSC_VER
#define _TIMESPEC_DEFINED
#pragma warning (push)
#pragma warning (disable: 4244 4305)
#endif

#include SYNET_INCLUDE_YOLO(network.h)
extern "C" 
{
#include SYNET_INCLUDE_YOLO(region_layer.h)
#include SYNET_INCLUDE_YOLO(utils.h)
#include SYNET_INCLUDE_YOLO(parser.h)
#include SYNET_INCLUDE_YOLO(box.h)
#include SYNET_INCLUDE_YOLO(stb_image.h)
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

namespace Synet
{
    class YoloToSynet
    {
    public:
        bool Convert(const String & srcModelPath, const String & srcWeightPath, const String & dstModelPath, const String & dstWeightPath)
        {
            if (!Synet::FileExist(srcModelPath))
                return false;

            if (!Synet::FileExist(srcWeightPath))
                return false;

            ::network net = ::parse_network_cfg((char*)srcModelPath.c_str());

            ::load_weights(&net, (char*)srcWeightPath.c_str());

            Synet::NetworkParamHolder holder;
            Tensors weight;
            if (!ConvertNetwork(net, holder(), weight))
            {
                ::free_network(net);
                return false;
            }

            ::free_network(net);

            if (!holder.Save(dstModelPath, false))
                return false;

            if (!SaveWeight(weight, dstWeightPath))
                return false;

            return true;
        }

    private:

        typedef Synet::Tensor<float> Tensor;
        typedef std::vector<Tensor> Tensors;

        bool ConvertNetwork(const ::network & net, Synet::NetworkParam & dstNetwork, Tensors & weight)
        {
            dstNetwork.layers().reserve(net.n*2);
            for (int i = 0; i < net.n; ++i)
            {
                Synet::LayerParam main, func;
                if (!ConvertLayer(net.layers[i], main, func, weight))
                    return false;
                if (main.type() != LayerTypeUnknown)
                    dstNetwork.layers().push_back(main);
                if (func.type() != LayerTypeUnknown)
                    dstNetwork.layers().push_back(func);
            }
            return true;
        }

        bool ConvertLayer(const ::layer & layer, Synet::LayerParam & main, Synet::LayerParam & func, Tensors & weight)
        {
            switch (layer.type)
            {
            case ::CONVOLUTIONAL:
                break;
            case ::DETECTION:
                break;
            case ::MAXPOOL:
                break;
            case ::REGION:
                break;
            case ::REORG:
                break;
            case ::ROUTE:
                break;
            default:
                assert(0);
                return false;
            }
            return true;
        }

        bool SaveWeight(const Tensors & weight, const String & path)
        {
            std::ofstream ofs(path.c_str(), std::ofstream::binary);
            if (ofs.is_open())
            {
                for (size_t i = 0; i < weight.size(); ++i)
                {
                    ofs.write((const char*)weight[i].Data(), weight[i].Size()*sizeof(float));
                }
                ofs.close();
                return true;
            }
            return false;
        }
    };

    bool ConvertYoloToSynet(const String & srcData, const String & srcWeights, const String & dstXml, const String & dstBin)
    {
        YoloToSynet yoloToSynet;
        return yoloToSynet.Convert(srcData, srcWeights, dstXml, dstBin);
    }
}

#endif