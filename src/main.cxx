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

#pragma region GlobalTypeDefinitions
typedef std::string string;
typedef signed short PixelType;
typedef itk::Image<PixelType, 3> ImageType;
#pragma endregion

#pragma region InputArgumentsValidation

bool ValidateArguments(int argc, char *argv[]) {
	if (argc < 5) {
		std::cout << "Za ma³o argumentów wejœciowych \t\n";
		std::cout << "Argumenty: \t\n";
		std::cout << "- sciezka do folderu z seri¹ plików DICOM \t\n";
		std::cout << "- sciezka do folderu wynikowego \t\n";
		std::cout << "- punkty startowe segmentacji x,y,z \t\n";
		return false;
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
	seriesWriter->SetFileName(pathToResults + "/"+ fileName+".vtk");
	seriesWriter->Update();

	std::cout << "Zapisano obraz o nazwie: " << fileName << "\t\n";
}


#pragma endregion

#pragma region Preprocessing

typename ImageType::Pointer AnisotrophyDyfusion(ImageType::Pointer image) {

	unsigned int numberOfIteration = 10;
	double conductanceParameter = 2;
	double timeStep = 0.125;

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

	filter->ThresholdAtMeanIntensityOn(); //W³¹cza metodê wykluczania t³a. Histogram referencyjny zwrócony z tego filtra rozszerzy pierwsz¹ i ostatni¹ granicê bin, 
	//aby obj¹æ minimalne i maksymalne wartoœci intensywnoœci ca³ego obrazu referencyjnego, ale do wype³nienia histogramu zostan¹ u¿yte tylko 
	//wartoœci intensywnoœci wiêksze ni¿ œrednia.

	filter->SetNumberOfHistogramLevels(5); //ustawia liczbê pojemników u¿ywanych podczas tworzenia histogramów obrazów Ÿród³owych i referencyjnych.
	filter->SetNumberOfMatchPoints(5); // reguluje liczbê dopasowywanych wartoœci kwantyli.

	
	filter->Update();
	return filter->GetOutput();
}

#pragma endregion

#pragma region Segmentation

typename ImageType::Pointer ConnectedThreshold(ImageType::Pointer image, int x, int y, int z) {

	using ConnectedFilterType = itk::ConnectedThresholdImageFilter<ImageType, ImageType>;
	ConnectedFilterType::Pointer connectedThreshold = ConnectedFilterType::New();
	connectedThreshold->SetInput(image);
	connectedThreshold->SetLower(50);
	connectedThreshold->SetUpper(150);
	ImageType::IndexType index;
	index[0] = x;
	index[1] = y;
	index[2] = z;
	connectedThreshold->SetSeed(index);
	connectedThreshold->SetReplaceValue(255);
	connectedThreshold->Update();
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

	int radius = 5;

	using StructuringElementType = itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension>;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(radius);
	structuringElement.CreateStructuringElement();

	using BinaryMorphologicalClosingImageFilterType = itk::BinaryMorphologicalClosingImageFilter<ImageType, ImageType, StructuringElementType>;
	BinaryMorphologicalClosingImageFilterType::Pointer closingFilter = BinaryMorphologicalClosingImageFilterType::New();
	closingFilter->SetInput(image);
	closingFilter->SetKernel(structuringElement);
	closingFilter->Update();

	return closingFilter->GetOutput();
}

#pragma endregion


int main(int argc, char *argv[]) {

	if (!ValidateArguments(argc, argv)) {
		return EXIT_FAILURE;
	}

	try {
		string pathToImages = argv[1];
		string pathToResults = argv[2];
		int x = std::atoi(argv[3]);
		int y = std::atoi(argv[4]);
		int z = std::atoi(argv[5]);

		ImageType::Pointer image = ReadImage(pathToImages);
		SaveImage(image, pathToResults, "obraz_wejsciowy");

		/*ImageType::Pointer anisotrophyDyfusion = AnisotrophyDyfusion(image);
		SaveImage(anisotrophyDyfusion, pathToResults, "obraz_po_dyfuzji");*/

		ImageType::Pointer imageSharped = ImageSharpening(image);
		SaveImage(imageSharped, pathToResults, "obraz_po_wyostrzeniu");

		ImageType::Pointer histogramMatched = HistogramMatching(imageSharped, image);
		SaveImage(histogramMatched, pathToResults, "obraz_po_korekcji_histogramu");

		
		ImageType::Pointer connectedThreshold = ConnectedThreshold(image, x, y, z);
		SaveImage(connectedThreshold, pathToResults, "obraz_po_segmentacji_connectedthreshold");

		ImageType::Pointer confidenceConnected = ConfidenceConnected(imageSharped, x, y, z);
		SaveImage(confidenceConnected, pathToResults, "obraz_po_segmentacji_confidenceConnected");


		ImageType::Pointer binaryOpen = BinaryOpen(connectedThreshold);
		SaveImage(binaryOpen, pathToResults, "obraz_po_operacji_otwarcia");

		ImageType::Pointer binaryClose = BinaryClose(binaryOpen);
		SaveImage(binaryClose, pathToResults, "obraz_po_operacji_zamkniecia");

	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
		return EXIT_FAILURE;
	}

	std::cin.get();
	return EXIT_SUCCESS;
}


