/*********************************************************************
 *  Author  : Himangshu Saikia
 *  Init    : Monday, October 02, 2017 - 13:31:17
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#include <dd2257lab4/licprocessor.h>
#include <dd2257lab4/integrator.h>
// #include <dd2257lab3/integrator.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <ctime>

namespace inviwo
{

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo LICProcessor::processorInfo_
{
    "org.inviwo.LICProcessor",      // Class identifier
    "LICProcessor",                // Display name
    "DD2257",              // Category
    CodeState::Experimental,  // Code state
    Tags::None,               // Tags
};

const ProcessorInfo LICProcessor::getProcessorInfo() const
{
    return processorInfo_;
}

vec4 LICProcessor::LIC_atPoint(const inviwo::ImageRAM * tr, size2_t texDims, const VolumeRAM * vr, size3_t dims, const vec2 & position)
{
	//map texture position to vector field position
	double pxWidth = (double)dims.x / texDims.x, pxHeight = (double)dims.y / texDims.y;
	double step = pxWidth > pxHeight ? pxWidth: pxHeight;

	vec2 mapPos(position.x* (double)dims.x / texDims.x, (double)position.y*dims.y / texDims.y);
	auto listForward = integrator.RK4_arc(vr, dims, mapPos, step, INTEGRATOR_DIRECTION_FORWARD, true, propArcLength.get());
	auto listBackward = integrator.RK4_arc(vr, dims, mapPos, step, INTEGRATOR_DIRECTION_BACKWARD, true, propArcLength.get());
	

	auto colLayer = tr->getColorLayerRAM();
	
	// go through positions, interpolate the colors and sum them up
	vec4 convSum(0, 0, 0, 0);

	for (int t = listBackward.size()-1; t >= 1; t--) {
		auto pt = listBackward[t];
		//map vector field position to texture position
		int mapPosX = int(pt.x* (double)texDims.x / dims.x);
		int mapPosY = int(pt.y*(double)texDims.y / dims.y);
		auto color = colLayer->getAsDVec4(size2_t(mapPosX, mapPosY));
		auto k = kernelGauss(t, 0, listBackward.size() + listForward.size());
		convSum += k*color;
	}

	for (int t = 0; t < listForward.size(); t++) {
		auto pt = listForward[t];
		//map vector field position to texture position
		int mapPosX = int(pt.x* (double)texDims.x / dims.x);
		int mapPosY = int(pt.y*(double)texDims.y / dims.y);
		auto color = colLayer->getAsDVec4(size2_t(mapPosX, mapPosY));
		auto k = kernelGauss(t, 0, listBackward.size() + listForward.size());
		convSum += k*color;

	}

	convSum[3] = 255;
	return convSum;

}

std::vector<std::vector<vec4>> LICProcessor::LIC(const inviwo::ImageRAM * tr, size2_t texDims, const VolumeRAM * vr, size3_t dims)
{
	std::vector<std::vector<vec4>> matrix(texDims.x, std::vector<vec4>(texDims.y));
	
	for (int j = 0; j < texDims_.y; j++) {
		for (int i = 0; i < texDims_.x; i++) {
			// not working : LogProcessorInfo(std::to_string(i)+","+ std::to_string(j));
			vec2 position(i,j);
			matrix[i][j] = LIC_atPoint(tr,texDims,vr,dims, position);
			
		}
	}

	return matrix;
}


std::vector<std::pair<vec2, vec4>> LICProcessor::FastLIC_atPoint(const inviwo::ImageRAM * tr, size2_t texDims, const VolumeRAM * vr, size3_t dims, const vec2 & position)
{
	//map texture position to vector field position
	double pxWidth = (double)dims.x / texDims.x, pxHeight = (double)dims.y / texDims.y;
	double step = pxWidth > pxHeight ? pxWidth : pxHeight;

	vec2 mapPos(position.x* (double)dims.x / texDims.x, (double)position.y*dims.y / texDims.y);
	auto listForward = integrator.RK4_boundary(vr, dims, mapPos, step, INTEGRATOR_DIRECTION_FORWARD, true, 0, 0, dims.x - 1, dims.y - 1);
	auto listBackward = integrator.RK4_boundary(vr, dims, mapPos, step, INTEGRATOR_DIRECTION_BACKWARD, true, 0, 0, dims.x - 1, dims.y - 1);


	auto colLayer = tr->getColorLayerRAM();

	// go through positions, interpolate the colors and sum them up
	std::vector<std::pair<vec2, vec4>> outData;
	// initialise all the points on the line
	for (int t = listBackward.size() - 1; t >= 1; t--) {
		auto pt = listBackward[t];
		vec2 mapPos(int(pt.x* (double)texDims.x / dims.x), int(pt.y*(double)texDims.y / dims.y));
		outData.push_back(std::make_pair(vec2(mapPos), vec4(0,0,0,0)));

	}
	for (int t = 0; t < listForward.size(); t++) {
		auto pt = listForward[t];
		vec2 mapPos(int(pt.x* (double)texDims.x / dims.x), int(pt.y*(double)texDims.y / dims.y));
		outData.push_back(std::make_pair(vec2(mapPos), vec4(0, 0, 0, 0)));
	}

	int L = int(propArcLength.get() / 2);
	double k = 1. / (2. * L); //box kernel value
	//LogProcessorInfo("Kernel : "+ std::to_string(k)+" .\n\n");
	std::list<vec4> convKernel;
	vec4 convSum(0, 0, 0, 0);

	for (int t = 0; t < outData.size(); t++) {
		//LogProcessorInfo("t = " + std::to_string(t));
		vec2 pt_t = outData[t].first;
		vec4 color_t = colLayer->getAsDVec4(size2_t(pt_t.x, pt_t.y));
		color_t *= k;
		convKernel.push_back(color_t);

		if (convKernel.size() >= 2 * L) {
			vec4 first_color = convKernel.front();
			convKernel.pop_front();
			// subtract the first element
			convSum -= first_color;
			// add the lastelement 
			convSum += color_t;
			//LogProcessorInfo("conv. sum ->  (" + std::to_string(convSum[0])+" , " + std::to_string(convSum[1]) + " , " + std::to_string(convSum[2]) + " ). ");

		}
		else {
			convSum += color_t;
			//LogProcessorInfo("conv. sum ->  (" + std::to_string(convSum[0]) + " , " + std::to_string(convSum[1]) + " , " + std::to_string(convSum[2]) + " ). ");
		}

		convSum[3] = 255;
		outData[t].second = convSum;

	}

	return outData;

}

std::vector<std::vector<vec4>> LICProcessor::FastLIC(const inviwo::ImageRAM * tr, size2_t texDims, const VolumeRAM * vr, size3_t dims)
{
	std::vector<std::vector<vec4>> matrix(texDims.x, std::vector<vec4>(texDims.y));

	std::vector<std::vector<bool>> unvisitedPoints(texDims.x, std::vector<bool>(texDims.y,true));


	int savedPoints = 0;
	for (int j = 0; j < texDims_.y; j++) {
		for (int i = 0; i < texDims_.x; i++) {
			if (unvisitedPoints[i][j] == false){
				continue;
			}
			unvisitedPoints[i][j] = false;
			vec2 position(i, j);
			auto data = FastLIC_atPoint(tr, texDims, vr, dims, position);
			
			// std::ofstream myfile;
			// myfile.open("out/example"+std::to_string(i)+" " + std::to_string(j) + ".txt");

			for (auto pair : data) {
				// if point is outside of the boundary just continue
				if (pair.first.x > texDims_.x || pair.first.x < 0 || pair.first.y > texDims_.y || pair.first.y < 0)
					continue;

				//LogProcessorInfo("Pt = "+std::to_string(pt.x)+" , "+std::to_string(pt.y)+" .");
				// myfile << " [ " << pair.first.x<< " , " <<pair.first.y << " ] = ( " << pair.second[0] << " , "<< pair.second[1] << " , " << pair.second[2] << ")" << std::endl;
				matrix[pair.first.x][pair.first.y] = pair.second;
				unvisitedPoints[pair.first.x][pair.first.y] = false;
				savedPoints++;
				//LogProcessorInfo("matrix color @ "+ std::to_string(i)+" , "+std::to_string(j)+"  ->  " + std::to_string(matrix[pt.x][pt.y][0]) +" , " + std::to_string(matrix[pt.x][pt.y][1])+" , "+std::to_string(matrix[pt.x][pt.y][2])+" .");
				//LogProcessorInfo("matrix visited @" + std::to_string(i) + " , " + std::to_string(j) + "  ->  " + std::to_string(unvisitedPoints[pt.x][pt.y]));

			}
			// myfile.close();
		}
	}

	LogProcessorInfo("Saved points : " + std::to_string(savedPoints) + " .");
	return matrix;
}

double LICProcessor::kernelGauss(int t, double minL, double maxL)
{
	// we assume the mean is 0
	double std = (maxL - minL) / 4.; // let's assume that the whole interval is 4 sigma wide 
	double Z = 1. / (std* sqrt(2*M_PI));
	double a = - t * t / (2* std* std);
	return Z * exp(a);
}

double LICProcessor::kernelBox(int t, double minL, double maxL)
{
	if (t < minL || t > maxL)
		return 0.0;
	else
		return 1./ (maxL - minL);
}

double LICProcessor::kernelTriangle(int t, double minL, double maxL)
{
	double lHalf= (maxL - minL) / 2;
	if (t < minL || t >= maxL)
		return 0.0;
	else if (t >= minL && t < lHalf)
		return (t - minL) * 1./(lHalf*lHalf); 
	else //t >= lHalf && t < maxL 
		return -(t ) * 1. / (lHalf*lHalf);
}

void LICProcessor::applyContrastEnhancement(std::vector<std::vector<vec4>> & licTexture, size2_t dims, double contrastMean, double contrastStd)
{

	vec4 mean(0.);
	vec4 sumSq(0.);
	vec4 std(0.);
	double N = 0;
	// calculate mean
	for (int i = 0; i < dims.x; i++) {
		for (int j = 0; j < dims.y; j++) {
			auto val = licTexture[i][j];
			if (val[0] == 0 && val[1] == 0 && val[2] == 0)
				continue;
			mean += val;

			//meanSq += val*val; // TODO not sure how vec4 multiplication is implemented
			sumSq[0] += val[0] * val[0];
			sumSq[1] += val[1] * val[1];
			sumSq[2] += val[2] * val[2];

			N++;
		}
	}
	mean *= 1. / N;
	mean[3] = 255;

	// LogProcessorInfo("MEAN: " + std::to_string(mean[0]) + " , " + std::to_string(mean[1]) + " , " + std::to_string(mean[2]) + "," + std::to_string(std[3]) + " .");
	// LogProcessorInfo("N: " + std::to_string(N) );

	// calculate std
	vec4 n_meanSq(0.);
	n_meanSq[0] = mean[0] * mean[0];
	n_meanSq[1] = mean[1] * mean[1];
	n_meanSq[2] = mean[2] * mean[2];
	n_meanSq *= N;
	
	// LogProcessorInfo("sum of squares: " + std::to_string(sumSq[0]) + " , " + std::to_string(sumSq[1]) + " , " + std::to_string(sumSq[2]) + "," + std::to_string(std[3]) + " .");
	// LogProcessorInfo("n * mean**2: " + std::to_string(n_meanSq[0]) + " , " + std::to_string(n_meanSq[1]) + " , " + std::to_string(n_meanSq[2]) + "," + std::to_string(std[3]) + " .");

	std = sumSq - n_meanSq;
	// LogProcessorInfo("STD: " + std::to_string(std[0]) + " , " + std::to_string(std[1]) + " , " + std::to_string(std[2]) + "," + std::to_string(std[3]) + " .");
	std *= 1. / (N - 1);
	// LogProcessorInfo("STD: " + std::to_string(std[0]) + " , " + std::to_string(std[1]) + " , " + std::to_string(std[2]) + "," + std::to_string(std[3]) + " .");
	std[0] = sqrt(std[0]);
	std[1] = sqrt(std[1]);
	std[2] = sqrt(std[2]);
	// LogProcessorInfo("STD: " + std::to_string(std[0]) + " , " + std::to_string(std[1]) + " , " + std::to_string(std[2]) + "," + std::to_string(std[3]) + " .");


	// update parameters
 	vec4 f(0.);
	f[0] = contrastStd / std[0];
	f[1] = contrastStd / std[1];
	f[2] = contrastStd / std[2];
	f[3] = 1;

	// LogProcessorInfo("f: " + std::to_string(f[0]) + " , " + std::to_string(f[1]) + " , " + std::to_string(f[2]) + "," + std::to_string(f[3]) + " .");


	for (int i = 0; i < dims.x; i++) {
		for (int j = 0; j < dims.y; j++) {
			licTexture[i][j][0] -= mean[0];
			licTexture[i][j][1] -= mean[1];
			licTexture[i][j][2] -= mean[2];

			licTexture[i][j][0] *= f[0];
			licTexture[i][j][1] *= f[1];
			licTexture[i][j][2] *= f[2];

			licTexture[i][j][0] += contrastMean;
			licTexture[i][j][1] += contrastMean;
			licTexture[i][j][2] += contrastMean;

			licTexture[i][j][0] = licTexture[i][j][0] < 0 ? 0 : licTexture[i][j][0];
			licTexture[i][j][1] = licTexture[i][j][1] < 0 ? 0 : licTexture[i][j][1];
			licTexture[i][j][2] = licTexture[i][j][2] < 0 ? 0 : licTexture[i][j][2];

			licTexture[i][j][0] = licTexture[i][j][0] > 255 ? 255 : licTexture[i][j][0];
			licTexture[i][j][1] = licTexture[i][j][1] > 255 ? 255 : licTexture[i][j][1];
			licTexture[i][j][2] = licTexture[i][j][2] > 255 ? 255 : licTexture[i][j][2];


		}
	}

}


LICProcessor::LICProcessor()
    :Processor()
	, volumeIn_("volIn")
	, noiseTexIn_("noiseTexIn")
	, licOut_("licOut")
    // TODO: Register properties
	, propArcLength("archLength", "Arch Length", 10, 1, 100000)
	, propApplyContrastEnh("applyContrastEnh","Apply Contrast Enhancement",false)
	, propContrastMean("contrastMean", "Contrast Mean Values", 128, 0, 255)
	, propContrastStd("contrastStd", "Contrast Std Values", 50, 0.000000001, 255)
	, propUseFastLIC("useFastLIC", "Use Fast LIC", false)

{
    // Register ports
	addPort(volumeIn_);
	addPort(noiseTexIn_);
	addPort(licOut_);

    // TODO: Register properties
	addProperty(propArcLength);
	addProperty(propContrastMean);
	addProperty(propContrastStd);
	addProperty(propApplyContrastEnh);
	addProperty(propUseFastLIC);
}


void LICProcessor::process()
{
    // Get input
	if (!volumeIn_.hasData()) {
		return;
	}

	if (!noiseTexIn_.hasData()) {
		return;
	}

	auto vol = volumeIn_.getData();
    vectorFieldDims_ = vol->getDimensions();
    auto vr = vol->getRepresentation<VolumeRAM>();

    // An accessible form of on image is retrieved analogous to a volume
    auto tex = noiseTexIn_.getData();
    texDims_ = tex->getDimensions();
	auto tr = tex->getRepresentation<ImageRAM>();
	
	

    // Prepare the output, it has the same dimensions and datatype as the output
    // and an editable version is retrieved analogous to a volume
    auto outImage = tex->clone();
    auto outLayer = outImage->getColorLayer();
    auto lr = outLayer->getEditableRepresentation<LayerRAM>();

    // TODO: Implement LIC and FastLIC
    // This code instead sets all pixels to the same gray value
    
	std::vector<std::vector<vec4> > licTexture;

	clock_t begin = clock();
	if (propUseFastLIC.get())
		licTexture = FastLIC(tr, texDims_, vr, vectorFieldDims_);
	else
		licTexture = LIC(tr, texDims_, vr, vectorFieldDims_);
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	LogProcessorInfo("Elapsed seconds for computation: "+ std::to_string(elapsed_secs)+" .");
	//double val11 = licTexture[1][1][0], val3030= licTexture[30][30][0];
	if (propApplyContrastEnh.get())
		applyContrastEnhancement(licTexture, texDims_, propContrastMean.get(), propContrastStd.get());

	//double val11_ = licTexture[1][1][0], val3030_ = licTexture[30][30][0];

	//LogProcessorInfo(" - "+std::to_string(val11) + " == " + std::to_string(val11_) + " | "+ std::to_string(val3030) + " == " + std::to_string(val3030_) + "");

	for (auto j = 0; j < texDims_.y; j++)
    {
        for (auto i = 0; i < texDims_.x; i++)
        {
			vec4 val = licTexture[i][j];
			//vec4 val = tr->readPixel(size2_t(i,j), LayerType::Color);
			lr->setFromDVec4(size2_t(i, j), dvec4(val[0], val[1], val[2], 255));
        }
    }
	licOut_.setData(outImage);

}

} // namespace

