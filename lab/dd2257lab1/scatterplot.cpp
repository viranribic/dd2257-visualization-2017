/*********************************************************************
 *  Author  : Wiebke 
 *  Init    : Wednesday, August 30, 2017 - 18:02:35
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab1/scatterplot.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>

namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo ScatterPlot::processorInfo_
{
    "org.inviwo.ScatterPlot",   // Class identifier
    "Scatter Plot",             // Display name
    "DD2257",                   // Category
    CodeState::Experimental,    // Code state
    "Plotting",                 // Tags
};

const ProcessorInfo ScatterPlot::getProcessorInfo() const
{
    return processorInfo_;
}

ScatterPlot::ScatterPlot()
    :Processor()
    , inData("indata")
    , outMeshPoints("outMeshPoints")
    , outMeshLines("outMeshLines")
    , propColorPoint("pointColor", "Point Color", vec4(0.0f, 0.0f, 1.0f, 1.0f), 
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propColorAxes("axisColor", "Axis Color", vec4(1.0f, 1.0f, 1.0f, 1.0f),
        vec4(0.0f), vec4(1.0f), vec4(0.1f),
        InvalidationLevel::InvalidOutput, PropertySemantics::Color)
    , propXAxis("xAxis", "X Axis")
    , propYAxis("yAXis", "Y Axis")
	, propMeshSpacing("meshSpacing", "Mesh Spacing", vec2(0.1, 0.1))
{

    // Register ports
    addPort(inData);
    addPort(outMeshPoints);
    addPort(outMeshLines);

    // Register properties
    addProperty(propColorPoint);
    addProperty(propColorAxes);
    addProperty(propXAxis);
    addProperty(propYAxis);
	addProperty(propMeshSpacing);

    // When the data changes we need new axis labels 
    inData.onChange([&]() { updateAxisLabels(); });

}

void ScatterPlot::updateAxisLabels()
{
    if (!inData.hasData()) return;
    
    // Default values
    int selectedX = 0;
    int selectedY = 1;

    // Save old values in order to recover them later
    if (propXAxis.getValues().size() > 0)
    {
        selectedX = propXAxis.get();
        selectedY = propYAxis.get();
    }

    propXAxis.clearOptions();
    propYAxis.clearOptions();

    // Extract header information for choice of columns to display
    int i = 0;
    for (auto column: inData.getData()->getHeaders())
    {
        // Skip the index column
        if (i != 0)
        {
            // Headers contains pairs of name and data format for each column
            propXAxis.addOption(column.first, column.first, i - 1);
            propYAxis.addOption(column.first, column.first, i - 1);
        }
        i++;
    }

    // Make sure selected values are within range
    selectedX = std::min(selectedX, i - 2);
    selectedY = std::min(selectedY, i - 2);

    propXAxis.setSelectedIndex(selectedX);
    propYAxis.setSelectedIndex(selectedY);
}

void ScatterPlot::process()
{

	std::ofstream myfile;
	myfile.open("wubalubadubdub.txt");

	// origin of our plot
	vec3 diagramOrigin(
		propMeshSpacing.get()[0],
		propMeshSpacing.get()[1],
		0);

	//for offset in lines in scatterplot
	float xAxisSize = 1 - 2 * diagramOrigin[0];
	float yAxisSize = 1 - 2 * diagramOrigin[1];


    auto dataFrame = inData.getData();

    // Get data columns but skip over the index (=first) column
    auto dataX = dataFrame->getColumn(propXAxis.get() + 1);
    auto dataY = dataFrame->getColumn(propYAxis.get() + 1);

    size_t numberOfRows = dataFrame->getNumberOfRows();

    // Create a vector of vertices that we will later add to the points mesh
    std::vector<BasicMesh::Vertex> verticesPoints;
    auto meshPoints = std::make_shared<BasicMesh>();
	

    // Create an index buffer for the mesh. This buffer will tell the renderer
    // which vertices are connected. For example for DrawType::Lines and 
    // ConnectivityType::None, the renderer will take 2 indices and treat them 
    // as a line. Then the next two indices in the buffer will be the next line.
    // Here we just specify that we are creating a point mesh
    // For e.g. ConnectivityType::Strip and DrawType::Lines, the first two indices 
    // would be treated as the starting point and the following indices lead to a 
    // strip of lines where the vertices for subsequent indices in the buffer 
    // are connected
    auto indexBufferPoints =
        meshPoints->addIndexBuffer(DrawType::Points, ConnectivityType::None);
	
	double minX= std::numeric_limits<double>::max(), maxX= std::numeric_limits<double>::min();
	double minY = std::numeric_limits<double>::max(), maxY = std::numeric_limits<double>::min();

	for (int j = 0; j < dataX->getSize(); j++) {
		minX= dataX->getAsDouble(j) < minX ? dataX->getAsDouble(j) : minX;
		maxX= dataX->getAsDouble(j) > maxX ? dataX->getAsDouble(j) : maxX;

		minY = dataY->getAsDouble(j) < minY ? dataY->getAsDouble(j) : minY;
		maxY = dataY->getAsDouble(j) > maxY ? dataY->getAsDouble(j) : maxY;
	}

	myfile << "The column X has the min: " << minX << " and max: " << maxX << std::endl;
	myfile << "The column Y has the min: " << minY << " and max: " << maxY << std::endl;

    // TODO: Add points according the data within the chosen columns.
	for (int i = 0; i < numberOfRows; i++) {
		float x = (float)dataX->getAsDouble(i);
		float y = (float)dataY->getAsDouble(i);
		float px = (x - minX) / (maxX - minX);
		float py = (y - minY) / (maxY - minY);

		//add point in the mesh by squeezing it and take into account the origin (shift)
		verticesPoints.push_back({ diagramOrigin + vec3(px*xAxisSize, py*yAxisSize, 0), vec3(0), vec3(0), propColorPoint.get() });
		indexBufferPoints->add(static_cast<std::uint32_t>(i));
		myfile << "Added : " << vec3(px*xAxisSize, py*yAxisSize, 0) << " on grid: " << diagramOrigin + vec3(px, py, 0)  << std::endl;

	}


    // You can access values within one column by their index
    //size_t index = 0;
    //float exampleValueX = (float)dataX->getAsDouble(index);

    // A single vertex is defined a position, a normal, a texture coordinates and a color:
    // {pos, norm, texcor, color}
    // For this simple example we are only using the position and the color
    //verticesPoints.push_back({ vec3(0.4, 0.5, 0), vec3(0), vec3(0), propColorPoint.get() });
    // Add index of the vertex (it is the one with index 0 in the vertices vector)
    //indexBufferPoints->add(static_cast<std::uint32_t>(0));

    // Add the vertices to the mesh
    meshPoints->addVertices(verticesPoints);

    // Create a mesh and vertex vector for the axes
    std::vector<BasicMesh::Vertex> verticesAxis;
    auto meshLines = std::make_shared<BasicMesh>();

    // Now we need lines as the drawtype
    auto indexBufferLines =
        meshLines->addIndexBuffer(DrawType::Lines, ConnectivityType::None);

    // TODO: Add lines corresponding to axis
	//Two lines (4 points)
    verticesAxis.push_back({ diagramOrigin, vec3(0), vec3(0), propColorAxes.get() });
    indexBufferLines->add(static_cast<std::uint32_t>(0));
    verticesAxis.push_back({ vec3(1 - diagramOrigin[0] , diagramOrigin[1], 0) , vec3(0), vec3(0), propColorAxes.get()});
    indexBufferLines->add(static_cast<std::uint32_t>(1));
	myfile << "Grid x from " << diagramOrigin  << " to " << vec3(1 - diagramOrigin[0], diagramOrigin[1], 0) << std::endl;

	verticesAxis.push_back({ diagramOrigin, vec3(0), vec3(0), propColorAxes.get() });
	indexBufferLines->add(static_cast<std::uint32_t>(2));
	verticesAxis.push_back({ vec3(diagramOrigin[0] , 1 - diagramOrigin[1], 0) , vec3(0), vec3(0), propColorAxes.get() });
	indexBufferLines->add(static_cast<std::uint32_t>(3));
	myfile << "Grid y from " << diagramOrigin << " to " << vec3(diagramOrigin[0], 1 - diagramOrigin[1], 0) << std::endl;

    meshLines->addVertices(verticesAxis);

    // Push the meshes out
    outMeshPoints.setData(meshPoints);
    outMeshLines.setData(meshLines);
}

} // namespace

