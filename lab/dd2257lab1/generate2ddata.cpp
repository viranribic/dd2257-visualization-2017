/*********************************************************************
 *  Author  : Wiebke Koepp, ... 
 *  Init    : Friday, September 01, 2017 - 12:13:44
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab1/generate2ddata.h>
#include <random>


namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo Generate2DData::processorInfo_
{
    "org.inviwo.Generate2DData",      // Class identifier
    "Generate 2D Data",               // Display name
    "DD2257",                         // Category
    CodeState::Experimental,          // Code state
    "CPU, DataFrame"                  // Tags
};

const ProcessorInfo Generate2DData::getProcessorInfo() const
{
    return processorInfo_;
}

Generate2DData::Generate2DData()
    :Processor()
    , outDataFrame("outDataFrame")
    , propdataShape("dataShape", "Data Shape")
	, aScalar("aScalar", "a multipler", 1,-10,10)
	, bScalar("bScalar", "b multipler",1, -10,10)
	, muScalar("muScalar", "mu multipler",1,-10,10)
	, muOffset("muOffset", "mu offset",0,-10,10)
	, xTrans("xTrans", "x offset", 0, -10, 10)
	, yTrans("yTrans", "y offset", 0, -10, 10)
{
    //Register ports
    
    addPort(outDataFrame);
    
    // Register properties

    addProperty(propdataShape);
	addProperty(aScalar);
	addProperty(bScalar);
	addProperty(muScalar);
	addProperty(muOffset);
	addProperty(xTrans);
	addProperty(yTrans);
    propdataShape.addOption("line", "Line", DataShape::Line);
    propdataShape.addOption("circle", "Circle", DataShape::Circle);
    propdataShape.addOption("hyperbola", "Hyperbola", DataShape::Hyperbola);
    propdataShape.set(DataShape::Line);
    propdataShape.setCurrentStateAsDefault();
}


void Generate2DData::process()
{
    // Initialize the dataframe
    auto dataframe = std::make_shared<DataFrame>();

    // Create data container with two columns
    auto colX = dataframe->addColumn<float>("x");
    auto colY = dataframe->addColumn<float>("y");

    // Lamda function for adding a single data point
    auto addPoint = [&](float x, float y)
    {
        colX->add(x);
        colY->add(y);
    };

	//index for how many points that should be created
	const int numPoints = 100;
	//init arrays for points
	float x[numPoints], y[numPoints];

    switch (propdataShape.get())
    {
        case DataShape::Line: {

            // TODO: Add points that lie on a line with negative slope
			//init K as negative slope and m as intercept values may need be changed
			const float K = -1;
			const float m = 1;
			//min <= max else error (as an offset is calculated in the uniform_real_distribution(..)
			const double max = 1.0;
			const double min = 0.0;
			//Stuff needed for random number generation
			std::random_device                  rand_dev; //seed for random generator
			std::mt19937                        generator(rand_dev()); //Standard mersenne_twister_engine
			std::uniform_real_distribution<double>  dis(min, max);
			//create for loop to add N points of the form y = -kx + m 
			//as we want to create points forming a line with negative slope

			//create uniformly distributed points for x_j first
			for (int j = 0; j < numPoints; j++) {
				//dis is used to transform unsigned int to a double
				x[j] = (float) dis(generator);
			}

			//create points for y_i
			for (int i = 0; i < numPoints; i++) {
				y[i] = x[i] * K + m;
				//add N points with O(n) complexity in worst case
				addPoint(x[i], y[i]);
				//addPoint(0.3f, 0.4f);
			}

            break;
        }
        case DataShape::Circle: {
			//random number generator
			std::random_device                  rand_dev; //seed for random generator
			std::mt19937                        generator(rand_dev()); //Standard mersenne_twister_engine
			// circle with orgin in (0,0)
			// x = r cos(theta), y = r sin(theta)
			//define radius and theta
			const float r = 5.0f; 
			//loop for all points
			for (int i = 0; i < numPoints; i++) {
				//chose random radius in [0, r]
				//chose random angle between [0, 2pi]
				//float radius = r*sqrt(generator());
				//keep radius fixed
				float radius = r;
				float theta = 2 * M_PI*generator();
				//convert to x,y coordinates
				x[i] = radius * cos(theta);
				y[i] = radius * sin(theta);
				// TODO: Add points that lie on a circle
				//addPoint(0.24f, 0.4f);
				//add points
				addPoint(x[i], y[i]);

			}

            break;
        }
        case DataShape::Hyperbola: {

            // TODO: Add points that lie on a hyperbola
			// x = a cosh(mu), y = b sinh(mu)
			const double max = 3.0;
			const double min = -3.0;
			//Stuff needed for random number generation
			//std::random_device                  rand_dev; //seed for random generator
			//std::mt19937                        generator(rand_dev()); //Standard mersenne_twister_engine
			std::default_random_engine generator;
			std::uniform_real_distribution<double>  dis(min, max);

			float alpha = 1.0f, beta = 1.0f;
			for (int i = 0; i < numPoints; i++) {
				//generate random mu in range [-3,3]
				float mu = (float)dis(generator);
				//calculate x,y in hyperbolic coordinates
				x[i] = aScalar.get()*alpha*cosh(muScalar.get()*mu) +xTrans.get();
				y[i] = bScalar.get()*beta*sinh(muScalar.get()*mu)+yTrans.get();
				//update alpha and beta (not sure how??
				//addPoint(0.3f, 0.4f);
				x[i] = x[i] * cos(muOffset.get()) - y[i] * sin(muOffset.get());
				y[i] = y[i] * cos(muOffset.get()) + x[i] * sin(muOffset.get());
				addPoint(x[i], y[i]);
			}

			//add same points with neg direction to get other part of hyperbola
			for (int i = 0; i < numPoints; i++) {
				//generate random mu in range [-3,3]
				float mu = (float)dis(generator);
				//calculate x,y in hyperbolic coordinates
				x[i] = aScalar.get()*alpha*-cosh(muScalar.get()*mu)+xTrans.get();
				y[i] = bScalar.get()*beta*sinh(muScalar.get()*mu)+yTrans.get();
				//update alpha and beta (not sure how??
				//addPoint(0.3f, 0.4f);
				x[i] = x[i] * cos(muOffset.get()) - y[i] * sin(muOffset.get());
				y[i] = y[i] * cos(muOffset.get()) + x[i] * sin(muOffset.get());
				addPoint(x[i], y[i]);
			}
            break;
        }
        default: {
            break;
        }
    }

    // We did not take care of the indices (first column of the dataframe)
    // so we will do this now
    dataframe->updateIndexBuffer();

    // Push the created data out
    outDataFrame.setData(dataframe);
}

} // namespace

