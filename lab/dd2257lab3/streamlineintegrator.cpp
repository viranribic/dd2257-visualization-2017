/*********************************************************************
 *  Author  : Himangshu Saikia
 *  Init    : Tuesday, September 19, 2017 - 15:08:33
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab3/streamlineintegrator.h>
#include <dd2257lab3/integrator.h>
#include <inviwo/core/util/utilities.h>
#include <inviwo/core/interaction/events/mouseevent.h>

namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo StreamlineIntegrator::processorInfo_
{
    "org.inviwo.StreamlineIntegrator",      // Class identifier
    "Streamline Integrator",                // Display name
    "DD2257",                               // Category
    CodeState::Experimental,                // Code state
    Tags::None,                             // Tags
};

const ProcessorInfo StreamlineIntegrator::getProcessorInfo() const
{
    return processorInfo_;
}


StreamlineIntegrator::StreamlineIntegrator()
	:Processor()
	, inData("volIn")
	, outMesh("meshOut")
    , propStartPoint("startPoint", "Start Point", vec2(0.5, 0.5), vec2(0), vec2(1024), vec2(0.5))
    , propSeedMode("seedMode", "Seeds")
    // TODO: Initialize additional properties
    // propertyName("propertyIdentifier", "Display Name of the Propery", 
    // default value (optional), minimum value (optional), maximum value (optional), increment (optional));
    // propertyIdentifier cannot have spaces
    , mouseMoveStart("mouseMoveStart", "Move Start", [this](Event* e) { eventMoveStart(e); },
        MouseButton::Left, MouseState::Press | MouseState::Move)
	, propSingleColor("singleColor", "Single line color", vec4(0.0f, 1.0f, 0.0f, 1.0f),
		vec4(0.0f), vec4(1.0f), vec4(0.1f),
		InvalidationLevel::InvalidOutput, PropertySemantics::Color)
	, propIntegrationStepsCount("intSteps", "Integration Step Count", 100,0)
	, propIntegrationStepsMagnitude("stepSize", "Integration Step Magnitude", 0.1, 0, 10000)
	, propIntegDirection("integDirection", "Integration Direction")
	, propStopingCondition("stopingCondition", "Stoping Condition")
	, propMethod("method", "Method")
	, propInDirectionField("inDirField","In Direction Field",false)
	, propArcLength("archLength", "Arch Length", 10, 0, 100000)
	, propMinimalSpeed("minSpeed", "Minimal Speed", 0.01, 0, 1)
	
{
    // Register Ports
	addPort(inData);
	addPort(outMesh);

    // Register Properties
    propSeedMode.addOption("one", "Single Start Point", 0);
    propSeedMode.addOption("multiple", "Multiple Seeds", 1);
    addProperty(propSeedMode);
    addProperty(propStartPoint);
    addProperty(mouseMoveStart);
	addProperty(propSingleColor);


    // TODO: Register additional properties
	addProperty(propInDirectionField);
	addProperty(propIntegrationStepsCount);
	addProperty(propIntegrationStepsMagnitude);
	addProperty(propArcLength);
	addProperty(propMinimalSpeed);

	addProperty(propIntegDirection);
	propIntegDirection.addOption("forward", "Forward", INTEGRATOR_DIRECTION_FORWARD);
	propIntegDirection.addOption("backward", "Backward", INTEGRATOR_DIRECTION_BACKWARD);
	
	addProperty(propStopingCondition);
	propStopingCondition.addOption("steps", "Steps", INTEGRATOR_STOP_STEPS);
	propStopingCondition.addOption("arc", "ARC", INTEGRATOR_STOP_ARC);
	propStopingCondition.addOption("boundary", "Boundary", INTEGRATOR_STOP_BOUNDARY);
	propStopingCondition.addOption("zeroes", "Zeroes", INTEGRATOR_STOP_ZEROES);
	propStopingCondition.addOption("slow", "Slow", INTEGRATOR_STOP_SLOW);


	addProperty(propMethod);
	propMethod.addOption("eulerMethod", "Euler", INTEGRATOR_EULER);
	propMethod.addOption("rk4Method", "RK4", INTEGRATOR_RK4);

    // Show properties for a single seed and hide properties for multiple seeds (TODO)
    propSeedMode.onChange([this]()
    {
        if (propSeedMode.get() == 0)
        {
            util::show(propStartPoint, mouseMoveStart);
            // util::hide(...)
        }
        else
        {
            util::hide(propStartPoint, mouseMoveStart);
            // util::show(...)
        }
    });

}

void StreamlineIntegrator::eventMoveStart(Event* event)
{
    // Handle mouse interaction only if we 
    // are in the mode with a single point
    if (propSeedMode.get() == 1) return;
    auto mouseEvent = static_cast<MouseEvent*>(event);
    vec2 mousePos = mouseEvent->posNormalized();
    // Denormalize to volume dimensions
    mousePos.x *= dims.x - 1;
    mousePos.y *= dims.y - 1;
    // Update starting point
    propStartPoint.set(mousePos);
    event->markAsUsed();
}

double StreamlineIntegrator::getInputValue(const VolumeRAM* data, size3_t dims, size_t x, size_t y)
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

void StreamlineIntegrator::drawPoint(vec3& pt, vec4& xColor, std::vector<BasicMesh::Vertex>& vertices, int& pointCounter, inviwo::IndexBufferRAM* bufferLine, inviwo::IndexBufferRAM* bufferPoint)
{
	float  xMargin = 0.1, yMargin = 0.1;
	double xScale = (1 - 2 * xMargin), yScale = (1 - 2 * yMargin);
	vec3 origin(xMargin, yMargin, 0);
	vec3 axisScaler(xScale, yScale, 0);

	// add first point
	auto plt_pt = vec3(pt.x / (dims.x - 1), pt.y / (dims.y - 1), 0);
	pt = origin + plt_pt * axisScaler;
	LogProcessorInfo("@" + std::to_string(pointCounter) + " [ " + std::to_string(pt[0]) + " , " + std::to_string(pt[1]) + " ] ");

	vertices.push_back({ pt, vec3(0), vec3(0), xColor });
	bufferPoint->add(static_cast<std::uint32_t>(pointCounter));
	bufferLine->add(static_cast<std::uint32_t>(pointCounter));
	pointCounter++;

}

void StreamlineIntegrator::process()
{
    // Get input
	if (!inData.hasData()) {
		return;
	}
	auto vol = inData.getData();

    // Retreive data in a form that we can access it
    auto vr = vol->getRepresentation<VolumeRAM>();
    dims = vol->getDimensions();
    // The start point should be inside the volume (set maximum to the upper right corner)
    propStartPoint.setMaxValue(vec2(dims.x - 1, dims.y - 1));
    
	auto mesh = std::make_shared<BasicMesh>();
	std::vector<BasicMesh::Vertex> vertices;

    if (propSeedMode.get() == 0)
    {
		auto indexBufferLines = mesh->addIndexBuffer(DrawType::Lines, ConnectivityType::Strip);
        auto indexBufferPoints = mesh->addIndexBuffer(DrawType::Points, ConnectivityType::None);
		int pointCounter = 0;


        // Draw start point
        vec2 startPoint = propStartPoint.get();
		auto pt = startPoint;
		Integrator integrator;
		std::vector<vec2> list;

		// TODO add different options
		if(propMethod.get() == INTEGRATOR_EULER){
			if (propStopingCondition.get() == INTEGRATOR_STOP_STEPS) {
				list = integrator.Euler_steps(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propIntegrationStepsCount.get());
			}
			else if (propStopingCondition.get() == INTEGRATOR_STOP_ARC) {
				list = integrator.Euler_arc(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propArcLength.get());
			}
			else if (propStopingCondition.get() == INTEGRATOR_STOP_BOUNDARY) {
				list = integrator.Euler_boundary(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), 0, 0, dims.x - 1, dims.y - 1);
			}if (propStopingCondition.get() == INTEGRATOR_STOP_SLOW) {
				list = integrator.Euler_slow(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propMinimalSpeed.get());
			}if (propStopingCondition.get() == INTEGRATOR_STOP_ZEROES) {
				list = integrator.Euler_zeroes(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get());
			}		
		}
		else if (propMethod.get() == INTEGRATOR_EULER) {
			if (propStopingCondition.get() == INTEGRATOR_STOP_STEPS) {
				list = integrator.RK4_steps(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propIntegrationStepsCount.get());
			}
			else if (propStopingCondition.get() == INTEGRATOR_STOP_ARC) {
				list = integrator.RK4_arc(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propArcLength.get());
			}
			else if (propStopingCondition.get() == INTEGRATOR_STOP_BOUNDARY) {
				list = integrator.RK4_boundary(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), 0, 0, dims.x - 1, dims.y - 1);
			}if (propStopingCondition.get() == INTEGRATOR_STOP_SLOW) {
				list = integrator.RK4_slow(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get(), propMinimalSpeed.get());
			}if (propStopingCondition.get() == INTEGRATOR_STOP_ZEROES) {
				list = integrator.RK4_zeroes(vr, dims, pt, propIntegrationStepsMagnitude.get(), propIntegDirection.get(), propInDirectionField.get());
			}

		}


		for (auto pt : list) {
			auto plt_pt = vec3(pt, 0);
			LogProcessorInfo(std::to_string(plt_pt[0]) +" , " + std::to_string(plt_pt[1]));
			drawPoint(plt_pt, propSingleColor.get(), vertices, pointCounter, indexBufferLines, indexBufferPoints);
		}

    }
    else
    {
        // TODO: Seed multiple stream lines either randomly or using a uniform grid

        // (TODO: Bonus, sample randomly according to magnitude of the vector field)
    }

	mesh->addVertices(vertices);
	outMesh.setData(mesh);
}

} // namespace

