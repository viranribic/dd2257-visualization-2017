/*********************************************************************
 *  Author  : Wiebke 
 *  Init    : Wednesday, August 30, 2017 - 18:02:19
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab1/parallelcoordinates.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>

#include <iostream>
#include <memory>
#include <fstream>

namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo ParallelCoordinates::processorInfo_
{
    "org.inviwo.ParallelCoordinatesDD2257", // Class identifier
    "Parallel Coordinates",                 // Display name
    "DD2257",                               // Category
    CodeState::Experimental,                // Code state
    "Plotting",                             // Tags
};

const ProcessorInfo ParallelCoordinates::getProcessorInfo() const
{
    return processorInfo_;
}

ParallelCoordinates::ParallelCoordinates()
	:Processor()
	, inData("indata")
	, outMeshLines("outMeshLines")
	, outMeshAxis("outMeshAxis")
	, propColorLines("linesColor", "Line Color", vec4(0.0f, 0.0f, 1.0f, 1.0f),
		vec4(0.0f), vec4(1.0f), vec4(0.1f),
		InvalidationLevel::InvalidOutput, PropertySemantics::Color)
	, propColorAxes("axisColor", "Axis Color", vec4(1.0f, 1.0f, 1.0f, 1.0f),
		vec4(0.0f), vec4(1.0f), vec4(0.1f),
		InvalidationLevel::InvalidOutput, PropertySemantics::Color)
	, propMeshSpacing("meshSpacing", "Mesh Spacing", vec2(0.1, 0.1))
	, propAxisHeightSpacing("axisHeightSpacing", "Axis Height Spacing", 0.1)
	, propLinesFinder("lineFinder","Line finder", vec2(0, 0))
{
    // Register ports
    addPort(inData);
    addPort(outMeshAxis);
    addPort(outMeshLines);

    // Register properties
    addProperty(propColorLines);
    addProperty(propColorAxes);

	addProperty(propMeshSpacing);
	addProperty(propAxisHeightSpacing);

	//delete soon
	addProperty(propLinesFinder);
}

//not used for now
template< class T, class U >
std::shared_ptr<T> dynamic_pointer_cast(const std::shared_ptr<U>& r) noexcept
{
	if (auto p = dynamic_cast<typename std::shared_ptr<T>::element_type*>(r.get())) {
		return std::shared_ptr<T>(r, p);
	}
	else {
		return std::shared_ptr<T>(nullptr);
	}
}

typedef std::shared_ptr<vec2> vec2_ptr;

void ParallelCoordinates::process()
{

	//file to see what's going on
	std::ofstream myfile;
	myfile.open("wubalubadubdub.txt");

	auto dataFrame = inData.getData();

	auto meshPoints = std::make_shared<BasicMesh>();
	auto meshAxes = std::make_shared<BasicMesh>();

	size_t numberOfRows = dataFrame->getNumberOfRows();
	size_t numberOfColumns = dataFrame->getNumberOfColumns();

	// TODO make checks

	vec3 diagramOrigin(
		propMeshSpacing.get()[0],
		propMeshSpacing.get()[1],
		0);

	// TODO: Create a parallel coordinates plot 
	// You have to create both an axis mesh and a line mesh
	// Examples on how to create a mesh and how to fill it
	// are given in the Scatter plot processor


	// find min/max values per column
	std::vector<double> colMinV;
	std::vector<double> colMaxV;

	//start at 1 for skipping index column in dataframe
	for (int i = 1; i < numberOfColumns; i++) {
		double colMin = std::numeric_limits<double>::max(), colMax = std::numeric_limits<double>::min();
		auto column = dataFrame->getColumn(i);
		for (int j = 0; j < numberOfRows; j++) {
			colMin = column->getAsDouble(j) < colMin ? column->getAsDouble(j) : colMin;
			colMax = column->getAsDouble(j) > colMax ? column->getAsDouble(j) : colMax;
		}
		//adding the min/max of all min and max values
		colMinV.push_back(colMin);
		colMaxV.push_back(colMax);
		
		myfile << "The column " << column ->  getHeader() << " ( col: " << i << " ) has the min: "<< colMin << " and max: "<< colMax << std::endl;
		myfile << "The column accessed by index  has the min: " << colMinV[i-1] << " and max: " << colMaxV[i-1] << std::endl;
	}
	myfile << std::endl;

	std::vector<BasicMesh::Vertex> verticesAxis;
	std::vector<BasicMesh::Vertex> verticesPoints;

	auto indexBufferLinesAxis =
		meshAxes->addIndexBuffer(DrawType::Lines, ConnectivityType::None);

	auto indexBufferLinesPoints =
		meshPoints->addIndexBuffer(DrawType::Lines, ConnectivityType::None);

	int axisIndex = 0;
	int pointIndex = 0;
	//distance between two columns (dx) on plot
	//height of axis in the plot (dH) - the white line.
	float dx = (1 - 2 * propMeshSpacing.get()[0]) / (numberOfColumns- 1 - 1 /*one more for header*/);
	float gH = (1 - 2 * propMeshSpacing.get()[1]);
	vec3 axisDelta( dx, 0, 0);
	vec3 axisHeight( 0, gH, 0);
	//property in order to shift blue lines for testing
	vec3 linesFinder(propLinesFinder.get(), 0);


	

    for (int i = 0; i < numberOfColumns-1 -1 /*one more for header*/; i++) {
		// TODO check if it's nominal or chategorical variable
		
		//add points lines same ideas as with scatterplot
		for (int j = 0; j < dataFrame->getColumn(i+1/*one more for header*/)->getSize(); j++) {
			float x = (float)dataFrame->getColumn(i+1/*one more for header*/)->getAsDouble(j);
			float y = (float)dataFrame->getColumn(i + 1 +1/*one more for header*/)->getAsDouble(j);
			float px = (x - colMinV[i]) / (colMaxV[i] - colMinV[i]) ;
			float py = (y - colMinV[i+1]) / (colMaxV[i+1] - colMinV[i+1]);

			//logging
			myfile << "In dataframe: row = "<< i << " col = "<< j << std::endl;
			myfile << "Dataset point: x = " << x << " y = " << y << std::endl;
			myfile << "x-min = " << (x - colMinV[i]) << " y - min = " << (y - colMinV[i + 1]) << std::endl;
			myfile << " Xmin= " << colMinV[i] << " Xmax= " << colMaxV[i] << std::endl;
			myfile << " Ymin= " << colMinV[i+1] << " Ymax= " << colMaxV[i+1] << std::endl;
			myfile << " X(Max-min) = " << (colMaxV[i] - colMinV[i]) << " Y(Max-min)  = " << (colMaxV[i + 1] - colMinV[i + 1] )<< std::endl;
			myfile << "Scaled point: x = " << px << " y = " << py << std::endl;

			// x
			verticesPoints.push_back({ linesFinder + diagramOrigin + axisDelta * (float)i + axisHeight * px, vec3(0), vec3(0), propColorLines.get()});
			indexBufferLinesPoints->add(static_cast<std::uint32_t>(pointIndex));
			pointIndex++;

			// y
			verticesPoints.push_back({ linesFinder + diagramOrigin + axisDelta * (float)(i+1) + axisHeight *py, vec3(0), vec3(0), propColorLines.get() });
			indexBufferLinesPoints->add(static_cast<std::uint32_t>(pointIndex));
			pointIndex++;

			myfile << "Point on grid: x = " << linesFinder + diagramOrigin + axisDelta * (float)i + axisHeight * px << " y = " << linesFinder + diagramOrigin + axisDelta * (float)(i + 1) + axisHeight *py << std::endl;
			myfile << std::endl;
		}


		// add axis lines 
		verticesAxis.push_back({ diagramOrigin + axisDelta * (float)i, vec3(0), vec3(0), propColorAxes.get() });
		indexBufferLinesAxis->add(static_cast<std::uint32_t>(axisIndex));
		axisIndex++;

		verticesAxis.push_back({ diagramOrigin + axisDelta * (float)i + axisHeight, vec3(0), vec3(0), propColorAxes.get() });
		indexBufferLinesAxis->add(static_cast<std::uint32_t>(axisIndex));
		axisIndex++;

	}
	// add the last line
	verticesAxis.push_back({ diagramOrigin + axisDelta * (float)(numberOfColumns-1 - 1 /*one more for header*/), vec3(0), vec3(0), propColorAxes.get() });
	indexBufferLinesAxis->add(static_cast<std::uint32_t>(axisIndex));
	axisIndex++;

	verticesAxis.push_back({ diagramOrigin + axisDelta * (float)(numberOfColumns - 1 - 1 /*one more for header*/) + axisHeight, vec3(0), vec3(0), propColorAxes.get() });
	indexBufferLinesAxis->add(static_cast<std::uint32_t>(axisIndex));
	axisIndex++;


	meshAxes->addVertices(verticesAxis);
	meshPoints->addVertices(verticesPoints);

    // Push the mesh out
    outMeshLines.setData(meshPoints);
    outMeshAxis.setData(meshAxes);
	myfile.close();
}

} // namespace

