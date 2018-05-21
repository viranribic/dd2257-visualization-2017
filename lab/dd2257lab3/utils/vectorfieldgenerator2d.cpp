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

#include <dd2257lab3/utils/vectorfieldgenerator2d.h>
#include <modules/opengl/texture/textureunit.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/opengl/shader/shaderutils.h>
#include <modules/opengl/volume/volumeutils.h>
#include <modules/opengl/volume/volumegl.h>
#include <modules/opengl/texture/textureutils.h>
#include <inviwo/core/util/utilities.h>

namespace inviwo {

const ProcessorInfo VectorFieldGenerator::processorInfo_{
    "org.inviwo.VectorFieldGeneratorDD2257",  // Class identifier
    "Vector Field Generator",          // Display name
    "DD2257",                      // Category
    CodeState::Experimental,              // Code state
    Tags::GL,                             // Tags
};
const ProcessorInfo VectorFieldGenerator::getProcessorInfo() const {
    return processorInfo_;
}

    VectorFieldGenerator::VectorFieldGenerator()
        : Processor()
        , outport_("outport")
        , xSize_("xSize", "Volume size", size_t(16), size_t(2), size_t(1024))
        , ySize_("ySize", "Volume size", size_t(16), size_t(2), size_t(1024))
        , xRange_("xRange", "X Range", -1, 1, -10, 10)
        , yRange_("yRange", "Y Range", -1, 1, -10, 10)
        , zRange_("zRange", "Z Range", -1, 1, -10, 10)
        , xValue_("x", "X", "-y", InvalidationLevel::InvalidResources)
        , yValue_("y", "Y", "x", InvalidationLevel::InvalidResources)
        , zValue_("z", "Z", "0", InvalidationLevel::InvalidResources)
        , shader_("volume_gpu.vert", "volume_gpu.geom", "vectorfieldgenerator.frag", false)
        , fbo_()
    {
        addPort(outport_);

        addProperty(xSize_);
        addProperty(ySize_);
        addProperty(xValue_);
        addProperty(yValue_);
        addProperty(zValue_);

        addProperty(xRange_);
        addProperty(yRange_);
        addProperty(zRange_);
          
        util::hide(zValue_, zRange_);

        shader_.onReload([this]() { invalidate(InvalidationLevel::Valid); });
    }

VectorFieldGenerator::~VectorFieldGenerator()  { }


void VectorFieldGenerator::initializeResources() {
    shader_.getFragmentShaderObject()->addShaderDefine("X_VALUE(x,y,z)", xValue_.get());
    shader_.getFragmentShaderObject()->addShaderDefine("Y_VALUE(x,y,z)", yValue_.get());
    shader_.getFragmentShaderObject()->addShaderDefine("Z_VALUE(x,y,z)", zValue_.get());

    shader_.build();
}

void VectorFieldGenerator::process() {

    const size3_t dim{xSize_.get(), ySize_.get() , 1};
    auto volume = std::make_shared<Volume>(dim, DataVec2Float32::get());
    volume->dataMap_.dataRange = vec2(0, 1);
    volume->dataMap_.valueRange = vec2(-1, 1);

    shader_.activate();
    TextureUnitContainer cont;
    utilgl::bindAndSetUniforms(shader_, cont, *volume.get(), "volume");
    utilgl::setUniforms(shader_, xRange_, yRange_, zRange_);
    fbo_.activate();
    glViewport(0, 0, static_cast<GLsizei>(dim.x), static_cast<GLsizei>(dim.y));

    VolumeGL* outVolumeGL = volume->getEditableRepresentation<VolumeGL>();
    fbo_.attachColorTexture(outVolumeGL->getTexture().get(), 0);

    utilgl::multiDrawImagePlaneRect(static_cast<int>(dim.z));

    shader_.deactivate();
    fbo_.deactivate();

     vec3 corners[4];
    corners[0] = vec3(xRange_.get().x, yRange_.get().x, zRange_.get().x);
    corners[1] = vec3(xRange_.get().y, yRange_.get().x, zRange_.get().x);
    corners[2] = vec3(xRange_.get().x, yRange_.get().y, zRange_.get().x);
    corners[3] = vec3(xRange_.get().x, yRange_.get().x, zRange_.get().y);

    mat3 basis;
    basis[0] = corners[1] - corners[0];
    basis[1] = corners[2] - corners[0];
    basis[2] = corners[3] - corners[0];
    
    volume->setBasis(basis);
    volume->setOffset(corners[0]);
    outport_.setData(volume);
}

} // namespace


