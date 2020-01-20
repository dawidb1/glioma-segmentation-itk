//General
#include <iostream> // std::cout, std::cin
#include <string> // std::string
#include <itkImage.h> // itk::Image
//Read Write
#include<itkImageSeriesReader.h>
#include<itkImageSeriesWriter.h>
#include<itkGDCMImageIO.h>
#include<itkGDCMSeriesFileNames.h>
#include<itkNumericSeriesFileNames.h>
//Preprocessing
#include<itkRescaleIntensityImageFilter.h>
#include<itkGradientAnisotropicDiffusionImageFilter.h>
#include<itkLaplacianSharpeningImageFilter.h>
#include<itkHistogramMatchingImageFilter.h>
//Segmentation
#include<itkConnectedThresholdImageFilter.h>
#include<itkConfidenceConnectedImageFilter.h>
#include<itkRegionGrowImageFilter.h>
//Postprocessing
#include<itkBinaryBallStructuringElement.h>
#include<itkBinaryMorphologicalClosingImageFilter.h>
#include<itkBinaryMorphologicalOpeningImageFilter.h>
//DICE
#include<itkRescaleIntensityImageFilter.h>
#include<itkLabelOverlapMeasuresImageFilter.h>

#pragma region GlobalTypeDefinitions
int pathToImagesIndex = 1;
int pathToResultIndex = 2;
int numberOfSeedPointsIndex = 3;

typedef std::string string;
typedef signed short PixelType;
typedef itk::Image<PixelType, 3> ImageType;

// IO
ImageType::Pointer ReadImage(string pathToImages);
void SaveImage(ImageType::Pointer resultImage, string pathToResults, string fileName);

// preprocessing
typename ImageType::Pointer AnisotrophyDyfusion(ImageType::Pointer image);
typename ImageType::Pointer ImageSharpening(ImageType::Pointer image);
typename ImageType::Pointer HistogramMatching(ImageType::Pointer image, ImageType::Pointer referenceImage);

// segmentation
typename ImageType::Pointer ConnectedThreshold(ImageType::Pointer image, int x, int y, int z);
typename ImageType::Pointer ConfidenceConnected(ImageType::Pointer image, int x, int y, int z);

// postprocessing
typename ImageType::Pointer BinaryOpen(ImageType::Pointer image);
typename ImageType::Pointer BinaryClose(ImageType::Pointer image);

// walidacja
typename ImageType::Pointer RescaleBinaryMaskTo255(ImageType::Pointer image);
double DiceResult(ImageType::Pointer image1, ImageType::Pointer image2);
#pragma endregion

#pragma region InputArgumentsValidation

string GetArgumentInfoMessage(string message) {
	std::stringstream error;
	error << message
		<< "Argumenty: \t\n" 
		<< "- sciezka do folderu z seria plikow DICOM \t\n"
		<< "- sciezka do folderu wynikowego \t\n"
		<< "- ilosc punktow startowych \t\n"
		<< "- punkty startowe segmentacji x;y;z \t\n"
		<< "- sciezka do folderu z seria plikow obrys�w eksperckich";
	return error.str();
}

bool ValidateArguments(int argc, char *argv[]) {

	int minimumRequiredParameters = numberOfSeedPointsIndex;

	if (argc < numberOfSeedPointsIndex +1) {
		std::cout << GetArgumentInfoMessage("Za malo argumentow wejsciowych\t\n");
		return false;
	}

	int numberOfSeedPoints = std::atoi(argv[numberOfSeedPointsIndex]);
	minimumRequiredParameters += numberOfSeedPoints;

	if (argc < minimumRequiredParameters+1) {
		std::cout << GetArgumentInfoMessage("Za ma�o punkt�w startowych\t\n");
	}
}

#pragma endregion

#pragma region ReadWriteMethods

typename ImageType::Pointer ReadImage(string pathToImages) {

	typedef itk::ImageSeriesReader<ImageType> ReaderTypeSeries;

	itk::GDCMSeriesFileNames::Pointer gdcmSeriesFileNames = itk::GDCMSeriesFileNames::New();
	gdcmSeriesFileNames->SetDirectory(pathToImages);
	itk::SerieUIDContainer series = gdcmSeriesFileNames->GetSeriesUIDs();
	int imagesSizeInDir = gdcmSeriesFileNames->GetFileNames(series[0]).size();

	int counter = 0;
	std::cout << "Sciezki do plikow" << std::endl;
	for (string x : gdcmSeriesFileNames->GetFileNames(series[0])) {
		std::cout << x << std::endl;
		counter++;
	}

	ReaderTypeSeries::Pointer seriesReader = ReaderTypeSeries::New();
	seriesReader->SetFileNames(gdcmSeriesFileNames->GetFileNames(series[0]));
	seriesReader->Update();

	return seriesReader->GetOutput();
}

void SaveImage(ImageType::Pointer resultImage, string pathToResults, string fileName) {

	typedef itk::ImageFileWriter<ImageType>WriterType;

	WriterType::Pointer seriesWriter = WriterType::New();
	seriesWriter->SetInput(resultImage);
	seriesWriter->SetFileName(pathToResults + "/" + fileName + ".vtk");
	seriesWriter->Update();

	std::cout << "Zapisano obraz o nazwie: " << fileName << "\t\n";
}


#pragma endregion

#pragma region Preprocessing

typename ImageType::Pointer AnisotrophyDyfusion(ImageType::Pointer image) {

	unsigned int numberOfIteration = 5; // typically set to 5;
	double conductanceParameter = 8;
	double timeStep = 0.02; // Typical values for the time step are 0.25 in 2D images and 0.125 in 3D images. T

	using FilterType = itk::GradientAnisotropicDiffusionImageFilter<ImageType, ImageType>;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(image);
	filter->SetNumberOfIterations(numberOfIteration);
	filter->SetTimeStep(timeStep);
	filter->SetConductanceParameter(conductanceParameter);
	filter->Update();

	return filter->GetOutput();
}

typename ImageType::Pointer ImageSharpening(ImageType::Pointer image) {

	using LaplacianSharpeningFilter = itk::LaplacianSharpeningImageFilter<ImageType, ImageType>;
	LaplacianSharpeningFilter::Pointer filter = LaplacianSharpeningFilter::New();

	filter->SetInput(image);
	filter->Update();

	return filter->GetOutput();
}

typename ImageType::Pointer HistogramMatching(ImageType::Pointer image, ImageType::Pointer referenceImage) {

	using HistogramMatching = itk::HistogramMatchingImageFilter<ImageType, ImageType, ImageType::PixelType>;
	HistogramMatching::Pointer filter = HistogramMatching::New();
	filter->SetInput(image);
	filter->SetReferenceImage(referenceImage);

	filter->ThresholdAtMeanIntensityOn(); //W��cza metod� wykluczania t�a. Histogram referencyjny zwr�cony z tego filtra rozszerzy pierwsz� i ostatni� granic� bin, 
	//aby obj�� minimalne i maksymalne warto�ci intensywno�ci ca�ego obrazu referencyjnego, ale do wype�nienia histogramu zostan� u�yte tylko 
	//warto�ci intensywno�ci wi�ksze ni� �rednia.

	filter->SetNumberOfHistogramLevels(1024); //ustawia liczb� pojemnik�w u�ywanych podczas tworzenia histogram�w obraz�w �r�d�owych i referencyjnych.
	filter->SetNumberOfMatchPoints(10); // reguluje liczb� dopasowywanych warto�ci kwantyli.


	filter->Update();
	return filter->GetOutput();
}

#pragma endregion

#pragma region Segmentation

typename ImageType::Pointer ConnectedThreshold(ImageType::Pointer image, int x, int y, int z, int low, int up) {

	using ConnectedFilterType = itk::ConnectedThresholdImageFilter<ImageType, ImageType>;
	ConnectedFilterType::Pointer connectedThreshold = ConnectedFilterType::New();
	connectedThreshold->SetInput(image);
	ImageType::IndexType index;
	index[0] = x;
	index[1] = y;
	index[2] = z;
	connectedThreshold->SetSeed(index);
	connectedThreshold->SetReplaceValue(255);

	connectedThreshold->SetLower(low);
	connectedThreshold->SetUpper(up);

	connectedThreshold->Update();
	SaveImage(connectedThreshold->GetOutput(), "..\\wyniki\\1", "obraz_po_segmentacji_connectedthreshold_" + std::to_string(low) + std::to_string(up));

	return connectedThreshold->GetOutput();
}

typename ImageType::Pointer ConfidenceConnected(ImageType::Pointer image, int x, int y, int z) {

	using ConfidenceConnectedFilterType = itk::ConfidenceConnectedImageFilter<ImageType, ImageType>;
	ConfidenceConnectedFilterType::Pointer confidenceConnectedFilter = ConfidenceConnectedFilterType::New();
	confidenceConnectedFilter->SetInitialNeighborhoodRadius(3);
	confidenceConnectedFilter->SetMultiplier(2);
	confidenceConnectedFilter->SetNumberOfIterations(30);
	confidenceConnectedFilter->SetReplaceValue(255);
	ImageType::IndexType index;
	index[0] = x;
	index[1] = y;
	index[2] = z;
	confidenceConnectedFilter->SetSeed(index);
	confidenceConnectedFilter->SetInput(image);
	confidenceConnectedFilter->Update();
	return confidenceConnectedFilter->GetOutput();
}

ImageType::Pointer GetLogicSumImage(std::vector<ImageType::Pointer> images) {

	ImageType::Pointer logicSumImage;

	if (images.size() == 1) {
		return images[0];
	}

	//TODO suma logiczna obraz�w je�li jest ich wi�cej ni� jeden
	logicSumImage = images[0];
	return logicSumImage;

}

#pragma endregion

#pragma region CalculateParameters

int CalculatePixelMean(ImageType::Pointer image, int x, int y, int z, int numberOfPixels) {
	ImageType::IndexType index;
	index[2] = z;

	int sum = 0;
	int counter = 0;

	int lowX = x - numberOfPixels;
	int upX = x + numberOfPixels;

	int lowY = y - numberOfPixels;
	int upY = y + numberOfPixels;

	for (int i = lowX; i < upX; i++) {
		for (int j = lowY; j < upY; j++) {
			index[0] = i;
			index[1] = j;

			sum += image->GetPixel(index);
			counter += 1;
		}
	}

	int mean = sum / counter;
	return mean;
}

#pragma endregion

#pragma region PostProcessing

typename ImageType::Pointer BinaryOpen(ImageType::Pointer image) {

	int radius = 5;

	using StructuringElementType = itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension>;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(radius);
	structuringElement.CreateStructuringElement();

	using BinaryMorphologicalOpeningImageFilterType = itk::BinaryMorphologicalOpeningImageFilter<ImageType, ImageType, StructuringElementType>;
	BinaryMorphologicalOpeningImageFilterType::Pointer openingFilter = BinaryMorphologicalOpeningImageFilterType::New();
	openingFilter->SetInput(image);
	openingFilter->SetKernel(structuringElement);
	openingFilter->Update();

	return openingFilter->GetOutput();
}

typename ImageType::Pointer BinaryClose(ImageType::Pointer image) {

	int radius = 1;

	//using StructuringElementType = itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension>;
	//StructuringElementType structuringElement;
	//structuringElement.SetRadius(radius);
	//structuringElement.CreateStructuringElement();

	//using BinaryMorphologicalClosingImageFilterType = itk::BinaryMorphologicalClosingImageFilter<ImageType, ImageType, StructuringElementType>;
	//BinaryMorphologicalClosingImageFilterType::Pointer closingFilter = BinaryMorphologicalClosingImageFilterType::New();
	//closingFilter->SetInput(image);
	//closingFilter->SetKernel(structuringElement);
	//closingFilter->Update();
	using StructuringElementType = itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension>;
	StructuringElementType structuringElement;

	using BinaryMorphologicalClosingImageFilterType = itk::BinaryMorphologicalClosingImageFilter<ImageType, ImageType, StructuringElementType>;
	BinaryMorphologicalClosingImageFilterType::Pointer closingFilter = BinaryMorphologicalClosingImageFilterType::New();

	for (size_t i = 0; i < 1; i++)
	{
		structuringElement.SetRadius(radius);
		structuringElement.CreateStructuringElement();

		closingFilter->SetInput(image);
		closingFilter->SetKernel(structuringElement);
		closingFilter->Update();

		SaveImage(closingFilter->GetOutput(), "..\\wyniki\\1", "obraz_po_domknieciu_" + std::to_string(radius));

		//low += 10;
		radius += 1;
	}

	return closingFilter->GetOutput();
}

#pragma endregion

#pragma region Walidacja

double DiceResult(ImageType::Pointer segmented, ImageType::Pointer originMask) {

	itk::LabelOverlapMeasuresImageFilter<ImageType>::Pointer overlap_filter = itk::LabelOverlapMeasuresImageFilter<ImageType>::New();
	overlap_filter->SetInput(0, RescaleBinaryMaskTo255(segmented));
	overlap_filter->SetInput(1, RescaleBinaryMaskTo255(originMask));
	overlap_filter->Update();
	return overlap_filter->GetDiceCoefficient();
}

typename ImageType::Pointer RescaleBinaryMaskTo255(ImageType::Pointer image) {
	using FilterType = itk::RescaleIntensityImageFilter<ImageType, ImageType>;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(image);
	filter->SetOutputMinimum(0);
	filter->SetOutputMaximum(255);
	filter->Update();

	return filter->GetOutput();
}
#pragma endregion

std::vector<int> GetCoordinationArray(string coordinations) {

	std::replace(coordinations.begin(), coordinations.end(), ';', ' '); 

	std::vector<int> array;
	std::stringstream ss(coordinations);
	int temp;
	while (ss >> temp)
		array.push_back(temp);

	return array;
}

int main(int argc, char *argv[]) {

	if (!ValidateArguments(argc, argv)) {
		return EXIT_FAILURE;
	}

	try {
		string pathToImages = argv[pathToImagesIndex];
		string pathToResults = argv[pathToResultIndex];
		int numberOfSeedPoints = std::atoi(argv[numberOfSeedPointsIndex]);

		std::vector<ImageType::Pointer> segmentedImages;

		ImageType::Pointer image = ReadImage(pathToImages);
		SaveImage(image, pathToResults, "obraz_wejsciowy");

		//ImageType::Pointer anisotrophyDyfusion = AnisotrophyDyfusion(image);
		//SaveImage(anisotrophyDyfusion, pathToResults, "obraz_po_dyfuzji");

		ImageType::Pointer imageSharped = ImageSharpening(image);
		SaveImage(imageSharped, pathToResults, "obraz_po_wyostrzeniu");

		ImageType::Pointer histogramMatched = HistogramMatching(imageSharped, image);
		SaveImage(histogramMatched, pathToResults, "obraz_po_korekcji_histogramu");

		for (int i = numberOfSeedPointsIndex + 1; i < numberOfSeedPointsIndex + numberOfSeedPoints + 1; i++) {

			std::vector<int> coordinationVektor = GetCoordinationArray(argv[i]);
			if (coordinationVektor.size() < 3) {
				std::cout << GetArgumentInfoMessage("Za ma�o punkt�w startowych\t\n");
				return EXIT_FAILURE;
			}

			int x = coordinationVektor[0];
			int y = coordinationVektor[1];
			int z = coordinationVektor[2];

			int numberOfPixel = 2;
			int mean = CalculatePixelMean(image, x, y, z, numberOfPixel);

			int low = 190;
			int up = 245;

			ImageType::Pointer connectedThreshold = ConnectedThreshold(imageSharped, x, y, z, low, up);
			SaveImage(connectedThreshold, pathToResults, ("obraz_po_segmentacji_connectedthreshold" + i));

			/*ImageType::Pointer confidenceConnected = ConfidenceConnected(imageSharped, x, y, z);
			 SaveImage(confidenceConnected, pathToResults, "obraz_po_segmentacji_confidenceConnected");*/

			segmentedImages.push_back(connectedThreshold);

		}

		ImageType::Pointer logicSumImage = GetLogicSumImage(segmentedImages);

		ImageType::Pointer binaryOpen = BinaryOpen(logicSumImage);
		SaveImage(binaryOpen, pathToResults, "obraz_po_operacji_otwarcia");

		ImageType::Pointer binaryClose = BinaryClose(binaryOpen);
		SaveImage(binaryClose, pathToResults, "obraz_po_operacji_zamkniecia");


		string pathToMasks;
		ImageType::Pointer mask;

		int restArguments = argc - numberOfSeedPointsIndex - numberOfSeedPoints - 1;

		if (restArguments > 0) {
			pathToMasks = argv[numberOfSeedPointsIndex + numberOfSeedPoints + 1];
			mask = ReadImage(pathToMasks);
			SaveImage(mask, pathToResults, "maski_obrazu");
		}

		if (!pathToMasks.empty())
		{
			double diceResult = DiceResult(binaryClose, mask);
			std::cout << "Wsp�czynnik DICE: " + std::to_string(diceResult) << "\t\n";
		}

		std::cout << "Koniec" << std::endl;
	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
		return EXIT_FAILURE;
	}

	std::cin.get();
	return EXIT_SUCCESS;


}
