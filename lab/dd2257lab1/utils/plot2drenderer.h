/*********************************************************************************
*
* Inviwo - Interactive Visualization Workshop
*
* Copyright (c) 2015-2017 Inviwo Foundation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*********************************************************************************/

#ifndef IVW_PLOTDRENDERER_H
#define IVW_PLOTDRENDERER_H

#include <dd2257lab1/dd2257lab1moduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/meshport.h>
#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/rendering/meshdrawer.h>
#include <modules/opengl/shader/shader.h>
#include <inviwo/core/properties/boolproperty.h>
#include <vector>

namespace inviwo
{

    class IVW_MODULE_DD2257LAB1_API Plot2DRenderer : public Processor
    {
    public:
        virtual const ProcessorInfo getProcessorInfo() const override;
        static const ProcessorInfo processorInfo_;
        Plot2DRenderer();
        virtual ~Plot2DRenderer();

        virtual void process() override;

    protected:
        MeshFlatMultiInport inport_;
        ImageInport imageInport_;
        ImageOutport outport_;
        Shader shader_;

        BoolProperty enableDepthTest_;
        FloatProperty top_, bottom_, left_, right_;
        FloatProperty pointSize_;

        using DrawerMap = std::multimap<const Outport*, std::unique_ptr<MeshDrawer>>;
        DrawerMap drawers_;
        void updateDrawers();

    };

} // namespace

#endif // IVW_PLOTDRENDERER_H