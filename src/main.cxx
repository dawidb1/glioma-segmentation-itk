#include <iostream> // std::cout, std::cin
#include <string> // std::string
#include <itkImage.h> // itk::Image

#include<itkImageSeriesReader.h>
#include<itkImageSeriesWriter.h>
#include<itkGDCMImageIO.h>

#include<itkGDCMSeriesFileNames.h>
#include<itkNumericSeriesFileNames.h>

#include<itkRescaleIntensityImageFilter.h>
#include<itkGradientAnisotropicDiffusionImageFilter.h>
#include<itkLaplacianSharpeningImageFilter.h>
#include<itkHistogramMatchingImageFilter.h>

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

typename ImageType::Pointer HistogramMatching(ImageType::Pointer image) {

	using HistogramMatching = itk::HistogramMatchingImageFilter<ImageType, ImageType, ImageType::PixelType>;
	HistogramMatching::Pointer filter = HistogramMatching::New();
	filter->SetInput(image);
	filter->SetReferenceImage(image);

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

typename ImageType::Pointer RegionGrowing(ImageType::Pointer image) {


	return image;
}

#pragma endregion

#pragma region PostProcessing

typename ImageType::Pointer BinaryOpen(ImageType::Pointer image) {


	return image;
}

typename ImageType::Pointer BinaryClose(ImageType::Pointer image) {


	return image;
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

		ImageType::Pointer anisotrophyDyfusion = AnisotrophyDyfusion(image);
		SaveImage(anisotrophyDyfusion, pathToResults, "obraz_po_dyfuzji");

		ImageType::Pointer imageSharped = ImageSharpening(image);
		SaveImage(imageSharped, pathToResults, "obraz_po_wyostrzeniu");

		ImageType::Pointer histogramMatched = HistogramMatching(image);
		SaveImage(histogramMatched, pathToResults, "obraz_po_korekcji_histogramu");

	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
		return EXIT_FAILURE;
	}

	std::cin.get();
	return EXIT_SUCCESS;
}


