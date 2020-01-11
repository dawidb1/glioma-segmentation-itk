#include <iostream> // std::cout, std::cin
#include <string> // std::string
#include <itkImage.h> // itk::Image

#include<itkImageSeriesReader.h>
#include<itkImageSeriesWriter.h>
#include<itkGDCMImageIO.h>

#include<itkGDCMSeriesFileNames.h>
#include<itkNumericSeriesFileNames.h>

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

void SaveImage(ImageType::Pointer resultImage, string pathToResults) {

	typedef itk::ImageFileWriter<ImageType>WriterType;

	WriterType::Pointer seriesWriter = WriterType::New();
	seriesWriter->SetInput(resultImage);
	seriesWriter->SetFileName(pathToResults + "/seriaWolumen.vtk");
	seriesWriter->Update();
}


#pragma endregion

#pragma region Preprocessing

typename ImageType::Pointer AnisotrophyDyfusion(ImageType::Pointer image) {


	return image;
}

typename ImageType::Pointer ImageSharpening(ImageType::Pointer image) {


	return image;
}

typename ImageType::Pointer HistogramMatching(ImageType::Pointer image) {


	return image;
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
		SaveImage(image, pathToResults);

	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
		return EXIT_FAILURE;
	}

	std::cout << "Sukces!";
	std::cin.get();
	return EXIT_SUCCESS;
}


