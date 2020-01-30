//General
#include <iostream> 
#include <string> 
#include <itkImage.h> 
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
#include<itkAddImageFilter.h>
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
int seedPointsIndex = 3;

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

// calculation
std::vector<int> GetCoordinationArray(string coordinations);
int CalculateStandardVariation(ImageType::Pointer image, int mean, int x, int y, int z, int numberOfPixels);
int CalculatePixelMean(ImageType::Pointer image, int x, int y, int z, int numberOfPixels);

// segmentation
double segmentationCoef = 1;
double segmentationCoef2 = 0.4;
ImageType::Pointer GetLogicSumImage(std::vector<ImageType::Pointer> images);
typename ImageType::Pointer ConnectedThreshold(ImageType::Pointer image, int x, int y, int z, int downside, int updside);
typename ImageType::Pointer ConfidenceConnected(ImageType::Pointer image, int x, int y, int z);

// postprocessing
typename ImageType::Pointer BinaryOpen(ImageType::Pointer image);
typename ImageType::Pointer BinaryClose(ImageType::Pointer image);

// walidacja
typename ImageType::Pointer RescaleBinaryMaskTo255(ImageType::Pointer image);
double DiceResult(ImageType::Pointer image1, ImageType::Pointer image2);
string logs = "";
#pragma endregion

#pragma region InputArgumentsValidation

string GetArgumentInfoMessage(string message) {
	std::stringstream error;
	error << message
		<< "GliomaSegmentation: Polautomatyczna metoda segmentacji glejaka wielorozdzielczego \t\n"
		<< ""
		<< "Argumenty: \t\n"
		<< "- sciezka do folderu z seria plikow DICOM \t\n"
		<< "- sciezka do folderu wynikowego \t\n"
		<< "- punkty startowe segmentacji x;y;z \t\n"
		<< ""
		<< "[-c] - wspoczynnik progu \t\n"
		<< "[-ot] - otoczka wypukla z punktami startowymi segmentacji x;y;z \t\n"
		<< "[-cot] - wspoczynnik progu otoczki wypuklej \t\n"
		<< "[-D] - sciezka do folderu z seria plikow masek eksperckich, wedlug ktorych zostanie obliczona wartosc DICE \t\n";

	return error.str();
}

bool ValidateArguments(int argc, char *argv[]) {

	int minimumRequiredParameters = seedPointsIndex;

	if (argc < seedPointsIndex + 1) {
		std::cout << GetArgumentInfoMessage("Za malo argumentow wejsciowych\t\n");

		return false;
	}
}
#pragma endregion

int main(int argc, char *argv[]) {

	if (!ValidateArguments(argc, argv)) {
		return EXIT_FAILURE;
	}

	try {
		string pathToImages = argv[pathToImagesIndex];
		string pathToResults = argv[pathToResultIndex];

		std::cout << "Obraz wejsciowy: " << pathToImages << "\t\n";

		std::vector<ImageType::Pointer> segmentedImages;

		ImageType::Pointer image = RescaleBinaryMaskTo255(ReadImage(pathToImages));
		SaveImage(image, pathToResults, "obraz_wejsciowy");

		ImageType::Pointer anisotrophyDyfusion = AnisotrophyDyfusion(image);
		SaveImage(anisotrophyDyfusion, pathToResults, "obraz_po_dyfuzji");

		ImageType::Pointer imageSharped = ImageSharpening(anisotrophyDyfusion);
		SaveImage(imageSharped, pathToResults, "obraz_po_wyostrzeniu");

		ImageType::Pointer histogramMatched = HistogramMatching(imageSharped, image);
		SaveImage(histogramMatched, pathToResults, "obraz_po_korekcji_histogramu");

		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-c") == 0)
			{
				segmentationCoef = std::atof(argv[i + 1]);
			}
			else if (strcmp(argv[i], "-cot") == 0)
			{
				segmentationCoef2 = std::atof(argv[i + 1]);
			}
		}

		// segmentation
		std::vector<int> coordinationVektor = GetCoordinationArray(argv[seedPointsIndex]);
		if (coordinationVektor.size() < 3) {
			std::cout << GetArgumentInfoMessage("Za malo punktow startowych\t\n");
			return EXIT_FAILURE;
		}

		int x = coordinationVektor[0];
		int y = coordinationVektor[1];
		int z = coordinationVektor[2];

		int numberOfPixel = 2;
		int mean = CalculatePixelMean(image, x, y, z, numberOfPixel);
		int stv = CalculateStandardVariation(image, mean, x, y, z, numberOfPixel);

		logs += ("Srednia intensywnosci pikseli wokol punktu: " + std::to_string(mean));
		logs += ("Odchylenie standardowe intensywnosci pikseli wokol punktu: " + std::to_string(stv));

		std::cout << "Srednia intensywnosci pikseli wokol punktu: " + std::to_string(mean) << "\t\n";
		std::cout << "Odchylenie intensywnosci pikseli wokol punktu: " + std::to_string(stv) << "\t\n";

		int low = mean - (segmentationCoef * stv);
		int up = mean + (segmentationCoef * stv);
		std::cout << "Wspoczynnik progu otoczki: " + std::to_string(segmentationCoef) << "\t\n";

		ImageType::Pointer connectedThreshold = ConnectedThreshold(histogramMatched, x, y, z, low, up);
		SaveImage(connectedThreshold, pathToResults, ("obraz_po_segmentacji"));
		segmentedImages.push_back(connectedThreshold);

		ImageType::Pointer segmentationResultImage = connectedThreshold;

		ImageType::Pointer binaryOpen;

		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-ot") == 0)
			{
				std::vector<int> coordinationVektor = GetCoordinationArray(argv[i + 1]);
				if (coordinationVektor.size() < 3) {
					std::cout << GetArgumentInfoMessage("Za malo punktow startowych\t\n");
					return EXIT_FAILURE;
				}

				int x = coordinationVektor[0];
				int y = coordinationVektor[1];
				int z = coordinationVektor[2];

				int numberOfPixel = 2;
				int mean = CalculatePixelMean(image, x, y, z, numberOfPixel);
				int stv = CalculateStandardVariation(image, mean, x, y, z, numberOfPixel);

				std::cout << "Srednia intensywnosci pikseli wokol otoczki: " + std::to_string(mean) << "\t\n";
				std::cout << "Odchylenie standardowe intensywnosci pikseli wokol otoczki: " + std::to_string(stv) << "\t\n";
				logs += "Srednia intensywnosci pikseli wokol otoczki: " + std::to_string(mean) + "\t\n";
				logs += "Odchylenie standardowe intensywnosci pikseli wokol otoczki: " + std::to_string(stv) + "\t\n";

				std::cout << "Wspoczynnik progu otoczki: " + std::to_string(segmentationCoef2) << "\t\n";

				int low = mean -(segmentationCoef2 * stv);
				int up = mean + (segmentationCoef2 * stv);

				std::cout << "Prog dolny otoczki: " + std::to_string(low) << "\t\n";
				std::cout << "Prog gorny otoczki: " + std::to_string(up) << "\t\n";

				ImageType::Pointer connectedThreshold = ConnectedThreshold(histogramMatched, x, y, z, low, up);
				segmentedImages.push_back(connectedThreshold);
				SaveImage(connectedThreshold, pathToResults, ("segmentacja_otoczki"));

				segmentationResultImage = GetLogicSumImage(segmentedImages);
				SaveImage(segmentationResultImage, pathToResults, "suma_masek_obrazu");

				binaryOpen = BinaryOpen(segmentationResultImage);
				SaveImage(binaryOpen, pathToResults, "obraz_po_operacji_otwarcia");
			}
			else if (strcmp(argv[i], "-D") == 0)
			{
				string pathToMasks = argv[i + 1];
				ImageType::Pointer mask = ReadImage(pathToMasks);
				SaveImage(mask, pathToResults, "maski_obrazu");

				if (!binaryOpen) {
					binaryOpen = BinaryOpen(segmentationResultImage);
					SaveImage(binaryOpen, pathToResults, "obraz_po_operacji_otwarcia");
				}

				double diceResult = DiceResult(binaryOpen, mask);
				std::cout << "Wspoczynnik DICE: " + std::to_string(diceResult) << "\t\n\t\n\t\n";
				logs += "Wspoczynnik DICE: " + std::to_string(diceResult);
			}
			else if (strcmp(argv[i], "-v") == 0)
			{
				std::cout << logs;
			}
		}
		if (!binaryOpen) {
			binaryOpen = BinaryOpen(segmentationResultImage);
			SaveImage(binaryOpen, pathToResults, "obraz_po_operacji_otwarcia");
		}

		std::cout << "Koniec\t\n\t\n" << std::endl;
	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
		return EXIT_FAILURE;
	}

	//std::cin.get();
	return EXIT_SUCCESS;
}


#pragma region ReadWriteMethods

typename ImageType::Pointer ReadImage(string pathToImages) {

	typedef itk::ImageSeriesReader<ImageType> ReaderTypeSeries;

	itk::GDCMSeriesFileNames::Pointer gdcmSeriesFileNames = itk::GDCMSeriesFileNames::New();
	gdcmSeriesFileNames->SetDirectory(pathToImages);
	itk::SerieUIDContainer series = gdcmSeriesFileNames->GetSeriesUIDs();
	int imagesSizeInDir = gdcmSeriesFileNames->GetFileNames(series[0]).size();

	int counter = 0;
	logs += ("\t\nPrzykladowy odczytany plik: " + gdcmSeriesFileNames->GetFileNames(series[0])[0]);
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

	//std::cout << "Zapisano obraz o nazwie: " << fileName << "\t\n";
	logs += "Zapisano obraz o nazwie: " + fileName + "\t\n";
}
#pragma endregion

#pragma region Preprocessing

typename ImageType::Pointer AnisotrophyDyfusion(ImageType::Pointer image) {

	unsigned int numberOfIteration = 5; // typically set to 5;
	double conductanceParameter = 4;
	double timeStep = 0.125; // Typical values for the time step are 0.25 in 2D images and 0.125 in 3D images. T

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

typename ImageType::Pointer ConnectedThreshold(ImageType::Pointer image, int x, int y, int z, int downside, int upside) {

	using ConnectedFilterType = itk::ConnectedThresholdImageFilter<ImageType, ImageType>;
	ConnectedFilterType::Pointer connectedThreshold = ConnectedFilterType::New();
	connectedThreshold->SetInput(image);
	ImageType::IndexType index;
	index[0] = x;
	index[1] = y;
	index[2] = z;
	connectedThreshold->SetSeed(index);
	connectedThreshold->SetReplaceValue(255);

	connectedThreshold->SetLower(downside);
	connectedThreshold->SetUpper(upside);

	connectedThreshold->Update();
	SaveImage(connectedThreshold->GetOutput(), "../wyniki/temp", "obraz_po_segmentacji_connectedthreshold_" + std::to_string(downside) + std::to_string(upside));

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

	using AddImageFilterType = itk::AddImageFilter<ImageType, ImageType>;
	AddImageFilterType::Pointer addFilter = AddImageFilterType::New();

	addFilter->SetInput1(images[0]);
	addFilter->SetInput2(images[1]);
	addFilter->Update();

	if (images.size() > 2) {
		for (int i = 2; i <= images.size(); i++) {
			addFilter->SetInput1(logicSumImage);
			addFilter->SetInput2(images[i]);
			addFilter->Update();
			logicSumImage = addFilter->GetOutput();
		}
	}

	logicSumImage = addFilter->GetOutput();

	return logicSumImage;

}

#pragma endregion

#pragma region CalculateParameters

int CalculatePixelMean(ImageType::Pointer image, int x, int y, int z, int numberOfPixels) {
	ImageType::IndexType index;

	index[0] = x;
	index[1] = y;
	index[2] = z;

	std::cout << "Wartosc intensywnosci wskazanego piksela: " + std::to_string(image->GetPixel(index)) << "\t\n";


	int sum = 0;
	int counter = 0;

	int lowX = x - numberOfPixels;
	int upX = x + numberOfPixels;

	int lowY = y - numberOfPixels;
	int upY = y + numberOfPixels;

	for (int i = lowX; i <= upX; i++) {
		for (int j = lowY; j <= upY; j++) {
			index[0] = i;
			index[1] = j;

			sum += image->GetPixel(index);
			counter += 1;
		}
	}

	int mean = sum / counter;
	return mean;
}

int CalculateStandardVariation(ImageType::Pointer image, int mean, int x, int y, int z, int numberOfPixels) {
	ImageType::IndexType index;
	index[2] = z;

	int sum = 0;

	int lowX = x - numberOfPixels;
	int upX = x + numberOfPixels;

	int lowY = y - numberOfPixels;
	int upY = y + numberOfPixels;

	for (int i = lowX; i <= upX; i++) {
		for (int j = lowY; j <= upY; j++) {
			index[0] = i;
			index[1] = j;

			int value = image->GetPixel(index) - mean;
			sum += std::pow(value, 2);
		}
	}

	int stv = std::sqrt(sum);
	return stv;
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
	
	openingFilter->SetBackgroundValue(255);
	openingFilter->SetForegroundValue(0);
	openingFilter->Update();

	return openingFilter->GetOutput();
}

typename ImageType::Pointer BinaryClose(ImageType::Pointer image) {

	int radius = 1;

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

		closingFilter->SetForegroundValue(0);

		closingFilter->Update();

		SaveImage(closingFilter->GetOutput(), "..\\wyniki\\temp", "obraz_po_domknieciu_" + std::to_string(radius));

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