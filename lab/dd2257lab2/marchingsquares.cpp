/*********************************************************************
 *  Author  : Himangshu Saikia, Wiebke Koepp, ...
 *  Init    : Monday, September 11, 2017 - 12:58:42
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab2/marchingsquares.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>
#include <inviwo/core/util/utilities.h>

namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo MarchingSquares::processorInfo_
{
    "org.inviwo.MarchingSquares",      // Class identifier
    "Marching Squares",                // Display name
    "DD2257",                          // Category
    CodeState::Experimental,           // Code state
    Tags::None,                        // Tags
};

const ProcessorInfo MarchingSquares::getProcessorInfo() const
{
    return processorInfo_;
}


MarchingSquares::MarchingSquares()
	:Processor()
	, inData("volumeIn")
	, meshOut("meshOut")
	, propShowGrid("showGrid", "Show Grid")
	, propShowBothDeciders("propShowBothDeciders", "Show Both Deciders")
	, propDeciderType("deciderType", "Decider Type")
    , propMultiple("multiple", "Iso Levels")
	, propIsoValue("isovalue", "Iso Value")
    , propGridColor("gridColor", "Grid Lines Color", vec4(0.0f, 0.0f, 0.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propIsoColor("isoColor", "Color", vec4(0.0f, 0.0f, 1.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propIsoColorMin("isoColorMin", "ColorMin", vec4(0.0f, 0.0f, 1.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propIsoColorMax("isoColorMax", "ColorMax", vec4(0.0f, 0.0f, 1.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propIsoColorAnother("isoColorAnother", "ColorAnother", vec4(0.0f, 0.0f, 1.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
	, propNumContours("numContours", "Number of Contours", 1, 1, 50, 1)
    , propIsoTransferFunc("isoTransferFunc", "Colors", &inData)
{
    // Register ports
	addPort(inData);
	addPort(meshOut);
	
    // Register properties
	addProperty(propShowGrid);
	addProperty(propShowBothDeciders);
    addProperty(propGridColor);
    addProperty(propIsoColorMin);
    addProperty(propIsoColorMax);
    addProperty(propIsoColorAnother);
	
	addProperty(propDeciderType);
	propDeciderType.addOption("midpoint", "Mid Point", 0);
	propDeciderType.addOption("asymptotic", "Asymptotic", 1);

	addProperty(propMultiple);
    
    propMultiple.addOption("single", "Single", 0);
    addProperty(propIsoValue);
    addProperty(propIsoColor);

	propMultiple.addOption("multiple", "Multiple", 1);
	addProperty(propNumContours);
    addProperty(propIsoTransferFunc);

    // The default transfer function has just two blue points
    propIsoTransferFunc.get().clearPoints();
	//new transfer function depending on max/min of IsoLines
    propIsoTransferFunc.get().addPoint(vec2(0.0f, 1.0f), propIsoColorMin.get());
    propIsoTransferFunc.get().addPoint(vec2(1.0f, 1.0f), propIsoColorMax.get());
    propIsoTransferFunc.setCurrentStateAsDefault();

    util::hide(propGridColor, propNumContours, propIsoTransferFunc);

    // Show the grid color property only if grid is actually displayed
    propShowGrid.onChange([this]()
    {
        if (propShowGrid.get())
        {
            util::show(propGridColor);
        }
        else
        {
            util::hide(propGridColor);
        }
    });

    propShowBothDeciders.onChange([this]()
    {
        if (propShowBothDeciders.get())
        {
            util::show(propIsoColorAnother);
            util::hide(propMultiple);
            propMultiple.set(0);
			util::show(propIsoValue, propIsoColor);
			util::hide(propNumContours, propIsoTransferFunc);

        }
        else
        {
            util::hide(propIsoColorAnother);
            util::show(propMultiple);
            propMultiple.set(1);
			util::hide(propIsoValue);
            util::show(propIsoColor, propNumContours);
        }
    });

    // Show options based on display of one or multiple iso contours
    propMultiple.onChange([this]()
    {
        if (propMultiple.get() == 0)
        {
            util::show(propIsoValue, propIsoColor);
            util::hide(propNumContours, propIsoTransferFunc);
        }
        else
        {
            util::hide(propIsoValue);
            util::show(propIsoColor, propNumContours);
            // TODO (Bonus): Comment out above if you are using the transfer function
            // and comment in below instead
            // util::hide(propIsoValue, propIsoColor);
            // util::show(propNumContours, propIsoTransferFunc);
        }
    });

}

double MarchingSquares::getInputValue(const VolumeRAM* data, size3_t dims, size_t x, size_t y)
{
    if (x < dims.x && y < dims.y)
    {
        return data->getAsDouble(size3_t(x, y, 0));
    }
    else
    {
        LogProcessorError("Attempting to access data outside the boundaries of the volume, value is set to 0");
        return 0;
    }
}

void MarchingSquares::process()
{
	propIsoTransferFunc.get().clearPoints();
    propIsoTransferFunc.get().addPoint(vec2(0.0f, 1.0f), propIsoColorMin.get());
    propIsoTransferFunc.get().addPoint(vec2(1.0f, 1.0f), propIsoColorMax.get());
    propIsoTransferFunc.setCurrentStateAsDefault();

	if (!inData.hasData()) {
		return;
	}

    // This results in a shared pointer to a volume
	auto vol = inData.getData();

    // Extract the minimum and maximum value from the input data
    const double minValue = vol->dataMap_.valueRange[0];
    const double maxValue = vol->dataMap_.valueRange[1];

    // Set the range for the isovalue to that minimum and maximum
    propIsoValue.setMinValue(minValue);
	propIsoValue.setMaxValue(maxValue);

    // You can print to the Inviwo console with Log-commands:
    LogProcessorInfo("This scalar field contains values between " << minValue << " and " << maxValue << ".");
    // You can also inform about errors and warnings:
    // LogProcessorWarn("I am warning about something"); // Will print warning message in yellow
    // LogProcessorError("I am letting you know about an error"); // Will print error message in red
    // (There is also LogNetwork...() and just Log...(), these display a different source,
    // LogProcessor...() for example displays the name of the processor in the workspace while
    // Log...() displays the identifier of the processor (thus with multiple processors of the
    // same kind you would not know which one the information is coming from
    
    // Retreive data in a form that we can access it
    const VolumeRAM* vr = vol->getRepresentation< VolumeRAM >();
    const size3_t dims = vol->getDimensions();

    // Initialize mesh and vertices
	auto meshGrid = std::make_shared<BasicMesh>();
	std::vector<BasicMesh::Vertex> vertices;
	auto indexBufferGrid =
		meshGrid->addIndexBuffer(DrawType::Lines, ConnectivityType::None);

    // Values within the input data are accessed by the function below
    // It's input is the VolumeRAM from above, the dimensions of the volume
    // and the x- and y- index of the position to be accessed where
    // x is in [0, dims.x-1] and y is in [0, dims.y-1]
    float valueat00 = getInputValue(vr, dims, 0, 0);
    LogProcessorInfo("Value at (0,0) is: " << valueat00);
    // You can assume that dims.z = 1 and do not need to consider others cases

    // Grid
    int verticesIndex = 0;

    if (propShowGrid.get())
    {
        // Add grid lines of the given color 
        // draw row lines
        for (size_t j = 0; j < dims.y; j++) {
            for (size_t i = 0; i < dims.x-1; i++) {
            	vertices.push_back({vec3((float)i/(dims.x-1), (float)j/(dims.y-1), 0), vec3(0), vec3(0), propGridColor.get()});
            	indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
            	verticesIndex++;

            	vertices.push_back({vec3((float)(i+1)/(dims.x-1), (float)j/(dims.y-1), 0), vec3(0), vec3(0), propGridColor.get()});
            	indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
            	verticesIndex++;

                // LogProcessorInfo("x Index is: " << (float)i/dims.x);
                // LogProcessorInfo("y Index is: " << (float)j/dims.y);
            }
        }

        // draw column lines
        for (size_t i = 0; i < dims.x; i++) {
            for (size_t j = 0; j < dims.y-1; j++) {
                vertices.push_back({vec3((float)i/(dims.x-1), (float)j/(dims.y-1), 0), vec3(0), vec3(0), propGridColor.get()});
                indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
                verticesIndex++;

                vertices.push_back({vec3((float)i/(dims.x-1), (float)(j+1)/(dims.y-1), 0), vec3(0), vec3(0), propGridColor.get()});
                indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
                verticesIndex++;
            }
        }
    }

    // Iso contours
	//propMultiple for single (0) or multiple isolines (1)
    if (propMultiple.get() == 0)
    {
        // Draw a single isoline at the specified isovalue (propIsoValue) 
        // and color it with the specified color (propIsoColor)
        float isoValue = propIsoValue.get();
        LogProcessorInfo("IsoValue is: " << isoValue);
        std::vector<vec3> v;

		//property for both midpoint/asymptotic strategy
        if (propShowBothDeciders.get() == 1) {
        	
	        for (size_t j = 0; j < dims.y-1; j++) {
	            for (size_t i = 0; i < dims.x-1; i++) {
					//get values for points in the uniform grids (for interpolation)
					// f_min <= c <= f_max
	                float val00 = getInputValue(vr, dims, i, j);
	                // LogProcessorInfo("Val00 is: " << val00);
	                float val10 = getInputValue(vr, dims, i+1, j);
	                // LogProcessorInfo("Val10 is: " << val10);
	                float val01 = getInputValue(vr, dims, i, j+1);
	                // LogProcessorInfo("Val01 is: " << val01);
	                float val11 = getInputValue(vr, dims, i+1, j+1);
	                // LogProcessorInfo("Val11 is: " << val11);
	                float min = getMinMax(val00, val10, val01, val11)[0];
	                float max = getMinMax(val00, val10, val01, val11)[1];

					//mid point decider
	                propDeciderType.set(0);
					//max > c (+) min < c ´(-)
	                if (max >= isoValue && min < isoValue) {
	                    // LogProcessorInfo("Cell with x Index is: " << (float)i);
	                    // LogProcessorInfo("Cell with y Index is: " << (float)j);
	                    v = getIsoVertices(i, j, val00, val01, val10, val11, isoValue, dims);
	                }
	              
	                for (size_t k = 0; k < v.size(); k++) {
	                	vertices.push_back({v[k], vec3(0), vec3(0), propIsoColor.get()});
	            		indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
	            		verticesIndex++;
	                }

					//asymptotic decider
					propDeciderType.set(1);
	                if (max >= isoValue && min < isoValue) {
	                    // LogProcessorInfo("Cell with x Index is: " << (float)i);
	                    // LogProcessorInfo("Cell with y Index is: " << (float)j);
	                    v = getIsoVertices(i, j, val00, val01, val10, val11, isoValue, dims);
	                }
	              
	                for (size_t k = 0; k < v.size(); k++) {
	                	vertices.push_back({v[k], vec3(0), vec3(0), propIsoColorAnother.get()});
	            		indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
	            		verticesIndex++;
	                }
	            }
	        }
	    }
	    else {
	        for (size_t j = 0; j < dims.y-1; j++) {
	            for (size_t i = 0; i < dims.x-1; i++) {
	                float val00 = getInputValue(vr, dims, i, j);
	                // LogProcessorInfo("Val00 is: " << val00);
	                float val10 = getInputValue(vr, dims, i+1, j);
	                // LogProcessorInfo("Val10 is: " << val10);
	                float val01 = getInputValue(vr, dims, i, j+1);
	                // LogProcessorInfo("Val01 is: " << val01);
	                float val11 = getInputValue(vr, dims, i+1, j+1);
	                // LogProcessorInfo("Val11 is: " << val11);
	                float min = getMinMax(val00, val10, val01, val11)[0];
	                float max = getMinMax(val00, val10, val01, val11)[1];
	                if (max >= isoValue && min < isoValue) {
	                    // LogProcessorInfo("Cell with x Index is: " << (float)i);
	                    // LogProcessorInfo("Cell with y Index is: " << (float)j);
	                    v = getIsoVertices(i, j, val00, val01, val10, val11, isoValue, dims);
	                }
	              
	                for (size_t k = 0; k < v.size(); k++) {
	                	vertices.push_back({v[k], vec3(0), vec3(0), propIsoColor.get()});
	            		indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
	            		verticesIndex++;
	                }
	            }
	        }
	    }

    }
    else
    {
        // TODO: Draw a the given number (propNumContours) of isolines between 
        // the minimum and maximum value
        // Hint: If the number of contours to be drawn is 1, the iso value for
        // that contour would be half way between maximum and minimum
        int n = propNumContours.get();
        float diff = (float)(maxValue - minValue)/(n + 1);
        std::vector<float> isoValues;
		std::vector<vec4> isoColors;        
        for (int i = 1; i <= n; i++) {
        	float f = minValue + diff * i;
        	isoValues.push_back(f);
        	isoColors.push_back(propIsoTransferFunc.get().sample((f - minValue)/(maxValue - minValue)));
        }

        for (int iso = 0; iso < n; iso++) {
        	float isoValue = isoValues[iso];
        	// LogProcessorInfo("IsoValue is: " << isoValue);
        	std::vector<vec3> v;

	        for (size_t j = 0; j < dims.y-1; j++) {
	            for (size_t i = 0; i < dims.x-1; i++) {
	                float val00 = getInputValue(vr, dims, i, j);
	                // LogProcessorInfo("Val00 is: " << val00);
	                float val10 = getInputValue(vr, dims, i+1, j);
	                // LogProcessorInfo("Val10 is: " << val10);
	                float val01 = getInputValue(vr, dims, i, j+1);
	                // LogProcessorInfo("Val01 is: " << val01);
	                float val11 = getInputValue(vr, dims, i+1, j+1);
	                // LogProcessorInfo("Val11 is: " << val11);
	                float min = getMinMax(val00, val10, val01, val11)[0];
	                float max = getMinMax(val00, val10, val01, val11)[1];
	                if (max >= isoValue && min < isoValue) {
	                    // LogProcessorInfo("Cell with x Index is: " << (float)i);
	                    // LogProcessorInfo("Cell with y Index is: " << (float)j);
	                    v = getIsoVertices(i, j, val00, val01, val10, val11, isoValue, dims);
	                }
	              
	                for (size_t k = 0; k < v.size(); k++) {
	                	vertices.push_back({v[k], vec3(0), vec3(0), isoColors[iso]});
	            		indexBufferGrid->add(static_cast<std::uint32_t>(verticesIndex));
	            		verticesIndex++;
	                }
	            }
	        }
        }
        
        // TODO (Bonus): Use the transfer function property to assign a color
        // The transfer function normalizes the input data and sampling colors
        // from the transfer function assumes normalized input, that means
        // vec4 color = propIsoTransferFunc.get().sample(0.0f);
        // is the color for the minimum value in the data
        // vec4 color = propIsoTransferFunc.get().sample(1.0f);
        // is the color for the maximum value in the data

    }

    // Note: It is possible to add multiple index buffers to the same mesh,
    // thus you could for example add one for the grid lines and one for
    // each isoline
    // Also, consider to write helper functions to avoid code duplication
    // e.g. for the computation of a single iso contour

	meshGrid->addVertices(vertices);
	meshOut.setData(meshGrid);
}

//get min/max value compared to end points
std::vector<float> MarchingSquares::getMinMax(float val00, float val01, float val10, float val11) {
    std::vector<float> vMinMax = {val00, val01, val10, val11};
    float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::min();
    for (size_t i = 0; i < vMinMax.size(); i++) {
        min = vMinMax[i] < min ? vMinMax[i] : min;
        max = vMinMax[i] > max ? vMinMax[i] : max;
    }
    std::vector<float> ret;
    ret.push_back(min);
    ret.push_back(max);
    return ret;
}


std::vector<vec3> MarchingSquares::getIsoVertices(int xIdx, int yIdx, float val00, float val01, float val10, float val11, float isoValue, size3_t dims) {
    // std::vector<float> v = {val00, val01, val10, val11};
    std::vector<vec3> vIso;
	//for interpolation of isoline
    vec3 isoPosBotton(((float)xIdx+(isoValue-val00)/(val10-val00))/(dims.x-1), (float)yIdx/(dims.y-1), 0);
    vec3 isoPosUp(vec3(((float)xIdx+(isoValue-val01)/(val11-val01))/(dims.x-1), ((float)yIdx+1)/(dims.y-1), 0));
    vec3 isoPosLeft(vec3((float)xIdx/(dims.x-1), ((float)yIdx+(isoValue-val00)/(val01-val00))/(dims.y-1), 0));
    vec3 isoPosRight(vec3(((float)xIdx+1)/(dims.x-1), ((float)yIdx+(isoValue-val10)/(val11-val10))/(dims.y-1), 0));
	//midpoint the average of the four
    float midPoint = (val00 + val01 + val10 + val11)/4;
	//use formula on f(x_a,y_a) to get asymPoint
	//decision rule if c > f(x_a,y_a) connect (a,b) and (c,d)
	//else connect (a,d) and (b,c).
    float asymPoint = (val00*val11 - val10*val01)/(val11 + val00 - val10 - val01);

	//some of the cases to consider in total 2^4 = 16 cases
	//case 0000, and 1111 is ignored as well some case are similar like (1001,0011,1100,0101) --> 12 cases

	//case 1000 or 0111
    if ((val00 >= isoValue && val01 < isoValue && val10 < isoValue && val11 < isoValue) ||
    	(val00 < isoValue && val01 >= isoValue && val10 >= isoValue && val11 >= isoValue)) {
		//positions of the isoLines
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosLeft);
    }
	//case 0101 or 1011
    else if ((val10 >= isoValue && val00 < isoValue && val01 < isoValue && val11 < isoValue) ||
    	(val10 < isoValue && val00 >= isoValue && val01 >= isoValue && val11 >= isoValue)) {
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosRight);
    }
	//case 0010 or 1101
    else if ((val01 >= isoValue && val00 < isoValue && val10 < isoValue && val11 < isoValue) ||
    	(val01 < isoValue && val00 >= isoValue && val10 >= isoValue && val11 >= isoValue)) {
    	vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosLeft);
    }
	//case 0001 or 1110
    else if ((val11 >= isoValue && val00 < isoValue && val10 < isoValue && val01 < isoValue) ||
    	(val11 < isoValue && val00 >= isoValue && val10 >= isoValue && val01 >= isoValue)) {
		vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosRight);
    }
	//case 1010 or  0101
    else if ((val00 >= isoValue && val01 >= isoValue && val10 < isoValue && val11 < isoValue) ||
    	(val00 < isoValue && val01 < isoValue && val10 >= isoValue && val11 >= isoValue)) {
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosUp);
    }
	//case 1100 or 0011
    else if ((val00 >= isoValue && val10 >= isoValue && val01 < isoValue && val11 < isoValue) ||
    	(val00 < isoValue && val10 < isoValue && val01 >= isoValue && val11 >= isoValue)) {
    	vIso.push_back(isoPosLeft);
    	vIso.push_back(isoPosRight);
    }
	//to solve ambiguties with midpoint (not exact) let mid point decided via bilinear interpolation where to draw isoline
	//negative case (-)
    else if ((val00 >= isoValue && val11 >= isoValue && val01 < isoValue && val10 < isoValue && midPoint < isoValue && propDeciderType.get() == 0) ||
    	(val00 < isoValue && val11 < isoValue && val01 >= isoValue && val10 >= isoValue && midPoint >= isoValue && propDeciderType.get() == 0)) {
    	LogProcessorInfo("mid point:" << midPoint);
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosLeft);
        vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosRight);
    }
	//positive case (+)
    else if ((val00 >= isoValue && val11 >= isoValue && val01 < isoValue && val10 < isoValue && midPoint >= isoValue && propDeciderType.get() == 0) ||
    	(val00 < isoValue && val11 < isoValue && val01 >= isoValue && val10 >= isoValue && midPoint < isoValue && propDeciderType.get() == 0)) {
    	LogProcessorInfo("mid point:" << midPoint);
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosRight);
        vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosLeft);
    }
	//asymptotic decider two cases if c < f(x_a, y_a) connect (a,d) and (b,c)
	// else if c > f(x_a,y_a) connect (a,b) and (c,d)
    else if ((val00 >= isoValue && val11 >= isoValue && val01 < isoValue && val10 < isoValue && asymPoint < isoValue && propDeciderType.get() == 1) ||
    	(val00 < isoValue && val11 < isoValue && val01 >= isoValue && val10 >= isoValue && asymPoint >= isoValue && propDeciderType.get() == 1)) {
    	LogProcessorInfo("asymptotic point:" <<asymPoint);
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosLeft);
        vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosRight);
    }
    else if ((val00 >= isoValue && val11 >= isoValue && val01 < isoValue && val10 < isoValue && asymPoint >= isoValue && propDeciderType.get() == 1) ||
    	(val00 < isoValue && val11 < isoValue && val01 >= isoValue && val10 >= isoValue && asymPoint < isoValue && propDeciderType.get() == 1)) {
    	LogProcessorInfo("Asymptotic point:" << asymPoint);
    	vIso.push_back(isoPosBotton);
    	vIso.push_back(isoPosRight);
        vIso.push_back(isoPosUp);
    	vIso.push_back(isoPosLeft);
    }

    return vIso;
}



} // namespace

















